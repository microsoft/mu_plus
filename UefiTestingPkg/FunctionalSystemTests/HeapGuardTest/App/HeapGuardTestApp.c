/** @file -- HeapGuardTestApp.c

Tests for page guard, pool guard, NX protections, stack guard, and null pointer detection.

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

#include <Uefi.h>

#include <Protocol/Cpu.h>
#include <Protocol/SmmCommunication.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <UnitTestTypes.h>
#include <Library/UnitTestLib.h>
#include <Library/UnitTestLogLib.h>
#include <Library/UnitTestAssertLib.h>
#include <Library/UnitTestBootUsbLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Register\ArchitecturalMsr.h>

#include <Guid/PiSmmCommunicationRegionTable.h>

#include "../HeapGuardTestCommon.h"
#include "UefiHardwareNxProtectionStub.h"


#define UNIT_TEST_APP_NAME        L"Heap Guard Test"
#define UNIT_TEST_APP_VERSION     L"0.5"

VOID                            *mPiSmmCommonCommBufferAddress = NULL;
UINTN                            mPiSmmCommonCommBufferSize;
EFI_CPU_ARCH_PROTOCOL           *mCpu = NULL;

///================================================================================================
///================================================================================================
///
/// HELPER FUNCTIONS
///
///================================================================================================
///================================================================================================

/**
  Resets the system on interrupt

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor.This parameter is processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.
**/
VOID
EFIAPI
InterruptHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  DEBUG((DEBUG_ERROR, __FUNCTION__" SystemContextX64->ExceptionData: %x - InterruptType: %x\n", SystemContext.SystemContextX64->ExceptionData, InterruptType));
  gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
} // InterruptHandler()


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
  IN  UINT16    RequestedFunction,
  IN  HEAP_GUARD_TEST_CONTEXT*    Context
  )
{
  EFI_STATUS                              Status = EFI_SUCCESS;
  EFI_SMM_COMMUNICATE_HEADER              *CommHeader;
  HEAP_GUARD_TEST_COMM_BUFFER *VerificationCommBuffer;
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
  CommBufferSize = sizeof( HEAP_GUARD_TEST_COMM_BUFFER ) + OFFSET_OF( EFI_SMM_COMMUNICATE_HEADER, Data );
  if (CommBufferSize > mPiSmmCommonCommBufferSize) {
    DEBUG(( DEBUG_ERROR, __FUNCTION__" - Communication buffer is too small!\n" ));
    return EFI_ABORTED;
  }
  ZeroMem( CommHeader, CommBufferSize );

  //
  // Update some parameters.
  //
  // SMM Communication Parameters
  CopyGuid( &CommHeader->HeaderGuid, &gHeapGuardTestSmiHandlerGuid );
  CommHeader->MessageLength = sizeof( HEAP_GUARD_TEST_COMM_BUFFER );

  // Parameters Specific to this Implementation
  VerificationCommBuffer                = (HEAP_GUARD_TEST_COMM_BUFFER*)CommHeader->Data;
  VerificationCommBuffer->Function      = RequestedFunction;
  VerificationCommBuffer->Status        = EFI_NOT_FOUND;
  CopyMem(&VerificationCommBuffer->Context, Context, sizeof(HEAP_GUARD_TEST_CONTEXT));

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

  return VerificationCommBuffer->Status;
} // SmmMemoryProtectionsDxeToSmmCommunicate()



STATIC
VOID
LocateSmmCommonCommBuffer (
  VOID
  )
{
  EFI_STATUS                                Status;
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE   *PiSmmCommunicationRegionTable;
  EFI_MEMORY_DESCRIPTOR                     *SmmCommMemRegion;
  UINTN                                     Index, BufferSize;

  if (mPiSmmCommonCommBufferAddress == NULL) {
    Status = EfiGetSystemConfigurationTable( &gEdkiiPiSmmCommunicationRegionTableGuid, (VOID**)&PiSmmCommunicationRegionTable );

    // We only need a region large enough to hold a HEAP_GUARD_TEST_COMM_BUFFER,
    // so this shouldn't be too hard.
    BufferSize = 0;
    SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR*)(PiSmmCommunicationRegionTable + 1);
    for (Index = 0; Index < PiSmmCommunicationRegionTable->NumberOfEntries; Index++) {
      if (SmmCommMemRegion->Type == EfiConventionalMemory) {
        BufferSize = EFI_PAGES_TO_SIZE( (UINTN)SmmCommMemRegion->NumberOfPages );
        if (BufferSize >= (sizeof( HEAP_GUARD_TEST_COMM_BUFFER ) + OFFSET_OF( EFI_SMM_COMMUNICATE_HEADER, Data ))) {
          break;
        }
      }
      SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)SmmCommMemRegion + PiSmmCommunicationRegionTable->DescriptorSize);
    }

    mPiSmmCommonCommBufferAddress = (VOID*)SmmCommMemRegion->PhysicalStart;
    mPiSmmCommonCommBufferSize = BufferSize;
  }
} // LocateSmmCommonCommBuffer()

STATIC
#pragma warning(disable:4717)
UINT64
Recursion (
  UINT64 Count
  )
{
  UINT64 Sum = 0;
  DEBUG((DEBUG_ERROR, __FUNCTION__"  %x\n", Count));
  Sum = Recursion(++ Count);
  return Sum + Count;
}


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
  if ((PcdGet8(PcdHeapGuardPropertyMask) & BIT7) == 0)
  {
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
  else
  {
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
} // HeadPageTest()


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
} // TailPageTest()


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

VOID
NxTest (
  UINT8 *CodeRegionToCopyTo
  )
{
  #pragma warning(suppress:4054)
  UINT8   *CodeRegionToCopyFrom = (UINT8*)DummyFunctionForCodeSelfTest;
  CopyMem( CodeRegionToCopyTo, CodeRegionToCopyFrom, 512);

  DEBUG((DEBUG_ERROR, __FUNCTION__" writing to %p\n", CodeRegionToCopyTo));

  #pragma warning(suppress:4055)
  ((DUMMY_VOID_FUNCTION_FOR_DATA_TEST)CodeRegionToCopyTo)();

  DEBUG((DEBUG_ERROR, __FUNCTION__" failure \n"));
} // NxTest()


///================================================================================================
///================================================================================================
///
/// PRE REQ FUNCTIONS
///
///================================================================================================
///================================================================================================

UNIT_TEST_STATUS
EFIAPI
UefiHardwareNxProtectionEnabledPreReq (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  DEBUG((DEBUG_ERROR, __FUNCTION__"\n"));
  if (PcdGetBool(PcdSetNxForStack) || PcdGet64(PcdDxeNxMemoryProtectionPolicy)) {
    return UNIT_TEST_PASSED;
  }
  return UNIT_TEST_SKIPPED;
}


UNIT_TEST_STATUS
EFIAPI
UefiNxStackPreReq (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  DEBUG((DEBUG_ERROR, __FUNCTION__"\n"));
  if (PcdGetBool(PcdSetNxForStack) == FALSE) {
    return UNIT_TEST_SKIPPED;
  }
  if (UefiHardwareNxProtectionEnabled(Framework, Context) != UNIT_TEST_PASSED) {
    UT_LOG_WARNING("HardwareNxProtection bit not on. NX Test would not be accurate.");
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // UefiNxStackPreReq()


UNIT_TEST_STATUS
EFIAPI
UefiNxProtectionPreReq (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  UINT64 TestBit = LShiftU64(1, HeapGuardContext.TargetMemoryType);
  DEBUG((DEBUG_ERROR, __FUNCTION__"\n"));
  if ((PcdGet64(PcdDxeNxMemoryProtectionPolicy) & TestBit) == 0) {
    UT_LOG_WARNING("PCD for this memory type is disabled: %s", MEMORY_TYPES[HeapGuardContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }
  if (UefiHardwareNxProtectionEnabled(Framework, Context) != UNIT_TEST_PASSED) {
    UT_LOG_WARNING("HardwareNxProtection bit not on. NX Test would not be accurate.");
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // UefiNxProtectionPreReq()


UNIT_TEST_STATUS
EFIAPI
UefiPageGuardPreReq (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  UINT64 TestBit = LShiftU64(1, HeapGuardContext.TargetMemoryType);
  if (((PcdGet8(PcdHeapGuardPropertyMask) & BIT0) == 0) || ((PcdGet64(PcdHeapGuardPageType) & TestBit) == 0)) {
    UT_LOG_WARNING("PCD for this memory type is disabled: %s", MEMORY_TYPES[HeapGuardContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // UefiPageGuardPreReq()


UNIT_TEST_STATUS
EFIAPI
UefiPoolGuardPreReq (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  UINT64 TestBit = LShiftU64(1, HeapGuardContext.TargetMemoryType);
  if (((PcdGet8(PcdHeapGuardPropertyMask) & BIT1) == 0) || ((PcdGet64(PcdHeapGuardPoolType) & TestBit) == 0)) {
    UT_LOG_WARNING("PCD for this memory type is disabled: %s", MEMORY_TYPES[HeapGuardContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // UefiPoolGuardPreReq()


UNIT_TEST_STATUS
EFIAPI
UefiStackGuardPreReq (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  if (PcdGetBool(PcdCpuStackGuard) == FALSE) {
    UT_LOG_WARNING("PCD for this feature is disabled");
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // UefiStackGuardPreReq()


UNIT_TEST_STATUS
EFIAPI
UefiNullPointerPreReq (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  if ((PcdGet8(PcdNullPointerDetectionPropertyMask) & BIT0) == 0) {
    UT_LOG_WARNING("PCD for this feature is disabled");
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // UefiNullPointerPreReq


UNIT_TEST_STATUS
EFIAPI
SmmNxProtectionPreReq (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  UINT64 TestBit = LShiftU64(1, HeapGuardContext.TargetMemoryType);
  if ((PcdGet64(PcdDxeNxMemoryProtectionPolicy) & TestBit) == 0) {
    UT_LOG_WARNING("PCD for this memory type is disabled: %s", MEMORY_TYPES[HeapGuardContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }
  if (UefiHardwareNxProtectionEnabled(Framework, Context) != UNIT_TEST_PASSED) {
    UT_LOG_WARNING("HardwareNxProtection bit not on. NX Test would not be accurate.");
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
} // SmmNxProtectionPreReq()


UNIT_TEST_STATUS
EFIAPI
SmmPageGuardPreReq (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  UINT64 TestBit = LShiftU64(1, HeapGuardContext.TargetMemoryType);
  if (((PcdGet8(PcdHeapGuardPropertyMask) & BIT2) == 0) || ((PcdGet64(PcdHeapGuardPageType) & TestBit) == 0)) {
    UT_LOG_WARNING("PCD for this memory type is disabled: %s", MEMORY_TYPES[HeapGuardContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // SmmPageGuardPreReq()


UNIT_TEST_STATUS
EFIAPI
SmmPoolGuardPreReq (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  UINT64 TestBit = LShiftU64(1, HeapGuardContext.TargetMemoryType);
  if (((PcdGet8(PcdHeapGuardPropertyMask) & BIT3) == 0) || ((PcdGet64(PcdHeapGuardPoolType) & TestBit) == 0)) {
    UT_LOG_WARNING("PCD for this memory type is disabled: %s", MEMORY_TYPES[HeapGuardContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // SmmPoolGuardPreReq()


UNIT_TEST_STATUS
EFIAPI
SmmNullPointerPreReq (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  if (((PcdGet8(PcdNullPointerDetectionPropertyMask) & BIT1) == 0)) {
    UT_LOG_WARNING("PCD for this feature is disabled");
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // SmmNullPointerPreReq()

///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================

UNIT_TEST_STATUS
EFIAPI
UefiPageGuard (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  EFI_PHYSICAL_ADDRESS ptr;
  EFI_STATUS Status;

  if (HeapGuardContext.TestProgress < 2) {
    //
    // Context.TestProgress indicates progress within this specific test.
    // 0 - Just started.
    // 1 - Completed head guard test.
    // 2 - Completed tail guard test.
    //
    // We need to indicate we are working on the next part of the test and save our progress.
    //
    HeapGuardContext.TestProgress ++;
    SetUsbBootNext();
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

    Status = gBS->AllocatePages(AllocateAnyPages, (EFI_MEMORY_TYPE) HeapGuardContext.TargetMemoryType, 1, &ptr);

    if (EFI_ERROR(Status))
    {
      UT_LOG_WARNING("Memory allocation failed for type %s - %r\n", MEMORY_TYPES[HeapGuardContext.TargetMemoryType], Status);
      return UNIT_TEST_SKIPPED;
    }
    else if (HeapGuardContext.TestProgress == 1)
    {
      HeadPageTest((UINT64 *) ptr);

      //
      // Anything executing past this point indicates a failure.
      //
      UT_LOG_ERROR("Head guard page failed: %p", ptr);
    }
    else
    {
      TailPageTest((UINT64 *) ptr);

      //
      // Anything executing past this point indicates a failure.
      //
      UT_LOG_ERROR("Tail guard page failed: %p", ptr);
    }
    //
    // Reset test progress so failure gets recorded.
    //
    HeapGuardContext.TestProgress = 0;
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == 2);

  return UNIT_TEST_PASSED;
} // UefiPageGuard()


UNIT_TEST_STATUS
EFIAPI
UefiPoolGuard (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  UINT64 *ptr;
  EFI_STATUS Status;
  UINT64 AllocationSize;

  if (HeapGuardContext.TestProgress < NUM_POOL_SIZES) {
    //
    // Context.TestProgress indicates progress within this specific test.
    // The test progressively allocates larger areas to test the gaurd on.
    // These areas are defined in Pool.c as the 13 different sized chunks that are available
    // for pool allocation.
    //
    // We need to indicate we are working on the next part of the test and save our progress.
    //
    AllocationSize = mPoolSizeTable[HeapGuardContext.TestProgress];
    HeapGuardContext.TestProgress ++;
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
    SetUsbBootNext();

    //
    // Memory type rHardwareNxProtections to the bitmask for the PcdHeapGuardPoolType,
    // we need to RShift 1 to get it to reflect the correct EFI_MEMORY_TYPE.
    //
    Status = gBS->AllocatePool((EFI_MEMORY_TYPE) HeapGuardContext.TargetMemoryType, AllocationSize, &ptr);

    if (EFI_ERROR(Status)) {
      UT_LOG_WARNING("Memory allocation failed for type %s of size %x - %r\n", MEMORY_TYPES[HeapGuardContext.TargetMemoryType], AllocationSize, Status);
      return UNIT_TEST_SKIPPED;
    }
    else {
      PoolTest((UINT64 *) ptr, AllocationSize);

      //
      // At this point, the test has failed. Reset test progress so failure gets recorded.
      //
      HeapGuardContext.TestProgress = 0;
      SaveFrameworkState(Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
      UT_LOG_ERROR("Pool guard failed: %p", ptr);
    }
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == NUM_POOL_SIZES);

  return UNIT_TEST_PASSED;
} // UefiPoolGuard()


UNIT_TEST_STATUS
EFIAPI
UefiCpuStackGuard (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  if (HeapGuardContext.TestProgress < 1) {
    //
    // Context.TestProgress 0 indicates the test hasn't started yet.
    //
    // We need to indicate we are working on the test and save our progress.
    //
    HeapGuardContext.TestProgress ++;
    SetUsbBootNext();
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

    Recursion(1);

    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    HeapGuardContext.TestProgress = 0;
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
    UT_LOG_ERROR( "System was expected to reboot, but didn't." );
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == 1);

  return UNIT_TEST_PASSED;
} // UefiCpuStackGuard()


volatile UNIT_TEST_FRAMEWORK       *mFw = NULL;
UNIT_TEST_STATUS
EFIAPI
UefiNullPointerDetection (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  if (HeapGuardContext.TestProgress < 2) {
    //
    // Context.TestProgress indicates progress within this specific test.
    // 0 - Just started.
    // 1 - Completed NULL pointer read test.
    // 2 - Completed NULL pointer write test.
    //
    // We need to indicate we are working on the next part of the test and save our progress.
    //
    HeapGuardContext.TestProgress ++;
    SetUsbBootNext();
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

    if (HeapGuardContext.TestProgress == 1) {
      if (mFw->Title == NULL) {
        DEBUG((DEBUG_ERROR,  __FUNCTION__" should have failed \n"));
      }
      UT_LOG_ERROR( "Failed NULL pointer read test." );
    }
    else {
      mFw->Title = L"Title";
      UT_LOG_ERROR( "Failed NULL pointer write test." );
    }

    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    HeapGuardContext.TestProgress = 0;
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == 2);

  return UNIT_TEST_PASSED;
} // UefiNullPointerDetection()


UNIT_TEST_STATUS
EFIAPI
UefiNxStackGuard (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  DEBUG((DEBUG_ERROR, __FUNCTION__"\n"));
  UINT8 CodeRegionToCopyTo[512];
  #pragma warning(suppress:4054)
  UINT8   *CodeRegionToCopyFrom = (UINT8*)DummyFunctionForCodeSelfTest;

  if (HeapGuardContext.TestProgress < 1) {
    //
    // Context.TestProgress 0 indicates the test hasn't started yet.
    //
    // We need to indicate we are working on the test and save our progress.
    //
    HeapGuardContext.TestProgress ++;
    SetUsbBootNext();
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

    CopyMem( CodeRegionToCopyTo, CodeRegionToCopyFrom, 512);
    #pragma warning(suppress:4055)
    ((DUMMY_VOID_FUNCTION_FOR_DATA_TEST)CodeRegionToCopyTo)();


    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    HeapGuardContext.TestProgress = 0;
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
    UT_LOG_ERROR("NX Test failed.");
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == 1);

  return UNIT_TEST_PASSED;
} // UefiNxStackGuard()


UNIT_TEST_STATUS
EFIAPI
UefiNxProtection (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  UINT64 *ptr;
  EFI_STATUS Status;
  if (HeapGuardContext.TestProgress < 1) {
    //
    // Context.TestProgress 0 indicates the test hasn't started yet.
    //
    // We need to indicate we are working on the test and save our progress.
    //
    HeapGuardContext.TestProgress ++;
    SetUsbBootNext();
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

    Status = gBS->AllocatePool((EFI_MEMORY_TYPE) HeapGuardContext.TargetMemoryType, 4096, &ptr);

    if (EFI_ERROR(Status)) {
      UT_LOG_WARNING("Memory allocation failed for type %s - %r\n", MEMORY_TYPES[HeapGuardContext.TargetMemoryType], Status);
      return UNIT_TEST_SKIPPED;
    }
    else {
      NxTest((UINT8 *) ptr);
    }

    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    HeapGuardContext.TestProgress = 0;
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
    UT_LOG_ERROR("NX Test failed.");
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == 1);

  return UNIT_TEST_PASSED;
} // UefiNxProtection()

UNIT_TEST_STATUS
EFIAPI
SmmPageGuard (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  EFI_STATUS Status;
  if (HeapGuardContext.TestProgress < 2) {
    //
    // Context.TestProgress indicates progress within this specific test.
    // 0 - Just started.
    // 1 - Completed head guard test.
    // 2 - Completed tail guard test.
    //
    // We need to indicate we are working on the next part of the test and save our progress.
    //
    HeapGuardContext.TestProgress ++;
    SetUsbBootNext();
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

    Status = SmmMemoryProtectionsDxeToSmmCommunicate(HEAP_GUARD_TEST_PAGE, &HeapGuardContext);
    if (Status == EFI_NOT_FOUND) {
      UT_LOG_ERROR( "SMM test driver is not loaded." );
    }
    else {
      UT_LOG_ERROR( "System was expected to reboot, but didn't." );
    }

    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    HeapGuardContext.TestProgress = 0;
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == 2);

  return UNIT_TEST_PASSED;
} // SmmPageGuard()


UNIT_TEST_STATUS
EFIAPI
SmmPoolGuard (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  EFI_STATUS Status;

  if (HeapGuardContext.TestProgress < NUM_POOL_SIZES) {
    //
    // Context.TestProgress indicates progress within this specific test.
    // The test progressively allocates larger areas to test the gaurd on.
    // These areas are defined in Pool.c as the 13 different sized chunks that are available
    // for pool allocation.
    //
    // We need to indicate we are working on the next part of the test and save our progress.
    //
    HeapGuardContext.TestProgress ++;
    SetUsbBootNext();
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

    Status = SmmMemoryProtectionsDxeToSmmCommunicate(HEAP_GUARD_TEST_POOL, &HeapGuardContext);

    if (Status == EFI_NOT_FOUND) {
      UT_LOG_ERROR( "SMM test driver is not loaded." );
    }
    else {
      UT_LOG_ERROR( "System was expected to reboot, but didn't." );
    }

    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    HeapGuardContext.TestProgress = 0;
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == 1);

  return UNIT_TEST_PASSED;
} // SmmPoolGuard()


UNIT_TEST_STATUS
EFIAPI
SmmNullPointerDetection (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  EFI_STATUS Status;

  if (HeapGuardContext.TestProgress < 1) {
    //
    // Context.TestProgress 0 indicates the test hasn't started yet.
    //
    // We need to indicate we are working on the test and save our progress.
    //
    HeapGuardContext.TestProgress ++;
    SetUsbBootNext();
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

    Status = SmmMemoryProtectionsDxeToSmmCommunicate(HEAP_GUARD_TEST_NULL_POINTER, &HeapGuardContext);

    if (Status == EFI_NOT_FOUND) {
      UT_LOG_ERROR( "SMM test driver is not loaded." );
    }
    else {
      UT_LOG_ERROR( "System was expected to reboot, but didn't. %r", Status );
    }

    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    HeapGuardContext.TestProgress = 0;
    SaveFrameworkState( Framework, &HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == 1);


  return UNIT_TEST_PASSED;
} // SmmNullPointerDetection()


///================================================================================================
///================================================================================================
///
/// TEST ENGINE
///
///================================================================================================
///================================================================================================
VOID
AddUefiNxTest(
  UNIT_TEST_SUITE           *TestSuite
  )
{
  HEAP_GUARD_TEST_CONTEXT *HeapGuardContext = NULL;
  UINT8 Index;
  CHAR16 NameStub[] = L"Security.NxProtection.Uefi";
  CHAR16 DescriptionStub[] = L"Accesses before/after the pool should hit a guard page. Memory type: ";
  CHAR16 *TestName = NULL;
  CHAR16 *TestDescription = NULL;
  UINTN TestNameSize;
  UINTN TestDescriptionSize;

  //
  // Need to generate a test case for each type of memory.
  //
  for (Index = 0; Index < NUM_MEMORY_TYPES; Index ++) {
    //
    // Set memory type according to the bitmask for PcdDxeNxMemoryProtectionPolicy.
    // The test progress should start at 0.
    //
    HeapGuardContext =  AllocateZeroPool(sizeof(HEAP_GUARD_TEST_CONTEXT));
    HeapGuardContext->TargetMemoryType = Index;

    TestNameSize = sizeof(CHAR16) * (1 + StrnLenS(NameStub, UNIT_TEST_MAX_STRING_LENGTH) + StrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName = AllocateZeroPool(TestNameSize);

    TestDescriptionSize = sizeof(CHAR16) * (1 + StrnLenS(DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + StrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription = AllocateZeroPool(TestDescriptionSize);

    if (TestName != NULL && TestDescription != NULL && HeapGuardContext != NULL) {
      //
      // Name of the test is Security.PageGuard.Uefi + Memory Type Name (from MEMORY_TYPES)
      //
      StrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      StrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      //
      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)

      StrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      StrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      AddTestCase( TestSuite, TestDescription, TestName, UefiNxProtection, UefiPageGuardPreReq, NULL, HeapGuardContext);

      FreePool(TestName);
      FreePool(TestDescription);
    }
    else
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " allocating memory for test creation failed.\n"));
      return;
    }
  }
} // AddUefiNxTest()


VOID
AddUefiPoolTest(
  UNIT_TEST_SUITE           *TestSuite
  )
{
  HEAP_GUARD_TEST_CONTEXT *HeapGuardContext = NULL;
  UINT8 Index;
  CHAR16 NameStub[] = L"Security.PoolGuard.Uefi";
  CHAR16 DescriptionStub[] = L"Accesses before/after the pool should hit a guard page. Memory type: ";
  CHAR16 *TestName = NULL;
  CHAR16 *TestDescription = NULL;
  UINTN TestNameSize;
  UINTN TestDescriptionSize;

  //
  // Need to generate a test case for each type of memory.
  //
  for (Index = 0; Index < NUM_MEMORY_TYPES; Index ++) {
    //
    // Set memory type according to the bitmask for PcdHeapGuardPoolType.
    // The test progress should start at 0.
    //
    HeapGuardContext =  AllocateZeroPool(sizeof(HEAP_GUARD_TEST_CONTEXT));
    HeapGuardContext->TargetMemoryType = Index;

    //
    // Name of the test is Security.PoolGuard.Uefi + Memory Type Name (from MEMORY_TYPES)
    //
    TestNameSize = sizeof(CHAR16) * (1 + StrnLenS(NameStub, UNIT_TEST_MAX_STRING_LENGTH) + StrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName = AllocateZeroPool(TestNameSize);

    TestDescriptionSize = sizeof(CHAR16) * (1 + StrnLenS(DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + StrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription = AllocateZeroPool(TestDescriptionSize);

    if (TestName != NULL && TestDescription != NULL && HeapGuardContext != NULL) {
      StrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      StrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      //
      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)

      StrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      StrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      AddTestCase( TestSuite, TestDescription, TestName, UefiPoolGuard, UefiPoolGuardPreReq, NULL, HeapGuardContext);

      FreePool(TestName);
      FreePool(TestDescription);
    }
    else
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " allocating memory for test creation failed.\n"));
      return;
    }
  }
} // AddUefiPoolTest()


VOID
AddUefiPageTest(
  UNIT_TEST_SUITE           *TestSuite
  )
{
  HEAP_GUARD_TEST_CONTEXT *HeapGuardContext = NULL;
  UINT8 Index;
  CHAR16 NameStub[] = L"Security.PageGuard.Uefi";
  CHAR16 DescriptionStub[] = L"Accesses before and after an allocated page should hit a guard page. Memory type: ";
  CHAR16 *TestName = NULL;
  CHAR16 *TestDescription = NULL;
  UINTN TestNameSize;
  UINTN TestDescriptionSize;

  //
  // Need to generate a test case for each type of memory.
  //
  for (Index = 0; Index < NUM_MEMORY_TYPES; Index ++) {
    //
    // Set memory type according to the bitmask for PcdHeapGuardPageType.
    // The test progress should start at 0.
    //
    HeapGuardContext =  AllocateZeroPool(sizeof(HEAP_GUARD_TEST_CONTEXT));
    HeapGuardContext->TargetMemoryType = Index;

    TestNameSize = sizeof(CHAR16) * (1 + StrnLenS(NameStub, UNIT_TEST_MAX_STRING_LENGTH) + StrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName = AllocateZeroPool(TestNameSize);

    TestDescriptionSize = sizeof(CHAR16) * (1 + StrnLenS(DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + StrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription = AllocateZeroPool(TestDescriptionSize);

    if (TestName != NULL && TestDescription != NULL && HeapGuardContext != NULL) {
      //
      // Name of the test is Security.PageGuard.Uefi + Memory Type Name (from MEMORY_TYPES)
      //
      StrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      StrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      //
      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)

      StrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      StrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      AddTestCase( TestSuite, TestDescription, TestName, UefiPageGuard, UefiPageGuardPreReq, NULL, HeapGuardContext);

      FreePool(TestName);
      FreePool(TestDescription);
    }
    else
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " allocating memory for test creation failed.\n"));
      return;
    }
  }
} // AddUefiPageTest()


VOID
AddSmmPoolTest(
  UNIT_TEST_SUITE           *TestSuite
  )
{
  HEAP_GUARD_TEST_CONTEXT *HeapGuardContext = NULL;
  UINT8 Index;
  CHAR16 NameStub[] = L"Security.PoolGuard.Smm";
  CHAR16 DescriptionStub[] = L"Accesses before/after the pool should hit a guard page in SMM. Memory type: ";
  CHAR16 *TestName = NULL;
  CHAR16 *TestDescription = NULL;
  UINTN TestNameSize;
  UINTN TestDescriptionSize;

  //
  // Need to generate a test case for each type of memory.
  //
  for (Index = 0; Index < NUM_MEMORY_TYPES; Index ++) {
    //
    // Set memory type according to the bitmask for PcdHeapGuardPoolType.
    // The test progress should start at 0.
    //
    HeapGuardContext =  AllocateZeroPool(sizeof(HEAP_GUARD_TEST_CONTEXT));
    HeapGuardContext->TargetMemoryType = Index;

    TestNameSize = sizeof(CHAR16) * (1 + StrnLenS(NameStub, UNIT_TEST_MAX_STRING_LENGTH) + StrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName = AllocateZeroPool(TestNameSize);

    TestDescriptionSize = sizeof(CHAR16) * (1 + StrnLenS(DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + StrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription = AllocateZeroPool(TestDescriptionSize);

    if (TestName != NULL && TestDescription != NULL && HeapGuardContext != NULL) {
      //
      // Name of the test is Security.PoolGuard.Smm + Memory Type Name (from MEMORY_TYPES)
      //
      StrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      StrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      //
      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)

      StrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      StrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      AddTestCase( TestSuite, TestDescription, TestName, SmmPoolGuard, SmmPoolGuardPreReq, NULL, HeapGuardContext);

      FreePool(TestName);
      FreePool(TestDescription);
    }
    else
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " allocating memory for test creation failed.\n"));
      return;
    }
  }
} // AddSmmPoolTest()


VOID
AddSmmPageTest(
  UNIT_TEST_SUITE           *TestSuite
  )
{
  HEAP_GUARD_TEST_CONTEXT *HeapGuardContext = NULL;
  UINT8 Index;
  CHAR16 NameStub[] = L"Security.PageGuard.Smm";
  CHAR16 DescriptionStub[] = L"Accesses before and after an allocated page should hit a guard page in SMM. Memory type: ";
  CHAR16 *TestName = NULL;
  CHAR16 *TestDescription = NULL;
  UINTN TestNameSize;
  UINTN TestDescriptionSize;

  //
  // Need to generate a test case for each type of memory.
  //
  for (Index = 0; Index < NUM_MEMORY_TYPES; Index ++) {
    //
    // Set memory type according to the bitmask for PcdHeapGuardPageType.
    // The test progress should start at 0.
    //
    HeapGuardContext =  AllocateZeroPool(sizeof(HEAP_GUARD_TEST_CONTEXT));
    HeapGuardContext->TargetMemoryType = Index;

    TestNameSize = sizeof(CHAR16) * (1 + StrnLenS(NameStub, UNIT_TEST_MAX_STRING_LENGTH) + StrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName = AllocateZeroPool(TestNameSize);

    TestDescriptionSize = sizeof(CHAR16) * (1 + StrnLenS(DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + StrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription = AllocateZeroPool(TestDescriptionSize);

    if (TestName != NULL && TestDescription != NULL && HeapGuardContext != NULL) {
      //
      // Name of the test is Security.PageGuard.Smm + Memory Type Name (from MEMORY_TYPES)
      //
      StrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      StrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      //
      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)

      StrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      StrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      AddTestCase( TestSuite, TestDescription, TestName, SmmPageGuard, SmmPageGuardPreReq, NULL, HeapGuardContext);

      FreePool(TestName);
      FreePool(TestDescription);
    }
    else
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " allocating memory for test creation failed.\n"));
      return;
    }
  }
} // AddSmmPageTest()


/**
  HeapGuardTestAppEntryPoint

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
HeapGuardTestAppEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  UNIT_TEST_FRAMEWORK       *Fw = NULL;
  UNIT_TEST_SUITE           *PageGuard = NULL;
  UNIT_TEST_SUITE           *PoolGuard = NULL;
  UNIT_TEST_SUITE           *NxProtection = NULL;
  UNIT_TEST_SUITE           *Misc = NULL;
  HEAP_GUARD_TEST_CONTEXT *HeapGuardContext = AllocateZeroPool(sizeof(HEAP_GUARD_TEST_CONTEXT));

  CHAR16  ShortName[100];
  ShortName[0] = L'\0';
  DEBUG((DEBUG_ERROR, __FUNCTION__ "()\n"));

  UnicodeSPrint(&ShortName[0], sizeof(ShortName), L"%a", gEfiCallerBaseName);
  DEBUG(( DEBUG_ERROR, "%s v%s\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

  LocateSmmCommonCommBuffer();

  //
  // Find the CPU Arch protocol, we're going to install our own interrupt handler
  // with it later.
  //
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **) &mCpu);
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed to locate gEfiCpuArchProtocolGuid. Status = %r\n", Status));
    goto EXIT;
  }

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
  // Create seperate test suites for Page, Pool, NX tests.
  // Misc test suite for stack guard and null pointer.
  //
  CreateUnitTestSuite( &Misc, Fw, L"Stack Guard and Null Pointer Detection", L"Security.HeapGuardMisc", NULL, NULL);
  CreateUnitTestSuite( &PageGuard, Fw, L"Page Guard Tests", L"Security.PageGuard", NULL, NULL);
  CreateUnitTestSuite( &PoolGuard, Fw, L"Pool Guard Tests", L"Security.PoolGuard", NULL, NULL);
  CreateUnitTestSuite( &NxProtection, Fw, L"NX Protection Tests", L"Security.NxProtection", NULL, NULL);

  if ((PageGuard == NULL) || (PoolGuard == NULL) || (NxProtection == NULL) || (Misc == NULL))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for TestSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddUefiPoolTest(PoolGuard);
  AddUefiPageTest(PageGuard);
  AddSmmPageTest(PageGuard);
  AddSmmPoolTest(PoolGuard);
  AddUefiNxTest(NxProtection);

  AddTestCase( Misc, L"Null pointer access should trigger a page fault", L"Security.HeapGuardMisc.UefiNullPointerDetection", UefiNullPointerDetection, UefiNullPointerPreReq, NULL, HeapGuardContext );
  AddTestCase( Misc, L"Null pointer access in SMM should trigger a page fault", L"Security.HeapGuardMisc.SmmNullPointerDetection", SmmNullPointerDetection, SmmNullPointerPreReq, NULL, HeapGuardContext );
  AddTestCase( Misc, L"Blowing the stack should trigger a page fault", L"Security.HeapGuardMisc.UefiCpuStackGuard", UefiCpuStackGuard, UefiStackGuardPreReq, NULL, HeapGuardContext );
  AddTestCase( NxProtection, L"Check hardware configuration of HardwareNxProtection bit", L"Security.HeapGuardMisc.UefiHardwareNxProtectionEnabled", UefiHardwareNxProtectionEnabled, UefiHardwareNxProtectionEnabledPreReq, NULL, HeapGuardContext );
  AddTestCase( NxProtection, L"Stack NX Protection", L"Security.HeapGuardMisc.UefiNxStackGuard", UefiNxStackGuard, UefiNxStackPreReq, NULL, HeapGuardContext );

  //
  // Install an interrupt handler to reboot on page faults.
  //
  Status = mCpu->RegisterInterruptHandler (mCpu, EXCEPT_IA32_PAGE_FAULT, InterruptHandler);
  if (EFI_ERROR( Status ))
  {
    DEBUG(( DEBUG_ERROR, "Failed to install interrupt handler. Status = %r\n", Status ));
    goto EXIT;
  }

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
} // HeapGuardTestAppEntryPoint()
