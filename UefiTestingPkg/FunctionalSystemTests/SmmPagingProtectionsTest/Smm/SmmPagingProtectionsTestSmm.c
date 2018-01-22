/** @file -- SmmPagingProtectionsTestSmm.c
This is the SMM portion of the SmmPagingProtectionsTest driver.
This driver will be signalled by the DXE portion and will perform requested operations
to probe the extent of the SMM memory protections (like NX).

Copyright (c) 2017, Microsoft Corporation

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

// MS_CHANGE - Entire file created.

#include <PiSmm.h>

#include <Library/SmmServicesTableLib.h>
#include <Library/SmmMemLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

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
  static SMM_EXCEPTION_TEST_PROTOCOL    *SmmExceptionTestProtocol = NULL;

  // If we haven't found the protocol yet, do that now.
  if (SmmExceptionTestProtocol == NULL) {
    Status = gSmst->SmmLocateProtocol( &gSmmExceptionTestProtocolGuid, NULL, &SmmExceptionTestProtocol );
    if (EFI_ERROR( Status )) {
      DEBUG(( DEBUG_ERROR, __FUNCTION__" - Failed to locate SmmExceptionTestProtocol! %r\n", Status ));
      SmmExceptionTestProtocol = NULL;
    }

  }

  // If we have, request test mode.
  if (SmmExceptionTestProtocol != NULL) {
    Status = SmmExceptionTestProtocol->EnableTestMode();
    if (EFI_ERROR( Status )) {
      DEBUG(( DEBUG_ERROR, __FUNCTION__" - Failed to enable test mode!\n" ));
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
                                      should be considered a security viololation.

**/
EFI_STATUS
EFIAPI
SmmMemoryProtectionsSelfTestCode (
  VOID
  )
{
  UINTN   *CodeRegionToWriteTo;

  DEBUG(( DEBUG_VERBOSE, __FUNCTION__"()\n" ));

  // Make sure that the require telemetry/handling is
  // performed to get accurate test results.
  EnableExceptionTestMode();

  // Assign UINTN pointer to function address.
  // This will throw a warning with the Microsoft compiler, so we have to suppress it.
  // Behavior is unknown in other environments.
  #pragma warning(suppress:4054)
  CodeRegionToWriteTo = (VOID*)DummyFunctionForCodeSelfTest;

  // Attempt to write to function address.
  DEBUG(( DEBUG_INFO, __FUNCTION__" - Attempting to write to 0x%16lX...\n", CodeRegionToWriteTo ));
  *CodeRegionToWriteTo = 0xDEADBEEF;
  // This should fail.

  DEBUG(( DEBUG_ERROR, __FUNCTION__" - System proceeded through what should have been a critical failure!\n" ));

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
                                      should be considered a security viololation.

**/
EFI_STATUS
EFIAPI
SmmMemoryProtectionsSelfTestData (
  VOID
  )
{
  UINT8   *CodeRegionToCopyFrom;

  DEBUG(( DEBUG_VERBOSE, __FUNCTION__"()\n" ));

  // Make sure that the require telemetry/handling is
  // performed to get accurate test results.
  EnableExceptionTestMode();

  // Assign UINTN pointer to function address.
  // This will throw a warning with the Microsoft compiler, so we have to suppress it.
  // Behavior is unknown in other environments.
  #pragma warning(suppress:4054)
  CodeRegionToCopyFrom = (UINT8*)DummyFunctionForCodeSelfTest;

  // Copy the data that is necessary.
  CopyMem( &mDataExecutionTestBuffer[0], CodeRegionToCopyFrom, sizeof( mDataExecutionTestBuffer ) );

  // Now attempt to call the function. Like a sane person.
  // This will throw a warning with the Microsoft compiler, so we have to suppress it.
  // Behavior is unknown in other environments.
  DEBUG(( DEBUG_INFO, __FUNCTION__" - Attempting to execute from 0x%16lX...\n", &mDataExecutionTestBuffer[0] ));
  #pragma warning(suppress:4055)
  ((DUMMY_VOID_FUNCTION_FOR_DATA_TEST)&mDataExecutionTestBuffer[0])();

  DEBUG(( DEBUG_ERROR, __FUNCTION__" - System proceeded through what should have been a critical failure!\n" ));

  return EFI_SECURITY_VIOLATION;
} // SmmMemoryProtectionsSelfTestData()


/**
  Invalid Range Test
  This test will attempt to read from several regions outside of
  SMRAM and the declared CommBuffers. If any of these reads succeed,
  this test will fail.

  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, any sort of return value
                                      should be considered a security viololation.

**/
EFI_STATUS
EFIAPI
SmmMemoryProtectionsTestInvalidRange (
  VOID
  )
{
  UINTN                   ReadData;
  EFI_PHYSICAL_ADDRESS    ReadAddress;
  BOOLEAN                 ResultsValid = FALSE;

  DEBUG(( DEBUG_VERBOSE, __FUNCTION__"()\n" ));

  // Make sure that the require telemetry/handling is
  // performed to get accurate test results.
  EnableExceptionTestMode();

  // Attempt to read every 1MB for 100 MB.
  for (ReadAddress = 0x100000; ReadAddress <= (100 * 0x100000); ReadAddress += 0x100000) {
    // Make sure that these lie outside of valid SMRAM access locations.
    // If we don't find at least one address outside these locations, this test pass
    // is invalid.
    if (SmmIsBufferOutsideSmmValid( ReadAddress, sizeof( ReadData ) )) {
      continue;
    }

    // If we've found an invalid address, at least our test is legit.
    ResultsValid = TRUE;

    // Attempt to read from the address. This *should* fail.
    ReadData = *(UINTN*)ReadAddress;
  }

  // Note whether this was a valid test pass for debugging.
  if (!ResultsValid) {
    DEBUG(( DEBUG_ERROR, __FUNCTION__" - Could not find a single region outside of valid SMRAM ranges!\n" ));
    ASSERT( ResultsValid );
  }

  return EFI_SECURITY_VIOLATION;
} // SmmMemoryProtectionsTestInvalidRange()

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

  DEBUG(( DEBUG_VERBOSE, __FUNCTION__"()\n" ));

  //
  // If input is invalid, stop processing this SMI
  //
  if (CommBuffer == NULL || CommBufferSize == NULL) {
    return EFI_SUCCESS;
  }

  TempCommBufferSize = *CommBufferSize;

  if(TempCommBufferSize != sizeof( SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER )) {
    DEBUG(( DEBUG_ERROR, __FUNCTION__": SMM Communication buffer size is invalid for this handler!\n" ));
    return EFI_ACCESS_DENIED;
  }
  if (!SmmIsBufferOutsideSmmValid( (UINTN)CommBuffer, TempCommBufferSize )) {
    DEBUG(( DEBUG_ERROR, __FUNCTION__": SMM Communication buffer in invalid location!\n" ));
    return EFI_ACCESS_DENIED;
  }

  //
  // Farm out the job to individual functions based on what was requested.
  //
  CommParams = (SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER*)CommBuffer;
  Status = EFI_SUCCESS;
  switch (CommParams->Function) {
    case SMM_PAGING_PROTECTIONS_SELF_TEST_CODE:
      DEBUG(( DEBUG_VERBOSE, __FUNCTION__" - Function requested: SMM_PAGING_PROTECTIONS_SELF_TEST_CODE\n" ));
      Status = SmmMemoryProtectionsSelfTestCode();
      break;

    case SMM_PAGING_PROTECTIONS_SELF_TEST_DATA:
      DEBUG(( DEBUG_VERBOSE, __FUNCTION__" - Function requested: SMM_PAGING_PROTECTIONS_SELF_TEST_DATA\n" ));
      Status = SmmMemoryProtectionsSelfTestData();
      break;

    case SMM_PAGING_PROTECTIONS_TEST_INVALID_RANGE:
      DEBUG(( DEBUG_VERBOSE, __FUNCTION__" - Function requested: SMM_PAGING_PROTECTIONS_TEST_INVALID_RANGE\n" ));
      Status = SmmMemoryProtectionsTestInvalidRange();
      break;

    default:
      DEBUG(( DEBUG_INFO, __FUNCTION__" - Unknown function %d!\n", CommParams->Function ));
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
  Status = gSmst->SmiHandlerRegister( MemoryProtectionTestHandler, &gSmmPagingProtectionsTestSmiHandlerGuid, &DiscardedHandle );
  ASSERT_EFI_ERROR( Status );

  return Status;
}
