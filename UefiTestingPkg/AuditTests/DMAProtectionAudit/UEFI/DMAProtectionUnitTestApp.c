/** @file -- DMAProtectionUnitTestApp.c

This is an EFI Shell application meant to check
1) BME Breakdown on ExitBootServices()
2) Check the Global Status Registers of the DRHDs to verify VTd is enabled
3) Check IVMD memory ranges are set as reserved

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/UnitTestBootLib.h>

#include <Protocol/PciIo.h>
#include <IndustryStandard/Pci22.h>

#include "DMAProtectionTest.h"

#define UNIT_TEST_APP_NAME                    "IOMMU BME and Register Status Unit Test Library Application"
#define UNIT_TEST_APP_VERSION                 "0.2"
#define DMA_UNIT_TEST_VARIABLE_PRE_EBS_NAME   L"DMAUnitTestVariablePreEBS"
#define DMA_UNIT_TEST_VARIABLE_POST_EBS_NAME  L"DMAUnitTestVariablePostEBS"

#define GET_MEMORY_MAP_RETRIES  4

extern EFI_GUID                          gDMAUnitTestVariableGuid;
EFI_HANDLE                               gImageHandle;

///================================================================================================
///================================================================================================
///
/// STRUCTURE DEFINITIONS
///
///================================================================================================
///================================================================================================

typedef struct BMEListNode_t_def
{
  EFI_PCI_IO_PROTOCOL           *PciIo;
  BOOLEAN                       BMEPreEBS;
  BOOLEAN                       BMEPostEBS;
  struct BMEListNode_t_def*     Next;
} BMEListNode;

typedef struct BME_TEST_CONTEXT_T_DEF
{
  UINT64            TestProgress;
}BME_TEST_CONTEXT;


///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================

UNIT_TEST_STATUS
EFIAPI
CheckBMETeardown (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS                  Status;
  EFI_HANDLE                 *HandleBuffer;
  UINTN                       HandleCount;
  UINTN                       Index;
  EFI_PCI_IO_PROTOCOL         *PciIo;
  PCI_TYPE00                  PciConfigHeader;
  UINT16                      CommandReg;
  EFI_MEMORY_DESCRIPTOR       *EfiMemoryMap;
  UINTN                       EfiMemoryMapSize;
  UINTN                       EfiMapKey;
  UINTN                       EfiDescriptorSize;
  UINT32                      EfiDescriptorVersion;
  BMEListNode*                Head = NULL;
  BMEListNode*                Tail = Head;
  UINT32                      Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE;
  VOID*                       PreValue = NULL;
  VOID*                       PostValue = NULL;
  UINTN                       PreVarSize = 0;
  UINTN                       PostVarSize = 0;
  BME_TEST_CONTEXT            BMEContext = (*(BME_TEST_CONTEXT *)Context);
  UINTN                       Retry;

  if(BMEContext.TestProgress == 0)
  {
    BMEContext.TestProgress++;
    SaveFrameworkState(&BMEContext, sizeof(BME_TEST_CONTEXT));

    //
    // Step 1: Get all PCI IO protocols
    //
    Status = gBS->LocateHandleBuffer( ByProtocol,
                                      &gEfiPciIoProtocolGuid,
                                      NULL,
                                      &HandleCount,
                                      &HandleBuffer );
    UT_ASSERT_NOT_EFI_ERROR(Status);

    //
    // Step 2: Find all devices that have class code for PCI-to-PCI bridges
    //
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiPciIoProtocolGuid,
                      (VOID **)&PciIo
                      );
      UT_ASSERT_NOT_EFI_ERROR(Status);

      PciIo->Pci.Read (PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof (PciConfigHeader)/sizeof (UINT32),
                        &PciConfigHeader);

      //For PCI-to-PCI Bridges
      if((PciConfigHeader.Hdr.HeaderType & HEADER_LAYOUT_CODE) == (HEADER_TYPE_PCI_TO_PCI_BRIDGE))
      {
          //
          //Step 3: Read Command Register
          //
          Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint16,
                        PCI_COMMAND_OFFSET,
                        1,
                        &CommandReg);
          UT_ASSERT_NOT_EFI_ERROR(Status);

          //Add to linked list to compare against after ExitBootServices
          BMEListNode* temp = (BMEListNode*)AllocateZeroPool(sizeof(BMEListNode));
          UT_ASSERT_NOT_NULL(temp);

          temp->PciIo = PciIo;
          temp->BMEPreEBS = (BOOLEAN)(CommandReg & EFI_PCI_COMMAND_BUS_MASTER);
          temp->Next = NULL;

          if(Head == NULL)
          {
            Head = temp;
            Tail = temp;
          }
          else
          {
            Tail->Next = temp;
            Tail = temp;
          }
      }
    }

    //
    // Step 4: Initialize the memory pool that will be saved to variables
    //
    BMEListNode*  Iterator = Head;
    UINTN Count = 0;
    while(Iterator != NULL)
    {
      Iterator = Iterator->Next;
      Count++;
    }
    BOOLEAN* BMEPreEBSStatusArray = (BOOLEAN*)AllocateZeroPool(sizeof(BOOLEAN)*Count);
    BOOLEAN* BMEPostEBSStatusArray = (BOOLEAN*)AllocateZeroPool(sizeof(BOOLEAN)*Count);
    UT_ASSERT_NOT_NULL(BMEPreEBSStatusArray);
    UT_ASSERT_NOT_NULL(BMEPostEBSStatusArray);

    //
    // Step 5: Set Next Boot to Boot to USB
    //
    SetBootNextDevice();

    //
    // Step 6: Get the EFI memory map.
    //
    Retry = 0;
    EfiMemoryMap = NULL;

    do {
        if (EfiMemoryMap != NULL) {
            FreePool (EfiMemoryMap);
        }

        EfiMemoryMapSize  = 0;
        EfiMemoryMap      = NULL;
        Status = gBS->GetMemoryMap (
                        &EfiMemoryMapSize,
                        EfiMemoryMap,
                        &EfiMapKey,
                        &EfiDescriptorSize,
                        &EfiDescriptorVersion
                        );
        if (Status != EFI_BUFFER_TOO_SMALL || !EfiMemoryMapSize) {
            DEBUG((DEBUG_ERROR, "GetMemoryMap Error\n"));
            return UNIT_TEST_ERROR_TEST_FAILED;
        }

        EfiMemoryMapSize += EfiMemoryMapSize + 64 * EfiDescriptorSize;
        EfiMemoryMap = AllocateZeroPool(EfiMemoryMapSize);
        UT_ASSERT_NOT_NULL(EfiMemoryMap);

        Status = gBS->GetMemoryMap (
                        &EfiMemoryMapSize,
                        EfiMemoryMap,
                        &EfiMapKey,
                        &EfiDescriptorSize,
                        &EfiDescriptorVersion
                        );
        UT_ASSERT_NOT_EFI_ERROR(Status);

        //
        // Step 7: Create exit boot services event
        //
        DEBUG((DEBUG_INFO, "Calling ExitBootServices - Retry = %d\n", Retry));
        Status = gBS->ExitBootServices (
                        gImageHandle,
                        EfiMapKey
                      );

    } while (EFI_ERROR(Status) && Retry++ < GET_MEMORY_MAP_RETRIES);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "ERROR - Exit Boot Services returned %r\n", Status));
    }

    //
    // Step 8: Get Post EBS BME Status
    //
    Iterator = Head;
    Count = 0;
    while(Iterator != NULL)
    {
      DEBUG((DEBUG_INFO, "Calling PciIo\n"));
      Status = Iterator->PciIo->Pci.Read (
                        Iterator->PciIo,
                        EfiPciIoWidthUint16,
                        PCI_COMMAND_OFFSET,
                        1,
                        &CommandReg);
      Iterator->BMEPostEBS = (BOOLEAN)(CommandReg & EFI_PCI_COMMAND_BUS_MASTER);
      Iterator = Iterator->Next;
      Count++;
    }

    //
    // Step 9: Flatten Linked List to write to variable
    //
    Iterator = Head;
    Count = 0;
    while(Iterator != NULL)
    {
      BMEPreEBSStatusArray[Count]   = Iterator->BMEPreEBS;
      BMEPostEBSStatusArray[Count] = Iterator->BMEPostEBS;
      Iterator = Iterator->Next;
      Count++;
    }

    //
    // Step 10: Since we are post exitbootservices we need to save the variable and reboot for further processing
    //
    Status = gRT->SetVariable(
                  DMA_UNIT_TEST_VARIABLE_PRE_EBS_NAME,
                  &gDMAUnitTestVariableGuid,
                  Attributes,
                  sizeof(BOOLEAN)*Count,
                  BMEPreEBSStatusArray
      );
    UT_ASSERT_NOT_EFI_ERROR(Status);

    Status = gRT->SetVariable(
                  DMA_UNIT_TEST_VARIABLE_POST_EBS_NAME,
                  &gDMAUnitTestVariableGuid,
                  Attributes,
                  sizeof(BOOLEAN)*Count,
                  BMEPostEBSStatusArray
      );
    UT_ASSERT_NOT_EFI_ERROR(Status);

    gRT->ResetSystem( EfiResetCold, EFI_SUCCESS, 0, NULL );
  }
  else
  {

    //
    // Step 1: Check if variable exists from previous run
    //
    Status = GetVariable3(
                  DMA_UNIT_TEST_VARIABLE_PRE_EBS_NAME,
                  &gDMAUnitTestVariableGuid,
                  &PreValue,
                  &PreVarSize,
                  NULL
                  );
    UT_ASSERT_NOT_EFI_ERROR(Status);

    Status = GetVariable3(
                  DMA_UNIT_TEST_VARIABLE_POST_EBS_NAME,
                  &gDMAUnitTestVariableGuid,
                  &PostValue,
                  &PostVarSize,
                  NULL
                  );
    UT_ASSERT_NOT_EFI_ERROR(Status);
    UT_ASSERT_EQUAL(PreVarSize, PostVarSize);

    //
    // Step 2: Verify BME was disabled during exitbootservies
    //
    UINTN i;
    BOOLEAN* PreBuffer = PreValue;
    BOOLEAN* PostBuffer = PostValue;
    for(i = 0; i < (PreVarSize)/(sizeof(BOOLEAN)); i++)
    {
      //BME Enabled before exit boot services
      UT_LOG_INFO(PreBuffer[i] ? "Pre-EBS BME %d: True\n" : "Pre-EBS BME %d: False\n", i);
      //BME Disabled after exit boot services
      UT_LOG_INFO(PostBuffer[i] ? "Post-EBS BME %d: True\n" : "Post-EBS BME %d: False\n", i);
      UT_ASSERT_FALSE(PostBuffer[i]);
    }

    //Free Buffer
    FreePool(PreValue);
    FreePool(PostValue);
    PreBuffer = NULL;
    PostBuffer = NULL;

    //
    // Step 3: Delete Variable
    //
    Status = gRT->SetVariable(
                    DMA_UNIT_TEST_VARIABLE_PRE_EBS_NAME,
                    &gDMAUnitTestVariableGuid,
                    0,
                    0,
                    NULL );

    Status = gRT->SetVariable(
                    DMA_UNIT_TEST_VARIABLE_POST_EBS_NAME,
                    &gDMAUnitTestVariableGuid,
                    0,
                    0,
                    NULL );
  }

  return UNIT_TEST_PASSED;
} // CheckBMETeardown()

///================================================================================================
///================================================================================================
///
/// TEST ENGINE
///
///================================================================================================
///================================================================================================


/**
  SampleUnitTestApp

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
DMAProtectionUnitTestApp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_SUITE_HANDLE      IommuTests;
  UNIT_TEST_FRAMEWORK_HANDLE  Fw = NULL;
  BME_TEST_CONTEXT            *BMEContext;

  gImageHandle = ImageHandle;

  //
  // Setup Unit Test Framework
  //
  DEBUG(( DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework( &Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the Unit Test Suite.
  //
  Status = CreateUnitTestSuite( &IommuTests, Fw, "IOMMU ACPI and Register tests", "IOMMU", NULL, NULL );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for IOMMU\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // Prepare for context buffer used for BME test
  //
  BMEContext = AllocateZeroPool(sizeof(BME_TEST_CONTEXT));
  if (BMEContext == NULL) {
    DEBUG((DEBUG_ERROR, "Failed in creating BME Context\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase( IommuTests, "All Hardware Definition Units Have IOMMU Enabled", "IOMMU.StatusRegister", CheckIOMMUEnabled, NULL, NULL, NULL );
  AddTestCase( IommuTests, "BME Teardown at ExitBootServices", "IOMMU.BMETeardown", CheckBMETeardown, NULL, NULL, BMEContext);
  AddTestCase( IommuTests, "Verify excluded ranges are marked reserved", "IOMMU.ExcludedRangeTest", CheckExcludedRegions, NULL, NULL, NULL);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites( Fw );

EXIT:
  if (Fw)
  {
    FreeUnitTestFramework( Fw );
  }

  return Status;
}
