/** @file -- SmmPagingProtectionsTestSmm.c
This is the SMM portion of the SmmPagingProtectionsTest driver.
This driver will be signalled by the DXE portion and will perform requested operations
to probe the extent of the SMM memory protections (like NX).

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

// MS_CHANGE - Entire file created.

#include <PiSmm.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/SmmMemLib.h>

#include <Library/PlatformSmmProtectionsTestLib.h>

#include <Protocol/SmmExceptionTestProtocol.h>

#include "../SmmPagingProtectionsTestCommon.h"


//=============================================================================
// TEST HELPERS
//=============================================================================

VOID
EnableExceptionTestMode (
  VOID
  )
{
  EFI_STATUS    Status;
  UINTN         HandleBuffSize;
  EFI_HANDLE   *Handles;
  UINTN         Idx;
  SMM_EXCEPTION_TEST_PROTOCOL    *SmmExceptionTestProtocol;

  // Find all the SmmExceptionTestProtocol instances. There might be more than one if
  // different handlers are used based on what features are enabled.
  Handles = NULL;
  HandleBuffSize = 0;
  Status = gSmst->SmmLocateHandle (ByProtocol, &gSmmExceptionTestProtocolGuid, NULL, &HandleBuffSize, Handles);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to locate any instances of SmmExceptionTestProtocol: %r\n", __FUNCTION__, Status));
    return;
  }

  Handles = AllocatePool(HandleBuffSize);
  if (Handles == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to allocate space for instances of SmmExceptionTestProtocol.\n", __FUNCTION__));
    return;
  }

  Status = gSmst->SmmLocateHandle (ByProtocol, &gSmmExceptionTestProtocolGuid, NULL, &HandleBuffSize, Handles);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Error getting instances of SmmExceptionTestProtocol: %r\n", __FUNCTION__, Status));
    return;
  }

  //Iterate over all instacnes and call EnableTestMode() on each.
  for (Idx = 0; Idx < (HandleBuffSize/sizeof (EFI_HANDLE)); Idx++) {
    Status = gSmst->SmmHandleProtocol (Handles[Idx], &gSmmExceptionTestProtocolGuid, (VOID **)&SmmExceptionTestProtocol);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a] - Error getting instance %d of SmmExceptionTestProtocol: %r\n", __FUNCTION__, Idx, Status));
      continue;
    }
    Status = SmmExceptionTestProtocol->EnableTestMode ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a] - Failed to enable test mode for instance %d: %r\n", __FUNCTION__, Idx, Status));
    }
  }

  return;
} // EnableExceptionTestMode()

//=============================================================================
// TEST ASSETS
// These resources are used (and abused) by the test cases.
//=============================================================================

UINT8     mDataExecutionTestBuffer[512];

typedef
VOID
(*DUMMY_VOID_FUNCTION_FOR_DATA_TEST)(
  VOID
);

/**
  This is a function that serves as a placeholder in the driver code region.
  This function address will be written to by the SmmMemoryProtectionsSelfTestCode()
  test.

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
} // DummyFunctionForCodeSelfTest()

//=============================================================================
// TEST CASES
//=============================================================================

/**
  Self-Test Code
  This function will use the SmmPagingProtectionsTestSmm driver itself
  to determine whether paging protections are active for SMM driver images.
  This image should have at least one code page and one data page.
  Data pages should be marked NX.
  Code pages should be marked RO.

  To test the code page, we will attempt to write to a region of the driver that should hold code.

  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, and sort of return value
                                      should be considered a security violation.

**/
EFI_STATUS
EFIAPI
SmmMemoryProtectionsSelfTestCode (
  VOID
  )
{
  volatile UINTN   *CodeRegionToWriteTo;

  DEBUG ((DEBUG_VERBOSE, "%a()\n", __FUNCTION__));

  // Make sure that the required telemetry/handling is
  // performed to get accurate test results.
  EnableExceptionTestMode ();

  // Assign UINTN pointer to function address.
  // This will throw a warning with the Microsoft compiler, so we have to suppress it.
  // Behavior is unknown in other environments.
  #pragma warning(suppress:4054)
  CodeRegionToWriteTo = (VOID*)DummyFunctionForCodeSelfTest;

  // Attempt to write to function address.
  DEBUG ((DEBUG_INFO, "[%a] - Attempting to write to 0x%16lX...\n", __FUNCTION__, CodeRegionToWriteTo ));
  *CodeRegionToWriteTo = 0xDEADBEEF;
  // This should fail.

  DEBUG ((DEBUG_ERROR, "[%a] - System proceeded through what should have been a critical failure!\n", __FUNCTION__));

  return EFI_SECURITY_VIOLATION;
} // SmmMemoryProtectionsSelfTestCode()


/**
  Self-Test Data
  This function will use the SmmPagingProtectionsTestSmm driver itself
  to determine whether paging protections are active for SMM driver images.
  This image should have at least one code page and one data page.
  Data pages should be marked NX.
  Code pages should be marked RO.

  To test the data page, we will attempt to execute from a region of the driver that should hold data.

  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, and sort of return value
                                      should be considered a security violation.

**/
EFI_STATUS
EFIAPI
SmmMemoryProtectionsSelfTestData (
  VOID
  )
{
  UINT8   *CodeRegionToCopyFrom;

  DEBUG ((DEBUG_VERBOSE, "%a()\n",__FUNCTION__));

  // Make sure that the required telemetry/handling is
  // performed to get accurate test results.
  EnableExceptionTestMode ();

  // Assign UINTN pointer to function address.
  // This will throw a warning with the Microsoft compiler, so we have to suppress it.
  // Behavior is unknown in other environments.
  #pragma warning(suppress:4054)
  CodeRegionToCopyFrom = (UINT8*)DummyFunctionForCodeSelfTest;

  // Copy the data that is necessary.
  CopyMem (&mDataExecutionTestBuffer[0], CodeRegionToCopyFrom, sizeof(mDataExecutionTestBuffer));

  // Now attempt to call the function. Like a sane person.
  // This will throw a warning with the Microsoft compiler, so we have to suppress it.
  // Behavior is unknown in other environments.
  DEBUG ((DEBUG_INFO, "[%a] - Attempting to execute from 0x%16lX...\n", __FUNCTION__, &mDataExecutionTestBuffer[0]));
  #pragma warning(suppress:4055)
  ((DUMMY_VOID_FUNCTION_FOR_DATA_TEST)&mDataExecutionTestBuffer[0])();

  DEBUG ((DEBUG_ERROR, "[%a] - System proceeded through what should have been a critical failure!\n", __FUNCTION__));

  return EFI_SECURITY_VIOLATION;
} // SmmMemoryProtectionsSelfTestData()


/**
  Invalid Range Test
  This test will attempt to read from several regions outside of
  SMRAM and the declared CommBuffers. If any of these reads succeed,
  this test will fail.

  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, any sort of return value
                                      should be considered a security violation.

**/
EFI_STATUS
EFIAPI
SmmMemoryProtectionsTestInvalidRange (
  VOID
  )
{
  volatile UINTN          ReadData;
  EFI_PHYSICAL_ADDRESS    ReadAddress;
  BOOLEAN                 ResultsValid = FALSE;

  DEBUG ((DEBUG_VERBOSE, "%a()\n", __FUNCTION__));

  // Make sure that the required telemetry/handling is
  // performed to get accurate test results.
  EnableExceptionTestMode ();

  // Attempt to read every 1MB for 100 MB.
  for (ReadAddress = 0x100000; ReadAddress <= (100 * 0x100000); ReadAddress += 0x100000) {
    // Make sure that these lie outside of valid SMRAM access locations.
    // If we don't find at least one address outside these locations, this test pass
    // is invalid.
    if (SmmIsBufferOutsideSmmValid (ReadAddress, sizeof (ReadData))) {
      continue;
    }

    // If we've found an invalid address, at least our test is legit.
    ResultsValid = TRUE;

    // Attempt to read from the address. This *should* fail.
    ReadData = *(UINTN*)ReadAddress;
  }

  // Note whether this was a valid test pass for debugging.
  if (!ResultsValid) {
    DEBUG ((DEBUG_ERROR, "[%a] - Could not find a single region outside of valid SMRAM ranges!\n", __FUNCTION__));
    ASSERT (ResultsValid);
  }

  return EFI_SECURITY_VIOLATION;
} // SmmMemoryProtectionsTestInvalidRange()

/**
  Unauthorized I/O read test.
  This test will execute platform code to attempt an I/O read operation that should be prevented in SMM

  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, any sort of return value
                                      should be considered a security violation.

**/
EFI_STATUS
EFIAPI
SmmMemoryProtectionsTestUnathorizedIoRead (
  VOID
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_VERBOSE, "%a()\n", __FUNCTION__));

  // Make sure that the required telemetry/handling is
  // performed to get accurate test results.
  EnableExceptionTestMode ();

  DEBUG ((DEBUG_INFO, "[%a] - Attempting unauthorized I/O read.\n", __FUNCTION__));
  Status = TestUnauthorizedIoRead();
  if (Status == EFI_UNSUPPORTED) {
    return EFI_UNSUPPORTED;
  }
  DEBUG ((DEBUG_ERROR, "[%a] - System proceeded through what should have been a critical failure! Status = %r\n", __FUNCTION__, Status));

  return EFI_SECURITY_VIOLATION;
}

/**
  Unauthorized I/O write test.
  This test will execute platform code to attempt an I/O write operation that should be prevented in SMM

  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, any sort of return value
                                      should be considered a security violation.

**/
EFI_STATUS
EFIAPI
SmmMemoryProtectionsTestUnathorizedIoWrite (
  VOID
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_VERBOSE, "%a()\n", __FUNCTION__));

  // Make sure that the required telemetry/handling is
  // performed to get accurate test results.
  EnableExceptionTestMode ();

  DEBUG ((DEBUG_INFO, "[%a] - Attempting unauthorized I/O write.\n", __FUNCTION__));
  Status = TestUnauthorizedIoWrite();
  if (Status == EFI_UNSUPPORTED) {
    return EFI_UNSUPPORTED;
  }
  DEBUG ((DEBUG_ERROR, "[%a] - System proceeded through what should have been a critical failure! Status = %r\n", __FUNCTION__, Status));

  return EFI_SECURITY_VIOLATION;
}

/**
  Unauthorized MSR read test.
  This test will execute platform code to attempt an MSR read operation that should be prevented in SMM

  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, any sort of return value
                                      should be considered a security violation.

**/
EFI_STATUS
EFIAPI
SmmMemoryProtectionsTestUnathorizedMsrRead (
  VOID
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_VERBOSE, "%a()\n", __FUNCTION__));

  // Make sure that the required telemetry/handling is
  // performed to get accurate test results.
  EnableExceptionTestMode ();

  DEBUG ((DEBUG_INFO, "[%a] - Attempting unauthorized MSR read.\n", __FUNCTION__));
  Status = TestUnauthorizedMsrRead();
  if (Status == EFI_UNSUPPORTED) {
    return EFI_UNSUPPORTED;
  }
  DEBUG ((DEBUG_ERROR, "[%a] - System proceeded through what should have been a critical failure!\n", __FUNCTION__));

  return EFI_SECURITY_VIOLATION;
}

/**
  Unauthorized MSR write test.
  This test will execute platform code to attempt an MSR write operation that should be prevented in SMM

  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, any sort of return value
                                      should be considered a security violation.

**/
EFI_STATUS
EFIAPI
SmmMemoryProtectionsTestUnathorizedMsrWrite (
  VOID
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_VERBOSE, "%a()\n", __FUNCTION__));

  // Make sure that the required telemetry/handling is
  // performed to get accurate test results.
  EnableExceptionTestMode ();

  DEBUG ((DEBUG_INFO, "[%a] - Attempting unauthorized MSR write.\n", __FUNCTION__));
  Status = TestUnauthorizedMsrWrite();
  if (Status == EFI_UNSUPPORTED) {
    return EFI_UNSUPPORTED;
  }
  DEBUG ((DEBUG_ERROR, "[%a] - System proceeded through what should have been a critical failure!\n", __FUNCTION__));

  return EFI_SECURITY_VIOLATION;
}

/**
  Unauthorized privileged instruction test.
  This test will execute platform code to attempt a privileged instruction that should be prevented in SMM

  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, any sort of return value
                                      should be considered a security violation.

**/
EFI_STATUS
EFIAPI
SmmMemoryProtectionsTestPrivilegedInstructions (
  VOID
  )
{
  EFI_STATUS Status;
  DEBUG ((DEBUG_VERBOSE, "%a()\n", __FUNCTION__));

  // Make sure that the required telemetry/handling is
  // performed to get accurate test results.
  EnableExceptionTestMode ();

  DEBUG ((DEBUG_INFO, "[%a] - Attempting unauthorized privileged instruction.\n", __FUNCTION__));
  Status = TestPrivilegedInstruction();
  if (Status == EFI_UNSUPPORTED) {
    return EFI_UNSUPPORTED;
  }
  DEBUG ((DEBUG_ERROR, "[%a] - System proceeded through what should have been a critical failure!\n", __FUNCTION__));

  return EFI_SECURITY_VIOLATION;
}

/**
  Attempt to write SMM entry point.

  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, any sort of return value
                                      should be considered a security violation.

**/
EFI_STATUS
EFIAPI
SmmMemoryProtectionsTestEntryPointAccess (
  VOID
  )
{
  EFI_STATUS Status;
  DEBUG ((DEBUG_VERBOSE, "%a()\n", __FUNCTION__));

  // Make sure that the required telemetry/handling is
  // performed to get accurate test results.

  EnableExceptionTestMode ();
  DEBUG ((DEBUG_INFO, "[%a] - Attempting to access SMM EntryPoint\n", __FUNCTION__));
  Status = TestEntryPointAccess();
  if (Status == EFI_UNSUPPORTED) {
    return EFI_UNSUPPORTED;
  }
  DEBUG ((DEBUG_ERROR, "[%a] - System proceeded through what should have been a critical failure!\n", __FUNCTION__));

  return EFI_SECURITY_VIOLATION;
}

/**
  Self-Test Data
  This function will use the SmmPagingProtectionsTestSmm driver itself
  to determine whether paging protections are active for SMM driver images.
  This image should have at least one code page and one data page.
  Data pages should be marked NX.
  Code pages should be marked RO.

  To test the data page, we will attempt to execute from the specified code region outside of SMM.

  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, and sort of return value
                                      should be considered a security violation.

**/
EFI_STATUS
EFIAPI
SmmMemoryProtectionsRunArbitraryCode (
  EFI_PHYSICAL_ADDRESS TargetAddress
  )
{
  DEBUG ((DEBUG_VERBOSE, "%a()\n",__FUNCTION__));

  // Make sure that the required telemetry/handling is
  // performed to get accurate test results.
  EnableExceptionTestMode ();

  // Now attempt to call the function.
  DEBUG ((DEBUG_INFO, "[%a] - Attempting to execute from 0x%16lX...\n", __FUNCTION__, TargetAddress));
  ((DUMMY_VOID_FUNCTION_FOR_DATA_TEST)TargetAddress)();
  DEBUG ((DEBUG_ERROR, "[%a] - System proceeded through what should have been a critical failure!\n", __FUNCTION__));

  return EFI_SECURITY_VIOLATION;
}

/**
  Communication service SMI Handler entry.

  This handler takes requests to probe specific areas of memory and prove
  whether the SMM memory protections are covering the expected regions.

  Caution: This function may receive untrusted input.
  Communicate buffer and buffer size are external input, so this function will do basic validation.

  @param[in]      DispatchHandle    The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]      RegisterContext   Points to an optional handler context which was specified when the
                                    handler was registered.
  @param[in, out] CommBuffer        A pointer to a collection of data in memory that will
                                    be conveyed from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize    The size of the CommBuffer.

  @retval EFI_SUCCESS               The interrupt was handled and quiesced. No other handlers
                                    should still be called.
  @retval EFI_UNSUPPORTED           An unknown test function was requested.
  @retval EFI_ACCESS_DENIED         Part of the communication buffer lies in an invalid region.

**/
EFI_STATUS
EFIAPI
MemoryProtectionTestHandler (
  IN     EFI_HANDLE                   DispatchHandle,
  IN     CONST VOID                   *RegisterContext,
  IN OUT VOID                         *CommBuffer,
  IN OUT UINTN                        *CommBufferSize
  )
{
  EFI_STATUS                                Status;
  UINTN                                     TempCommBufferSize;
  SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER   *CommParams;

  DEBUG ((DEBUG_VERBOSE, "%a()\n", __FUNCTION__));

  //
  // If input is invalid, stop processing this SMI
  //
  if (CommBuffer == NULL || CommBufferSize == NULL) {
    return EFI_SUCCESS;
  }

  TempCommBufferSize = *CommBufferSize;

  if(TempCommBufferSize != sizeof (SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER)) {
    DEBUG ((DEBUG_ERROR, "[%a] SMM Communication buffer size is invalid for this handler!\n", __FUNCTION__));
    return EFI_ACCESS_DENIED;
  }
  if (!SmmIsBufferOutsideSmmValid ((UINTN)CommBuffer, TempCommBufferSize)) {
    DEBUG ((DEBUG_ERROR, "[%a] - SMM Communication buffer in invalid location!\n", __FUNCTION__));
    return EFI_ACCESS_DENIED;
  }

  //
  // Farm out the job to individual functions based on what was requested.
  //
  CommParams = (SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER*)CommBuffer;
  Status = EFI_SUCCESS;
  switch (CommParams->Function) {
    case SMM_PAGING_PROTECTIONS_SELF_TEST_CODE:
      DEBUG ((DEBUG_VERBOSE, "[%a] - Function requested: SMM_PAGING_PROTECTIONS_SELF_TEST_CODE\n", __FUNCTION__));
      Status = SmmMemoryProtectionsSelfTestCode();
      break;

    case SMM_PAGING_PROTECTIONS_SELF_TEST_DATA:
      DEBUG ((DEBUG_VERBOSE, "[%a] - Function requested: SMM_PAGING_PROTECTIONS_SELF_TEST_DATA\n", __FUNCTION__));
      Status = SmmMemoryProtectionsSelfTestData();
      break;

    case SMM_PAGING_PROTECTIONS_TEST_INVALID_RANGE:
      DEBUG ((DEBUG_VERBOSE, "[%a] - Function requested: SMM_PAGING_PROTECTIONS_TEST_INVALID_RANGE\n", __FUNCTION__));
      Status = SmmMemoryProtectionsTestInvalidRange();
      break;

    case SMM_PROTECTIONS_READ_UNAUTHORIZED_IO:
      DEBUG ((DEBUG_VERBOSE, "[%a] - Function requested: SMM_PROTECTIONS_READ_UNAUTHORIZED_IO\n", __FUNCTION__));
      Status = SmmMemoryProtectionsTestUnathorizedIoRead();
      break;

    case SMM_PROTECTIONS_WRITE_UNAUTHORIZED_IO:
      DEBUG ((DEBUG_VERBOSE, "[%a] - Function requested: SMM_PROTECTIONS_WRITE_UNAUTHORIZED_IO\n", __FUNCTION__));
      Status = SmmMemoryProtectionsTestUnathorizedIoWrite();
      break;

    case SMM_PROTECTIONS_READ_UNAUTHORIZED_MSR:
      DEBUG ((DEBUG_VERBOSE, "[%a] - Function requested: SMM_PROTECTIONS_READ_UNAUTHORIZED_MSR\n", __FUNCTION__));
      Status = SmmMemoryProtectionsTestUnathorizedMsrRead();
      break;

    case SMM_PROTECTIONS_WRITE_UNAUTHORIZED_MSR:
      DEBUG ((DEBUG_VERBOSE, "[%a] - Function requested: SMM_PROTECTIONS_WRITE_UNAUTHORIZED_MSR\n", __FUNCTION__));
      Status = SmmMemoryProtectionsTestUnathorizedMsrWrite();
      break;

    case SMM_PROTECTIONS_PRIVILEGED_INSTRUCTIONS:
      DEBUG ((DEBUG_VERBOSE, "[%a] - Function requested: SMM_PROTECTIONS_PRIVILEGED_INSTRUCTIONS\n", __FUNCTION__));
      Status = SmmMemoryProtectionsTestPrivilegedInstructions();
      break;

    case SMM_PROTECTIONS_ACCESS_ENTRY_POINT:
      DEBUG ((DEBUG_VERBOSE, "[%a] - Function requested: SMM_PROTECTIONS_ACCESS_ENTRY_POINT\n", __FUNCTION__));
      Status = SmmMemoryProtectionsTestEntryPointAccess();
      break;

    case SMM_PROTECTIONS_RUN_ARBITRARY_NON_SMM_CODE:
      DEBUG ((DEBUG_VERBOSE, "[%a] - Function requested: SMM_PROTECTIONS_RUN_ARBITRARY_NON_SMM_CODE\n", __FUNCTION__));
      Status = SmmMemoryProtectionsRunArbitraryCode(CommParams->TargetAddress);
      break;

    default:
      DEBUG ((DEBUG_INFO, "[%a] - Unknown function %d!\n", __FUNCTION__, CommParams->Function));
      Status = EFI_UNSUPPORTED;
      break;
  }

  CommParams->ReturnStatus = Status;
  return EFI_SUCCESS;
}

/**
  The module Entry Point of the driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
SmmPagingProtectionsTestEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                DiscardedHandle;

  //
  // Register SMI handler.
  //
  DiscardedHandle = NULL;
  Status = gSmst->SmiHandlerRegister (MemoryProtectionTestHandler, &gSmmPagingProtectionsTestSmiHandlerGuid, &DiscardedHandle);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
