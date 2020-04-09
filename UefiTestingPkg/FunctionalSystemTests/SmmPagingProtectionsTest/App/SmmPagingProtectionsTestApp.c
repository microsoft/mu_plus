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
  This helper function preps the shared CommBuffer for use by the test step.

  @param[out] CommBuffer   Returns a pointer to the CommBuffer for the test step to use.

  @retval     EFI_SUCCESS         CommBuffer initialized and ready to use.
  @retval     EFI_ABORTED         Some error occurred.

**/
STATIC
EFI_STATUS
SmmMemoryProtectionsGetCommBuffer (
  OUT  SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER **CommBuffer
  )
{
  EFI_SMM_COMMUNICATE_HEADER              *CommHeader;
  UINTN                                   CommBufferSize;

  if (mPiSmmCommonCommBufferAddress == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Communication buffer not found!\n", __FUNCTION__));
    return EFI_ABORTED;
  }

  // First, let's zero the comm buffer. Couldn't hurt.
  CommHeader = (EFI_SMM_COMMUNICATE_HEADER*)mPiSmmCommonCommBufferAddress;
  CommBufferSize = sizeof (SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER) + OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data);
  if (CommBufferSize > mPiSmmCommonCommBufferSize) {
    DEBUG ((DEBUG_ERROR, "[%a] - Communication buffer is too small!\n", __FUNCTION__));
    return EFI_ABORTED;
  }
  ZeroMem (CommHeader, CommBufferSize);

  // SMM Communication Parameters
  CopyGuid (&CommHeader->HeaderGuid, &gSmmPagingProtectionsTestSmiHandlerGuid);
  CommHeader->MessageLength = sizeof (SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER);

  // Return a pointer to the CommBuffer for the test to modify.
  *CommBuffer = (SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER*)CommHeader->Data;

  return EFI_SUCCESS;
}

/**
  This helper function actually sends the requested communication
  to the SMM driver.

  @retval     EFI_SUCCESS         Communication was successful.
  @retval     EFI_ABORTED         Some error occurred.

**/
STATIC
EFI_STATUS
SmmMemoryProtectionsDxeToSmmCommunicate (
  VOID
  )
{
  EFI_STATUS                              Status = EFI_SUCCESS;
  EFI_SMM_COMMUNICATE_HEADER              *CommHeader;
  UINTN                                   CommBufferSize;
  static EFI_SMM_COMMUNICATION_PROTOCOL   *SmmCommunication = NULL;

  if (mPiSmmCommonCommBufferAddress == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Communication buffer not found!\n" , __FUNCTION__));
    return EFI_ABORTED;
  }

  // Grab the CommBuffer
  CommHeader = (EFI_SMM_COMMUNICATE_HEADER*)mPiSmmCommonCommBufferAddress;
  CommBufferSize = sizeof (SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER) + OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data);

  // Locate the protocol, if not done yet.
  if (!SmmCommunication) {
    Status = gBS->LocateProtocol (&gEfiSmmCommunicationProtocolGuid, NULL, (VOID**)&SmmCommunication);
  }

  // Signal SMM.
  if (!EFI_ERROR (Status)) {
    Status = SmmCommunication->Communicate (SmmCommunication, CommHeader, &CommBufferSize);
    DEBUG ((DEBUG_VERBOSE, "[%a] - Communicate() = %r\n", __FUNCTION__, Status));
  }

  return ((SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER *)CommHeader->Data)->ReturnStatus;
}

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
    Status = EfiGetSystemConfigurationTable (&gEdkiiPiSmmCommunicationRegionTableGuid, (VOID**)&PiSmmCommunicationRegionTable);
    UT_ASSERT_NOT_EFI_ERROR (Status);

    // We only need a region large enough to hold a SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER,
    // so this shouldn't be too hard.
    BufferSize = 0;
    SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR*)(PiSmmCommunicationRegionTable + 1);
    for (Index = 0; Index < PiSmmCommunicationRegionTable->NumberOfEntries; Index++) {
      if (SmmCommMemRegion->Type == EfiConventionalMemory) {
        BufferSize = EFI_PAGES_TO_SIZE ((UINTN)SmmCommMemRegion->NumberOfPages);
        if (BufferSize >= (sizeof (SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER) + OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data))) {
          break;
        }
      }
      SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)SmmCommMemRegion + PiSmmCommunicationRegionTable->DescriptorSize);
    }

    UT_ASSERT_TRUE (Index < PiSmmCommunicationRegionTable->NumberOfEntries);

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
  SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER *CommBuffer;

  // Check to see whether we're loading a context, potentially after a reboot.
  if (Context != NULL) {
    PostReset = *(BOOLEAN*)Context;
  }

  // If we're not post-reset, this should be the first time this test runs.
  if (!PostReset) {
    UT_ASSERT_NOT_NULL (mPiSmmCommonCommBufferAddress);

    //
    // Since we expect the "test" code to cause a fault which will
    // reset the system. Let's save a state that suggests the system
    // has already reset. This way, when we resume we will consider
    // it a "pass". If we fall through we will consider it a "fail".
    //
    PostReset = TRUE;
    SetBootNextDevice();
    SaveFrameworkState( &PostReset, sizeof( PostReset ) );

    // Grab the CommBuffer and fill it in for this test
    Status = SmmMemoryProtectionsGetCommBuffer (&CommBuffer);
    UT_ASSERT_NOT_EFI_ERROR (Status);

    CommBuffer->Function = SMM_PAGING_PROTECTIONS_SELF_TEST_CODE;

    // This should cause the system to reboot.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate ();

    // If we're still here, things have gone wrong.
    UT_LOG_ERROR( "System was expected to reboot, but didn't." );
    PostReset = FALSE;
    SaveFrameworkState( &PostReset, sizeof( PostReset ) );
  }

  UT_ASSERT_TRUE (PostReset);

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
  SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER *CommBuffer;

  // Check to see whether we're loading a context, potentially after a reboot.
  if (Context != NULL) {
    PostReset = *(BOOLEAN*)Context;
  }

  // If we're not post-reset, this should be the first time this test runs.
  if (!PostReset) {
    UT_ASSERT_NOT_NULL (mPiSmmCommonCommBufferAddress);

    //
    // Since we expect the "test" code to cause a fault which will
    // reset the system. Let's save a state that suggests the system
    // has already reset. This way, when we resume we will consider
    // it a "pass". If we fall through we will consider it a "fail".
    //
    PostReset = TRUE;
    SetBootNextDevice();
    SaveFrameworkState( &PostReset, sizeof( PostReset ) );

    // Grab the CommBuffer and fill it in for this test
    Status = SmmMemoryProtectionsGetCommBuffer (&CommBuffer);
    UT_ASSERT_NOT_EFI_ERROR (Status);

    CommBuffer->Function = SMM_PAGING_PROTECTIONS_SELF_TEST_DATA;

    // This should cause the system to reboot.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate ();

    // If we're still here, things have gone wrong.
    UT_LOG_ERROR ("System was expected to reboot, but didn't.");
    PostReset = FALSE;
    SaveFrameworkState( &PostReset, sizeof( PostReset ) );
  }

  UT_ASSERT_TRUE (PostReset);

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
  SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER *CommBuffer;

  // Check to see whether we're loading a context, potentially after a reboot.
  if (Context != NULL) {
    PostReset = *(BOOLEAN*)Context;
  }

  // If we're not post-reset, this should be the first time this test runs.
  if (!PostReset) {
    UT_ASSERT_NOT_NULL (mPiSmmCommonCommBufferAddress);

    //
    // Since we expect the "test" code to cause a fault which will
    // reset the system. Let's save a state that suggests the system
    // has already reset. This way, when we resume we will consider
    // it a "pass". If we fall through we will consider it a "fail".
    //
    PostReset = TRUE;
    SetBootNextDevice();
    SaveFrameworkState( &PostReset, sizeof( PostReset ) );

    // Grab the CommBuffer and fill it in for this test
    Status = SmmMemoryProtectionsGetCommBuffer (&CommBuffer);
    UT_ASSERT_NOT_EFI_ERROR (Status);

    CommBuffer->Function = SMM_PAGING_PROTECTIONS_TEST_INVALID_RANGE;

    // This should cause the system to reboot.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate ();

    // If we're still here, things have gone wrong.
    UT_LOG_ERROR ("System was expected to reboot, but didn't.");
    PostReset = FALSE;
    SaveFrameworkState (&PostReset, sizeof (PostReset));
  }

  UT_ASSERT_TRUE (PostReset);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
UnauthorizedIoShouldBeReadProtected (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN   PostReset = FALSE;
  SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER *CommBuffer;

  // Check to see whether we're loading a context, potentially after a reboot.
  if (Context != NULL) {
    PostReset = *(BOOLEAN*)Context;
  }

  // If we're not post-reset, this should be the first time this test runs.
  if (!PostReset) {
    UT_ASSERT_NOT_NULL (mPiSmmCommonCommBufferAddress);

    //
    // Since we expect the "test" code to cause a fault which will
    // reset the system. Let's save a state that suggests the system
    // has already reset. This way, when we resume we will consider
    // it a "pass". If we fall through we will consider it a "fail".
    //
    PostReset = TRUE;
    SetBootNextDevice ();
    SaveFrameworkState ( &PostReset, sizeof (PostReset));

    // Grab the CommBuffer and fill it in for this test
    Status = SmmMemoryProtectionsGetCommBuffer (&CommBuffer);
    UT_ASSERT_NOT_EFI_ERROR (Status);

    CommBuffer->Function = SMM_PROTECTIONS_READ_UNAUTHORIZED_IO;

    // This should cause the system to reboot.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate ();
    if (Status == EFI_UNSUPPORTED) {
      return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
    }

    // If we're still here, things have gone wrong.
    UT_LOG_ERROR ("System was expected to reboot, but didn't.");
    PostReset = FALSE;
    SaveFrameworkState ( &PostReset, sizeof (PostReset));
  }

  UT_ASSERT_TRUE (PostReset);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
UnauthorizedIoShouldBeWriteProtected (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN   PostReset = FALSE;
  SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER *CommBuffer;

  // Check to see whether we're loading a context, potentially after a reboot.
  if (Context != NULL) {
    PostReset = *(BOOLEAN*)Context;
  }

  // If we're not post-reset, this should be the first time this test runs.
  if (!PostReset) {
    UT_ASSERT_NOT_NULL (mPiSmmCommonCommBufferAddress);

    //
    // Since we expect the "test" code to cause a fault which will
    // reset the system. Let's save a state that suggests the system
    // has already reset. This way, when we resume we will consider
    // it a "pass". If we fall through we will consider it a "fail".
    //
    PostReset = TRUE;
    SetBootNextDevice ();
    SaveFrameworkState (&PostReset, sizeof (PostReset));

    // Grab the CommBuffer and fill it in for this test
    Status = SmmMemoryProtectionsGetCommBuffer (&CommBuffer);
    UT_ASSERT_NOT_EFI_ERROR (Status);

    CommBuffer->Function = SMM_PROTECTIONS_WRITE_UNAUTHORIZED_IO;

    // This should cause the system to reboot.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate ();
    if (Status == EFI_UNSUPPORTED) {
      return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
    }

    // If we're still here, things have gone wrong.
    UT_LOG_ERROR ("System was expected to reboot, but didn't.");
    PostReset = FALSE;
    SaveFrameworkState (&PostReset, sizeof( PostReset ));
  }

  UT_ASSERT_TRUE (PostReset);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
UnauthorizedMsrShouldBeReadProtected (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN   PostReset = FALSE;
  SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER *CommBuffer;

  // Check to see whether we're loading a context, potentially after a reboot.
  if (Context != NULL) {
    PostReset = *(BOOLEAN*)Context;
  }

  // If we're not post-reset, this should be the first time this test runs.
  if (!PostReset) {
    UT_ASSERT_NOT_NULL (mPiSmmCommonCommBufferAddress);

    //
    // Since we expect the "test" code to cause a fault which will
    // reset the system. Let's save a state that suggests the system
    // has already reset. This way, when we resume we will consider
    // it a "pass". If we fall through we will consider it a "fail".
    //
    PostReset = TRUE;
    SetBootNextDevice ();
    SaveFrameworkState (&PostReset, sizeof (PostReset));

    // Grab the CommBuffer and fill it in for this test
    Status = SmmMemoryProtectionsGetCommBuffer (&CommBuffer);
    UT_ASSERT_NOT_EFI_ERROR (Status);

    CommBuffer->Function = SMM_PROTECTIONS_READ_UNAUTHORIZED_MSR;

    // This should cause the system to reboot.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate ();
    if (Status == EFI_UNSUPPORTED) {
      return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
    }

    // If we're still here, things have gone wrong.
    UT_LOG_ERROR ("System was expected to reboot, but didn't.");
    PostReset = FALSE;
    SaveFrameworkState (&PostReset, sizeof (PostReset));
  }

  UT_ASSERT_TRUE (PostReset);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
UnauthorizedMsrShouldBeWriteProtected (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN   PostReset = FALSE;
  SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER *CommBuffer;

  // Check to see whether we're loading a context, potentially after a reboot.
  if (Context != NULL) {
    PostReset = *(BOOLEAN*)Context;
  }

  // If we're not post-reset, this should be the first time this test runs.
  if (!PostReset) {
    UT_ASSERT_NOT_NULL (mPiSmmCommonCommBufferAddress);

    //
    // Since we expect the "test" code to cause a fault which will
    // reset the system. Let's save a state that suggests the system
    // has already reset. This way, when we resume we will consider
    // it a "pass". If we fall through we will consider it a "fail".
    //
    PostReset = TRUE;
    SetBootNextDevice ();
    SaveFrameworkState (&PostReset, sizeof (PostReset));

    // Grab the CommBuffer and fill it in for this test
    Status = SmmMemoryProtectionsGetCommBuffer (&CommBuffer);
    UT_ASSERT_NOT_EFI_ERROR (Status);

    CommBuffer->Function = SMM_PROTECTIONS_WRITE_UNAUTHORIZED_MSR;

    // This should cause the system to reboot.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate ();
    if (Status == EFI_UNSUPPORTED) {
      return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
    }

    // If we're still here, things have gone wrong.
    UT_LOG_ERROR ("System was expected to reboot, but didn't.");
    PostReset = FALSE;
    SaveFrameworkState (&PostReset, sizeof (PostReset));
  }

  UT_ASSERT_TRUE (PostReset);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
PrivilegedInstructionsShouldBePrevented (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN   PostReset = FALSE;
  SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER *CommBuffer;

  // Check to see whether we're loading a context, potentially after a reboot.
  if (Context != NULL) {
    PostReset = *(BOOLEAN*)Context;
  }

  // If we're not post-reset, this should be the first time this test runs.
  if (!PostReset) {
    UT_ASSERT_NOT_NULL (mPiSmmCommonCommBufferAddress);

    //
    // Since we expect the "test" code to cause a fault which will
    // reset the system. Let's save a state that suggests the system
    // has already reset. This way, when we resume we will consider
    // it a "pass". If we fall through we will consider it a "fail".
    //
    PostReset = TRUE;
    SetBootNextDevice ();
    SaveFrameworkState (&PostReset, sizeof (PostReset));

    // Grab the CommBuffer and fill it in for this test
    Status = SmmMemoryProtectionsGetCommBuffer (&CommBuffer);
    UT_ASSERT_NOT_EFI_ERROR (Status);

    CommBuffer->Function = SMM_PROTECTIONS_PRIVILEGED_INSTRUCTIONS;

    // This should cause the system to reboot.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate ();
    if (Status == EFI_UNSUPPORTED) {
      return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
    }

    // If we're still here, things have gone wrong.
    UT_LOG_ERROR ("System was expected to reboot, but didn't.");
    PostReset = FALSE;
    SaveFrameworkState ( &PostReset, sizeof (PostReset));
  }

  UT_ASSERT_TRUE (PostReset);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
AccessToSmmEntryPointShouldBePrevented (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN   PostReset = FALSE;
  SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER *CommBuffer;

  // Check to see whether we're loading a context, potentially after a reboot.
  if (Context != NULL) {
    PostReset = *(BOOLEAN*)Context;
  }

  // If we're not post-reset, this should be the first time this test runs.
  if (!PostReset) {
    UT_ASSERT_NOT_NULL (mPiSmmCommonCommBufferAddress);

    //
    // Since we expect the "test" code to cause a fault which will
    // reset the system. Let's save a state that suggests the system
    // has already reset. This way, when we resume we will consider
    // it a "pass". If we fall through we will consider it a "fail".
    //
    PostReset = TRUE;
    SetBootNextDevice ();
    SaveFrameworkState ( &PostReset, sizeof (PostReset));

    // Grab the CommBuffer and fill it in for this test
    Status = SmmMemoryProtectionsGetCommBuffer (&CommBuffer);
    UT_ASSERT_NOT_EFI_ERROR (Status);

    CommBuffer->Function = SMM_PROTECTIONS_ACCESS_ENTRY_POINT;

    // This should cause the system to reboot.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate ();

    // If we're still here, things have gone wrong.
    UT_LOG_ERROR ("System was expected to reboot, but didn't.");
    PostReset = FALSE;
    SaveFrameworkState ( &PostReset, sizeof (PostReset));
  }

  UT_ASSERT_TRUE (PostReset);

  return UNIT_TEST_PASSED;
}

typedef
VOID
(*DUMMY_VOID_FUNCTION_FOR_DATA_TEST)(
  VOID
);

/**
  This is a function that serves as a placeholder in non-SMM code region.
**/
STATIC
VOID
DummyFunctionForCodeSelfTest (
  VOID
  )
{
  volatile UINT8    DontCompileMeOut = 0;
  DontCompileMeOut++;
  return;
}

UNIT_TEST_STATUS
EFIAPI
CodeOutSideSmmShouldNotRun (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN   PostReset = FALSE;
  SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER *CommBuffer;

  // Check to see whether we're loading a context, potentially after a reboot.
  if (Context != NULL) {
    PostReset = *(BOOLEAN*)Context;
  }

  // If we're not post-reset, this should be the first time this test runs.
  if (!PostReset) {
    UT_ASSERT_NOT_NULL (mPiSmmCommonCommBufferAddress);

    //
    // Since we expect the "test" code to cause a fault which will
    // reset the system. Let's save a state that suggests the system
    // has already reset. This way, when we resume we will consider
    // it a "pass". If we fall through we will consider it a "fail".
    //
    PostReset = TRUE;
    SetBootNextDevice ();
    SaveFrameworkState ( &PostReset, sizeof (PostReset));

    // Grab the CommBuffer and fill it in for this test
    Status = SmmMemoryProtectionsGetCommBuffer (&CommBuffer);
    UT_ASSERT_NOT_EFI_ERROR (Status);

    CommBuffer->Function = SMM_PROTECTIONS_RUN_ARBITRARY_NON_SMM_CODE;
    CommBuffer->TargetAddress = (EFI_PHYSICAL_ADDRESS)&DummyFunctionForCodeSelfTest;

    // This should cause the system to reboot.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate ();

    // If we're still here, things have gone wrong.
    UT_LOG_ERROR ("System was expected to reboot, but didn't.");
    PostReset = FALSE;
    SaveFrameworkState ( &PostReset, sizeof (PostReset));
  }

  UT_ASSERT_TRUE (PostReset);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
CodeInCommBufferShouldNotRun (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS  Status;
  BOOLEAN   PostReset = FALSE;
  SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER *CommBuffer;

  // Check to see whether we're loading a context, potentially after a reboot.
  if (Context != NULL) {
    PostReset = *(BOOLEAN*)Context;
  }

  // If we're not post-reset, this should be the first time this test runs.
  if (!PostReset) {
    UT_ASSERT_NOT_NULL (mPiSmmCommonCommBufferAddress);

    //
    // Since we expect the "test" code to cause a fault which will
    // reset the system. Let's save a state that suggests the system
    // has already reset. This way, when we resume we will consider
    // it a "pass". If we fall through we will consider it a "fail".
    //
    PostReset = TRUE;
    SetBootNextDevice ();
    SaveFrameworkState ( &PostReset, sizeof (PostReset));

    // Grab the CommBuffer and fill it in for this test
    Status = SmmMemoryProtectionsGetCommBuffer (&CommBuffer);
    UT_ASSERT_NOT_EFI_ERROR (Status);

    CommBuffer->Function = SMM_PROTECTIONS_RUN_ARBITRARY_NON_SMM_CODE;
    CommBuffer->TargetValue = 0xC3; //ret instruction.
    CommBuffer->TargetAddress = (EFI_PHYSICAL_ADDRESS)&CommBuffer->TargetValue;

    // This should cause the system to reboot.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate ();

    // If we're still here, things have gone wrong.
    UT_LOG_ERROR ("System was expected to reboot, but didn't.");
    PostReset = FALSE;
    SaveFrameworkState( &PostReset, sizeof( PostReset ) );
  }

  UT_ASSERT_TRUE (PostReset);

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
  UNIT_TEST_SUITE_HANDLE      PagingSuite;
  UNIT_TEST_SUITE_HANDLE      ProtectionsSuite;

  DEBUG(( DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework( &Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto Cleanup;
  }

  //
  // Populate the TestSuite Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&PagingSuite, Fw, "SMM Paging Protections Tests", "Security.SMMPaging", NULL, NULL);
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for PagingSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }
  AddTestCase( PagingSuite, "Code regions should be write-protected", "Security.SMMPaging.CodeProtections", CodeShouldBeWriteProtected, LocateSmmCommonCommBuffer, NULL, NULL );
  AddTestCase( PagingSuite, "Data regions should be protected against execution", "Security.SMMPaging.DataProtections", DataShouldBeExecuteProtected, LocateSmmCommonCommBuffer, NULL, NULL );
  AddTestCase (PagingSuite, "Invalid ranges should be protected against access from SMM", "Security.SMMPaging.InvalidRangeProtections", InvalidRangesShouldBeReadProtected, LocateSmmCommonCommBuffer, NULL, NULL);
  AddTestCase (PagingSuite, "Execution of code outside of SMM should be prevented", "Security.SMMPaging.CodeOutSideSmmShouldNotRun", CodeOutSideSmmShouldNotRun, LocateSmmCommonCommBuffer, NULL, NULL);
  AddTestCase (PagingSuite, "Execution of code in SMM Comm Buffer should be prevented", "Security.SMMPaging.CodeInCommBufferShouldNotRun", CodeInCommBufferShouldNotRun, LocateSmmCommonCommBuffer, NULL, NULL);
  AddTestCase (PagingSuite, "Write Access to SMM Entry Point should be prevented", "Security.SMMPaging.EntryPointShouldNotBeAccessible", AccessToSmmEntryPointShouldBePrevented, LocateSmmCommonCommBuffer, NULL, NULL);

  //
  // Populate the ProtectionsSuite with general SMM Protection Unit tests
  //
  Status = CreateUnitTestSuite (&ProtectionsSuite, Fw, "SMM Protections Tests", "Security.SMMProtections", NULL, NULL);
  if (EFI_ERROR (Status)) 
  {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for ProtectionsSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  AddTestCase (ProtectionsSuite, "Reads to unauthorized I/O ports should be prevented", "Security.SMMProtections.IoReadProtections", UnauthorizedIoShouldBeReadProtected, LocateSmmCommonCommBuffer, NULL, NULL);
  AddTestCase (ProtectionsSuite, "Writes to unauthorized I/O ports should be prevented", "Security.SMMProtections.IoWriteProtections", UnauthorizedIoShouldBeWriteProtected, LocateSmmCommonCommBuffer, NULL, NULL);
  AddTestCase (ProtectionsSuite, "Reads to unauthorized MSRs should be prevented", "Security.SMMProtections.MsrReadProtections", UnauthorizedMsrShouldBeReadProtected, LocateSmmCommonCommBuffer, NULL, NULL);
  AddTestCase (ProtectionsSuite, "Writes to unauthorized MSRs should be prevented", "Security.SMMProtections.MsrWriteProtections", UnauthorizedMsrShouldBeWriteProtected, LocateSmmCommonCommBuffer, NULL, NULL);
  AddTestCase (ProtectionsSuite, "Execution of privileged instructions in SMM should be prevented", "Security.SMMProtections.PrivilegedInstructionProtections", PrivilegedInstructionsShouldBePrevented, LocateSmmCommonCommBuffer, NULL, NULL);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Fw);

Cleanup:
  if (Fw) {
    FreeUnitTestFramework (Fw);
  }

  return Status;
}
