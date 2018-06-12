/** @file -- HeapGuardTestSmm.c

Tests for page guard, pool guard, and null pointer detection in SMM.

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
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/SmmMemLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/SmmExceptionTestProtocol.h>

#include "../HeapGuardTestCommon.h"


//=============================================================================
// TEST HELPERS
//=============================================================================

/**

  Trigger reboot on interrupt instead of hang.

**/
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

VOID
PoolTest (
  IN UINT64* ptr,
  IN UINT64 AllocationSize
  )
{
  UINT64 *ptrLoc;
  DEBUG((DEBUG_ERROR, __FUNCTION__" Allocated pool at 0x%p\n", ptr));

  //
  // Check if guard page is going to be at the head or tail.
  //
  if ((PcdGet8(PcdHeapGuardPropertyMask) & BIT7) == 0) {
    //
    // Get to the beginning of the page the pool tail is on.
    //
    ptrLoc = (UINT64*) (((UINTN) ptr) + AllocationSize);
    ptrLoc = ALIGN_POINTER(ptrLoc, 0x1000);

    //
    // The guard page will be on the next page.
    //
    ptr = (UINT64*) (((UINTN) ptr) + 0x1000);
  }
  else {
    //
    // Get to the beginning of the page the pool head is on.
    //
    ptrLoc = ALIGN_POINTER(ptr, 0x1000);

    //
    // The guard page will be immidiately preceding the page the pool starts on.
    //
    ptrLoc = (UINT64*) (((UINTN) ptrLoc) - 0x1);
  }
  DEBUG((DEBUG_ERROR, __FUNCTION__" Writing to 0x%p\n", ptrLoc));
  *ptrLoc = 1;
  DEBUG((DEBUG_ERROR, __FUNCTION__" failure \n"));
} // PoolTest()


VOID
HeadPageTest (
  IN UINT64* ptr
  )
{
  DEBUG((DEBUG_ERROR, __FUNCTION__" Allocated page at 0x%p\n", ptr));

  // Hit the head guard page
  ptr = (UINT64*) (((UINTN) ptr) - 0x1);
  DEBUG((DEBUG_ERROR, __FUNCTION__" Writing to 0x%p\n", ptr));
  *ptr = 1;

  DEBUG((DEBUG_ERROR, __FUNCTION__" failure \n"));
}

VOID
TailPageTest (
  IN UINT64* ptr
  )
{
  DEBUG((DEBUG_ERROR, __FUNCTION__" Allocated page at 0x%p\n", ptr));

  // Hit the tail guard page
  ptr = (UINT64*) (((UINTN) ptr) + 0x1000);
  DEBUG((DEBUG_ERROR, __FUNCTION__" Writing to 0x%p\n", ptr));
  *ptr = 1;
  DEBUG((DEBUG_ERROR, __FUNCTION__" failure \n"));
} // HeadPageTest()



//=============================================================================
// TEST CASES
//=============================================================================

/**
  Page Guard
  Tests to make sure accessing the guard page at the head and the guard page
  at the tail result in a page fault.
**/
VOID
SmmPageGuard (
  IN HEAP_GUARD_TEST_CONTEXT           *Context
  )
{
  EFI_PHYSICAL_ADDRESS ptr;
  EFI_STATUS Status;
  UINT64 MemoryType;
  DEBUG((DEBUG_ERROR, __FUNCTION__"\n"));

  //
  // Memory type refers to the bitmask for the PcdHeapGuardPageType,
  // we need to RShift 1 to get it to reflect the correct EFI_MEMORY_TYPE.
  //
  MemoryType = Context->TargetMemoryType;
  MemoryType = RShiftU64(MemoryType, 1);
  Status = gBS->AllocatePages(AllocateAnyPages, (EFI_MEMORY_TYPE) MemoryType, 1, &ptr);

  //
  // Context.TestProgress indicates progress within this specific test.
  // 1 - Complete head guard test.
  // 2 - Complete tail guard test.
  //
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, __FUNCTION__" Memory allocation failed for %x- %r\n", MemoryType, Status));
  }
  else if (Context->TestProgress == 1)
  {
    HeadPageTest((UINT64 *) ptr);
    DEBUG((DEBUG_ERROR, "Head guard page failed."));
  }
  else {
    TailPageTest((UINT64 *) ptr);
    DEBUG((DEBUG_ERROR, "Tail guard page failed"));
  }
} // SmmPageGuard()

/**
  Pool Guard
  Tests to make sure accessing the guard page at the head/tail of the pool
  triggers a page fault.
**/
VOID
SmmPoolGuard (
  IN HEAP_GUARD_TEST_CONTEXT           *Context
  )
{
  UINT64 *ptr;
  EFI_STATUS Status;
  UINT64 AllocationSize;
  UINT64 MemoryType;
  DEBUG((DEBUG_ERROR, __FUNCTION__"\n"));

  //
  // Memory type refers to the bitmask for the PcdHeapGuardPageType,
  // we need to RShift 1 to get it to reflect the correct EFI_MEMORY_TYPE.
  //
  MemoryType = Context->TargetMemoryType;
  MemoryType = RShiftU64(MemoryType, 1);

  //
  // Context.TestProgress indicates progress within this specific test.
  // The test progressively allocates larger areas to test the gaurd on.
  // These areas are defined in Pool.c as the 13 different sized chunks that are available
  // for pool allocation.
  //
  AllocationSize = mPoolSizeTable[Context->TestProgress];
  Status = gBS->AllocatePool((EFI_MEMORY_TYPE) MemoryType, AllocationSize, &ptr);

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, __FUNCTION__" Memory allocation failed for %x- %r\n", MemoryType, Status));
  }
  else {
    PoolTest((UINT64 *) ptr, AllocationSize);
    DEBUG((DEBUG_ERROR, "Pool test failed."));
  }
} // SmmPoolGuard()


volatile HEAP_GUARD_TEST_CONTEXT       *mContext = NULL;

/**
  Null Pointer Detection
  Test checks to make sure reading and writing from a null pointer
  results in a page fault.
**/
VOID
SmmNullPointerDetection (
  IN HEAP_GUARD_TEST_CONTEXT           *Context
  )
{
  if (Context->TestProgress == 1) {
    if (mContext->TargetMemoryType == 0) {
    }
  }
  else {
    mContext->TargetMemoryType = 1;
  }
  DEBUG((DEBUG_ERROR,  __FUNCTION__" should have failed \n"));
} // SmmNullPointerDetection()

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
  HEAP_GUARD_TEST_COMM_BUFFER   *CommParams;

  DEBUG(( DEBUG_ERROR, __FUNCTION__"()\n" ));

  //
  // If input is invalid, stop processing this SMI
  //
  if (CommBuffer == NULL || CommBufferSize == NULL) {
    return EFI_SUCCESS;
  }

  TempCommBufferSize = *CommBufferSize;

  if(TempCommBufferSize != sizeof( HEAP_GUARD_TEST_COMM_BUFFER )) {
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
  CommParams = (HEAP_GUARD_TEST_COMM_BUFFER*)CommBuffer;
  CommParams->Status = EFI_SUCCESS;
  Status = EFI_SUCCESS;
  switch (CommParams->Function) {
    case HEAP_GUARD_TEST_POOL:
      DEBUG((DEBUG_ERROR, __FUNCTION__" - Function Requested - HEAP_GUARD_TEST_POOL\n"));
      SmmPageGuard(&CommParams->Context);
      break;

    case HEAP_GUARD_TEST_PAGE:
      DEBUG((DEBUG_ERROR, __FUNCTION__" - Function Requested - HEAP_GUARD_TEST_PAGE\n"));
      SmmPoolGuard(&CommParams->Context);
      break;

    case HEAP_GUARD_TEST_NULL_POINTER:
      DEBUG((DEBUG_ERROR, __FUNCTION__" - Function Requested - HEAP_GUARD_TEST_NULL_POINTER\n"));
      SmmNullPointerDetection(&CommParams->Context);
      break;

    default:
      DEBUG(( DEBUG_INFO, __FUNCTION__" - Unknown function - %d\n", CommParams->Function ));
      Status = EFI_UNSUPPORTED;
      break;
  }

  return Status;
} // MemoryProtectionTestHandler()

/**
  The module Entry Point of the driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
HeapGuardTestEntryPoint (
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
  Status = gSmst->SmiHandlerRegister( MemoryProtectionTestHandler, &gHeapGuardTestSmiHandlerGuid, &DiscardedHandle );
  ASSERT_EFI_ERROR( Status );

  return Status;
} // HeapGuardTestEntryPoint()
