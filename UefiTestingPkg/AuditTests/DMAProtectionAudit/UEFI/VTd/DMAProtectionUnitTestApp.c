/** @file -- DMAProtectionUnitTestApp.c
This is an EFI Shell application meant to check 
1) BME Breakdown on ExitBootServices()
2) Check the Global Status Registers of the DRHDs to verify VTd is enabled
3) Check RMRR memory ranges are set as reserved

Copyright (c) 2018, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include <Base.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <UnitTestTypes.h>
#include <Library/UnitTestLib.h>
#include <Library/UnitTestAssertLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/UnitTestLogLib.h>

#include <Protocol/PciIo.h>
#include <IndustryStandard/Pci22.h>

#include "DmaProtection.h"


#define UNIT_TEST_APP_NAME               L"DMA BME and Register Status Unit Test Library Application"
#define UNIT_TEST_APP_VERSION            L"0.1"
#define DMA_UNIT_TEST_VARIABLE_NAME      L"DMAUnitTestVariable"

extern EFI_GUID                          gDMAUnitTestVariableGuid;
EFI_HANDLE                               gImageHandle;

///================================================================================================
///================================================================================================
///
/// HELPER FUNCTIONS
///
///================================================================================================
///================================================================================================

struct BMEListNode
{
  EFI_PCI_IO_PROTOCOL           *PciIo;
  BOOLEAN                       BMEPreEBS;
  BOOLEAN                       BMEPostEBS;
  struct BMEListNode*              Next;
};

struct BME_TEST_CONTEXT
{
  UINT64            TestProgress;
};


///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================

UNIT_TEST_STATUS
EFIAPI
CheckRMRRRegions (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS                  Status;
  EFI_MEMORY_DESCRIPTOR       *EfiMemoryMap;
  EFI_MEMORY_DESCRIPTOR       *EfiMemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR       *EfiMemNext;
  UINTN                       EfiMemoryMapSize;
  UINTN                       EfiMapKey;
  UINTN                       EfiDescriptorSize;
  UINT32                      EfiDescriptorVersion;
  struct RMRRListNode*        Head;
  struct RMRRListNode*        Current;

  //
  // Step 1: Get DMAR Table
  //
  Status = GetDmarAcpiTable();
  UT_ASSERT_NOT_EFI_ERROR(Status);

  //
  // Step 2: Get the RMRR Headers from DMAR Table
  //
  Head = GetDmarAcpiTableRmrr();
  if (Head == NULL)
  {
    UT_LOG_INFO("No RMRRs Found\n");
    return UNIT_TEST_PASSED;
  }

  Current = Head;

  //
  // Step 3: Get the EFI memory map.
  //
  EfiMemoryMapSize  = 0;
  EfiMemoryMap      = NULL;
  Status = gBS->GetMemoryMap (
                  &EfiMemoryMapSize,
                  EfiMemoryMap,
                  &EfiMapKey,
                  &EfiDescriptorSize,
                  &EfiDescriptorVersion
                  );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    EfiMemoryMap = (EFI_MEMORY_DESCRIPTOR*)AllocateZeroPool(EfiMemoryMapSize + 8*EfiDescriptorSize);

    Status = gBS->GetMemoryMap (
                  &EfiMemoryMapSize,
                  EfiMemoryMap,
                  &EfiMapKey,
                  &EfiDescriptorSize,
                  &EfiDescriptorVersion
                  );
    UT_ASSERT_NOT_EFI_ERROR(Status);
  }
  else {
    UT_LOG_ERROR("GetMemoryMap Failed\n");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  //
  // Step 4: Step through memory map and verify each
  //         RMRR memory ranges are marked reserved
  //
  EfiMemNext = EfiMemoryMap;
  EfiMemoryMapEnd = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)EfiMemoryMap + EfiMemoryMapSize);

  while (EfiMemNext < EfiMemoryMapEnd) {
    //Check if memory range fully encompases RMRR
    if((EfiMemNext->PhysicalStart <= Current->RMRR->ReservedMemoryRegionBaseAddress)
    && (EfiMemNext->PhysicalStart + (EFI_PAGE_SIZE * EfiMemNext->NumberOfPages) >= Current->RMRR->ReservedMemoryRegionLimitAddress)) {
      //Verify memory range is marked as reserved
      UT_ASSERT_EQUAL(EfiMemNext->Type, EfiReservedMemoryType);
      UT_LOG_INFO("RMRRs between %X and %X found with type EfiReservedMemoryType\n", Current->RMRR->ReservedMemoryRegionBaseAddress, Current->RMRR->ReservedMemoryRegionLimitAddress);

      //Move on to next RMRR and start search at beginning of memory map again
      EfiMemNext = EfiMemoryMap;
      if(Current->Next == NULL)
      {
        return UNIT_TEST_PASSED;
      }
      Current = Current->Next;
    }
    else
    {
      //Move on to next descriptor
      EfiMemNext = NEXT_MEMORY_DESCRIPTOR( EfiMemNext, EfiDescriptorSize );

      //RMRR Not found in memory map
      UT_ASSERT_TRUE(EfiMemNext < EfiMemoryMapEnd);
    }
  } 
  
  //Should never reach here
  return UNIT_TEST_ERROR_TEST_FAILED;
}

UNIT_TEST_STATUS
EFIAPI
CheckBMETeardown (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
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
  struct BMEListNode*            Head = NULL;
  struct BMEListNode*            Tail = Head;
  UINT32                      Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE;
  VOID*                       Value = NULL;
  UINTN                       VarSize = 0;
  struct BME_TEST_CONTEXT     BMEContext = (*(struct BME_TEST_CONTEXT *)Context);

  if(BMEContext.TestProgress == 0)
  {
    BMEContext.TestProgress++;
    SaveFrameworkState(Framework, &BMEContext, sizeof(struct BME_TEST_CONTEXT));

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
          struct BMEListNode* temp = (struct BMEListNode*)AllocateZeroPool(sizeof(struct BMEListNode));
          UT_ASSERT_NOT_NULL(temp);

          temp->PciIo = PciIo;
          temp->BMEPreEBS = (BOOLEAN)(CommandReg & 0x4);
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
    //Step 4: Initialize the memory pool that will be saved to variables
    //
    struct BMEListNode*  Iterator = Head;
    UINTN Count = 0;
    while(Iterator != NULL)
    {
      Iterator = Iterator->Next;
      Count++;
    }
    BOOLEAN* BMEStatusArray = (BOOLEAN*)AllocateZeroPool(sizeof(BOOLEAN)*2*Count);
    UT_ASSERT_NOT_NULL(BMEStatusArray);
    
    //
    // Step 5: Get the EFI memory map.
    //
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
    // Step 6: Create exit boot services event
    //
    DEBUG((DEBUG_INFO, "Calling ExitBootServices\n"));
    Status = gBS->ExitBootServices (
                    gImageHandle,
                    EfiMapKey
                  );

    //
    // Step 7: Get Post EBS BME Status
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
      Iterator->BMEPostEBS = (BOOLEAN)(CommandReg & 0x4);
      Iterator = Iterator->Next;
      Count++;
    }
    
    //
    // Step 8: Flatten Linked List to write to variable
    //
    Iterator = Head;
    Count = 0;
    while(Iterator != NULL)
    {
      BMEStatusArray[Count*2]   = Iterator->BMEPreEBS;
      BMEStatusArray[Count*2+1] = Iterator->BMEPostEBS;
      Iterator = Iterator->Next;
      Count++;
    }

    //
    // Step 9: Since we are post exitbootservices we need to save the variable and reboot for further processing
    //
    gRT->SetVariable(
                  DMA_UNIT_TEST_VARIABLE_NAME,
                  &gDMAUnitTestVariableGuid,
                  Attributes,
                  sizeof(BOOLEAN)*2*Count,
                  BMEStatusArray
      );
    
    gRT->ResetSystem( EfiResetCold, EFI_SUCCESS, 0, NULL );
  }
  else
  {

    //
    // Step 1: Check if variable exists from previous run
    //
    Status = GetVariable3(
                  DMA_UNIT_TEST_VARIABLE_NAME,
                  &gDMAUnitTestVariableGuid,
                  &Value,
                  &VarSize,
                  NULL
                  );
    UT_ASSERT_NOT_EFI_ERROR(Status);

    //
    // Step 2: Verify BME was disabled during exitbootservies
    //
    UINTN i;
    BOOLEAN* Buffer = Value;
    for(i = 0; i < (VarSize/2)/(sizeof(BOOLEAN)); i++)
    {
      //BME Enabled before exit boot services
      UT_LOG_INFO(Buffer[2*i] ? "Pre-EBS BME %d: True\n" : "Pre-EBS BME %d: False\n", i);
      //BME Disabled after exit boot services
      UT_LOG_INFO(Buffer[2*i+1] ? "Post-EBS BME %d: True\n" : "Post-EBS BME %d: False\n", i);
      UT_ASSERT_FALSE(Buffer[2*i+1]);
    }

    //Free Buffer
    FreePool(Value);
    Buffer = NULL;

    //
    // Step 3: Delete Variable
    //
    Status = gRT->SetVariable( 
                    DMA_UNIT_TEST_VARIABLE_NAME,
                    &gDMAUnitTestVariableGuid,
                    0,
                    0,
                    NULL );
  }

  return UNIT_TEST_PASSED;
} // CheckBMETeardown()

UNIT_TEST_STATUS
EFIAPI
CheckDMAEnabled (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS                  Status;
  UINTN                       iterator;
  UINT32                      Reg32;

  //
  // Step 1: Get DMAR Table
  //
  Status = GetDmarAcpiTable();
  UT_ASSERT_NOT_EFI_ERROR(Status);

  //
  // Step 2: Find memory offset of DRHDs
  //
  Status = ParseDmarAcpiTableDrhd();
  UT_ASSERT_NOT_EFI_ERROR(Status);

  //
  // Step 3: Check Translation Enabled bit of each status register
  //
  for(iterator = 0; iterator < mVtdUnitNumber; iterator++)
  {
    Reg32 = MmioRead32 (mVtdUnitInformation[iterator].VtdUnitBaseAddress + R_GSTS_REG);
    UT_LOG_INFO("Global Status Register %X\n", Reg32);
    UINT32 DmaBit = (Reg32 & B_GSTS_REG_TE);
    UT_ASSERT_NOT_EQUAL(DmaBit, 0);
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
  @retval other           Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
DMAProtectionUnitTestApp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_SUITE             *VTdTests;
  UNIT_TEST_FRAMEWORK         *Fw = NULL;
  CHAR16                      ShortName[100];
  struct BME_TEST_CONTEXT     *BMEContext = AllocateZeroPool(sizeof(struct BME_TEST_CONTEXT));


  gImageHandle = ImageHandle;
  ShortName[0] = L'\0';

  //
  // Setup Unit Test Framework
  //
  UnicodeSPrint(&ShortName[0], sizeof(ShortName), L"%a", gEfiCallerBaseName); 
  DEBUG(( DEBUG_INFO, "%s v%s\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework( &Fw, UNIT_TEST_APP_NAME, ShortName, UNIT_TEST_APP_VERSION );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the Unit Test Suite.
  //
  Status = CreateUnitTestSuite( &VTdTests, Fw, L"VTd DMAR and Register tests", L"VTd", NULL, NULL );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for VTd\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase( VTdTests, L"All DRHD Units Have DMA Remapping Enabled", L"VTd.StatusRegister", CheckDMAEnabled, NULL, NULL, NULL );
  AddTestCase( VTdTests, L"BME Teardown at ExitBootServices", L"VTd.BMETeardown", CheckBMETeardown, NULL, NULL, BMEContext);
  AddTestCase( VTdTests, L"Verify RMRR ranges are marked reserved", L"VTd.RMRRRangeTest", CheckRMRRRegions, NULL, NULL, NULL);
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