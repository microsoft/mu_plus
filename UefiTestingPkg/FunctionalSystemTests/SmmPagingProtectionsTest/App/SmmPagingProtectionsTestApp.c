/** @file -- SmmPagingProtectionsTestApp.c
This user-facing application requests that the underlying SMM memory
protection test infrastructure exercise a particular test.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

// MS_CHANGE - Entire file created.

#include <Uefi.h>

#include <Protocol/SmmCommunication.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UnitTestLib.h>
#include <Library/UnitTestBootLib.h>

#include <Guid/PiSmmCommunicationRegionTable.h>

#include "../SmmPagingProtectionsTestCommon.h"


#define UNIT_TEST_APP_NAME        "SMM Memory Protections Test"
#define UNIT_TEST_APP_VERSION     "0.5"

VOID      *mPiSmmCommonCommBufferAddress = NULL;
UINTN     mPiSmmCommonCommBufferSize;


///================================================================================================
///================================================================================================
///
/// HELPER FUNCTIONS
///
///================================================================================================
///================================================================================================


/**
  This helper function actually sends the requested communication
  to the SMM driver.

  @param[in]  RequestedFunction   The test function to request the SMM driver run.

  @retval     EFI_SUCCESS         Communication was successful.
  @retval     EFI_ABORTED         Some error occurred.

**/
STATIC
EFI_STATUS
SmmMemoryProtectionsDxeToSmmCommunicate (
  IN  UINT16    RequestedFunction
  )
{
  EFI_STATUS                              Status = EFI_SUCCESS;
  EFI_SMM_COMMUNICATE_HEADER              *CommHeader;
  SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER *VerificationCommBuffer;
  static EFI_SMM_COMMUNICATION_PROTOCOL   *SmmCommunication = NULL;
  UINTN                                   CommBufferSize;

  if (mPiSmmCommonCommBufferAddress == NULL) {
    DEBUG(( DEBUG_ERROR, __FUNCTION__" - Communication buffer not found!\n" ));
    return EFI_ABORTED;
  }

  //
  // First, let's zero the comm buffer. Couldn't hurt.
  //
  CommHeader = (EFI_SMM_COMMUNICATE_HEADER*)mPiSmmCommonCommBufferAddress;
  CommBufferSize = sizeof( SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER ) + OFFSET_OF( EFI_SMM_COMMUNICATE_HEADER, Data );
  if (CommBufferSize > mPiSmmCommonCommBufferSize) {
    DEBUG(( DEBUG_ERROR, __FUNCTION__" - Communication buffer is too small!\n" ));
    return EFI_ABORTED;
  }
  ZeroMem( CommHeader, CommBufferSize );

  //
  // Update some parameters.
  //
  // SMM Communication Parameters
  CopyGuid( &CommHeader->HeaderGuid, &gSmmPagingProtectionsTestSmiHandlerGuid );
  CommHeader->MessageLength = sizeof( SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER );

  // Parameters Specific to this Implementation
  VerificationCommBuffer                = (SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER*)CommHeader->Data;
  VerificationCommBuffer->Function      = RequestedFunction;
  VerificationCommBuffer->ReturnStatus  = EFI_ABORTED;    // Default value.

  //
  // Locate the protocol, if not done yet.
  //
  if (!SmmCommunication) {
    Status = gBS->LocateProtocol( &gEfiSmmCommunicationProtocolGuid, NULL, (VOID**)&SmmCommunication );
  }

  //
  // Signal SMM.
  //
  if (!EFI_ERROR( Status )) {
    Status = SmmCommunication->Communicate( SmmCommunication,
                                            CommHeader,
                                            &CommBufferSize );
    DEBUG(( DEBUG_VERBOSE, __FUNCTION__" - Communicate() = %r\n", Status ));
  }

  return VerificationCommBuffer->ReturnStatus;
} // SmmMemoryProtectionsDxeToSmmCommunicate()

UNIT_TEST_STATUS
EFIAPI
LocateSmmCommonCommBuffer (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS                                Status;
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE   *PiSmmCommunicationRegionTable;
  EFI_MEMORY_DESCRIPTOR                     *SmmCommMemRegion;
  UINTN                                     Index, BufferSize;

  if (mPiSmmCommonCommBufferAddress == NULL) {
    Status = EfiGetSystemConfigurationTable( &gEdkiiPiSmmCommunicationRegionTableGuid, (VOID**)&PiSmmCommunicationRegionTable );
    UT_ASSERT_NOT_EFI_ERROR( Status );

    // We only need a region large enough to hold a SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER,
    // so this shouldn't be too hard.
    BufferSize = 0;
    SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR*)(PiSmmCommunicationRegionTable + 1);
    for (Index = 0; Index < PiSmmCommunicationRegionTable->NumberOfEntries; Index++) {
      if (SmmCommMemRegion->Type == EfiConventionalMemory) {
        BufferSize = EFI_PAGES_TO_SIZE( (UINTN)SmmCommMemRegion->NumberOfPages );
        if (BufferSize >= (sizeof( SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER ) + OFFSET_OF( EFI_SMM_COMMUNICATE_HEADER, Data ))) {
          break;
        }
      }
      SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)SmmCommMemRegion + PiSmmCommunicationRegionTable->DescriptorSize);
    }

    UT_ASSERT_TRUE( Index < PiSmmCommunicationRegionTable->NumberOfEntries );

    mPiSmmCommonCommBufferAddress = (VOID*)SmmCommMemRegion->PhysicalStart;
    mPiSmmCommonCommBufferSize = BufferSize;
  }

  return UNIT_TEST_PASSED;
} // LocateSmmCommonCommBuffer()


///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================


UNIT_TEST_STATUS
EFIAPI
CodeShouldBeWriteProtected (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN     PostReset = FALSE;

  // Check to see whether we're loading a context, potentially after a reboot.
  if (Context != NULL) {
    PostReset = *(BOOLEAN*)Context;
  }

  // If we're not post-reset, this should be the first time this test runs.
  if (!PostReset) {
    UT_ASSERT_NOT_NULL( mPiSmmCommonCommBufferAddress );

    //
    // Since we expect the "test" code to cause a fault which will
    // reset the system. Let's save a state that suggests the system
    // has already reset. This way, when we resume we will consider
    // it a "pass". If we fall through we will consider it a "fail".
    //
    PostReset = TRUE;
    SetBootNextDevice();
    SaveFrameworkState( &PostReset, sizeof( PostReset ) );

    // This should cause the system to reboot.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate( SMM_PAGING_PROTECTIONS_SELF_TEST_CODE );

    // If we're still here, things have gone wrong.
    UT_LOG_ERROR( "System was expected to reboot, but didn't." );
    PostReset = FALSE;
    SaveFrameworkState( &PostReset, sizeof( PostReset ) );
  }

  UT_ASSERT_TRUE( PostReset );

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
DataShouldBeExecuteProtected (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN   PostReset = FALSE;

  // Check to see whether we're loading a context, potentially after a reboot.
  if (Context != NULL) {
    PostReset = *(BOOLEAN*)Context;
  }

  // If we're not post-reset, this should be the first time this test runs.
  if (!PostReset) {
    UT_ASSERT_NOT_NULL( mPiSmmCommonCommBufferAddress );

    //
    // Since we expect the "test" code to cause a fault which will
    // reset the system. Let's save a state that suggests the system
    // has already reset. This way, when we resume we will consider
    // it a "pass". If we fall through we will consider it a "fail".
    //
    PostReset = TRUE;
    SetBootNextDevice();
    SaveFrameworkState( &PostReset, sizeof( PostReset ) );

    // This should cause the system to reboot.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate( SMM_PAGING_PROTECTIONS_SELF_TEST_DATA );

    // If we're still here, things have gone wrong.
    UT_LOG_ERROR( "System was expected to reboot, but didn't." );
    PostReset = FALSE;
    SaveFrameworkState( &PostReset, sizeof( PostReset ) );
  }

  UT_ASSERT_TRUE( PostReset );

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
InvalidRangesShouldBeReadProtected (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN   PostReset = FALSE;

  // Check to see whether we're loading a context, potentially after a reboot.
  if (Context != NULL) {
    PostReset = *(BOOLEAN*)Context;
  }

  // If we're not post-reset, this should be the first time this test runs.
  if (!PostReset) {
    UT_ASSERT_NOT_NULL( mPiSmmCommonCommBufferAddress );

    //
    // Since we expect the "test" code to cause a fault which will
    // reset the system. Let's save a state that suggests the system
    // has already reset. This way, when we resume we will consider
    // it a "pass". If we fall through we will consider it a "fail".
    //
    PostReset = TRUE;
    SetBootNextDevice();
    SaveFrameworkState( &PostReset, sizeof( PostReset ) );

    // This should cause the system to reboot.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate( SMM_PAGING_PROTECTIONS_TEST_INVALID_RANGE );

    // If we're still here, things have gone wrong.
    UT_LOG_ERROR( "System was expected to reboot, but didn't." );
    PostReset = FALSE;
    SaveFrameworkState( &PostReset, sizeof( PostReset ) );
  }

  UT_ASSERT_TRUE( PostReset );

  return UNIT_TEST_PASSED;
}

///================================================================================================
///================================================================================================
///
/// TEST ENGINE
///
///================================================================================================
///================================================================================================


/**
  SmmPagingProtectionsTestAppEntryPoint

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
SmmPagingProtectionsTestAppEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Fw = NULL;
  UNIT_TEST_SUITE_HANDLE      TestSuite;

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
  // Populate the TestSuite Unit Test Suite.
  //
  Status = CreateUnitTestSuite( &TestSuite, Fw, "SMM Paging Protections Tests", "Security.SMMPaging", NULL, NULL );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for TestSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase( TestSuite, "Code regions should be write-protected", "Security.SMMPaging.CodeProtections", CodeShouldBeWriteProtected, LocateSmmCommonCommBuffer, NULL, NULL );
  AddTestCase( TestSuite, "Data regions should be protected against execution", "Security.SMMPaging.DataProtections", DataShouldBeExecuteProtected, LocateSmmCommonCommBuffer, NULL, NULL );
  // THIS TEST DOESN'T WORK.
  // AddTestCase( TestSuite, "Invalid ranges should be protected against access from SMM", "Security.SMMPaging.InvalidRangeProtections", InvalidRangesShouldBeReadProtected, LocateSmmCommonCommBuffer, NULL, NULL );

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
} // SmmPagingProtectionsTestAppEntryPoint()
