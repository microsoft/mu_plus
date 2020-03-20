/** @file -- HeapGuardTestApp.c

Tests for page guard, pool guard, NX protections, stack guard, and null pointer detection.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
#include <UnitTestFrameworkTypes.h>
#include <Library/UnitTestLib.h>
#include <Library/UnitTestBootLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Register\ArchitecturalMsr.h>

#include <Guid/PiSmmCommunicationRegionTable.h>

#include "../HeapGuardTestCommon.h"
#include "UefiHardwareNxProtectionStub.h"


#define UNIT_TEST_APP_NAME        "Heap Guard Test"
#define UNIT_TEST_APP_VERSION     "0.5"

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
  DEBUG((DEBUG_ERROR, "%a SystemContextX64->ExceptionData: %x - InterruptType: %x\n", __FUNCTION__, SystemContext.SystemContextX64->ExceptionData, InterruptType));
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
    DEBUG(( DEBUG_ERROR, "%a - Communication buffer not found!\n", __FUNCTION__ ));
    return EFI_ABORTED;
  }

  //
  // First, let's zero the comm buffer. Couldn't hurt.
  //
  CommHeader = (EFI_SMM_COMMUNICATE_HEADER*)mPiSmmCommonCommBufferAddress;
  CommBufferSize = sizeof( HEAP_GUARD_TEST_COMM_BUFFER ) + OFFSET_OF( EFI_SMM_COMMUNICATE_HEADER, Data );
  if (CommBufferSize > mPiSmmCommonCommBufferSize) {
    DEBUG(( DEBUG_ERROR, "%a - Communication buffer is too small!\n", __FUNCTION__ ));
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
    DEBUG(( DEBUG_VERBOSE, "%a - Communicate() = %r\n", __FUNCTION__, Status ));
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
UINT64
Recursion (
  UINT64 Count
  )
{
  UINT64 Sum = 0;
  DEBUG((DEBUG_ERROR, "%a  %x\n", __FUNCTION__, Count));
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
  DEBUG((DEBUG_ERROR, "%a Allocated pool at 0x%p\n", __FUNCTION__, ptr));

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
    // The guard page will be immediately preceding the page the pool starts on.
    //
    ptrLoc = (UINT64*) (((UINTN) ptrLoc) - 0x1);
  }
  DEBUG((DEBUG_ERROR, "%a Writing to 0x%p\n", __FUNCTION__, ptrLoc));
  *ptrLoc = 1;
  DEBUG((DEBUG_ERROR, "%a failure \n", __FUNCTION__));
} // PoolTest()


VOID
HeadPageTest (
  IN UINT64* ptr
  )
{
  DEBUG((DEBUG_ERROR, "%a Allocated page at 0x%p\n", __FUNCTION__, ptr));

  // Hit the head guard page
  ptr = (UINT64*) (((UINTN) ptr) - 0x1);
  DEBUG((DEBUG_ERROR, "%a Writing to 0x%p\n", __FUNCTION__, ptr));
  *ptr = 1;

  DEBUG((DEBUG_ERROR, "%a failure \n", __FUNCTION__));
} // HeadPageTest()


VOID
TailPageTest (
  IN UINT64* ptr
  )
{
  DEBUG((DEBUG_ERROR, "%a Allocated page at 0x%p\n", __FUNCTION__, ptr));

  // Hit the tail guard page
  ptr = (UINT64*) (((UINTN) ptr) + 0x1000);
  DEBUG((DEBUG_ERROR, "%a Writing to 0x%p\n", __FUNCTION__, ptr));
  *ptr = 1;
  DEBUG((DEBUG_ERROR, "%a failure \n", __FUNCTION__));
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
  UINT8   *CodeRegionToCopyFrom = (UINT8*)DummyFunctionForCodeSelfTest;
  CopyMem( CodeRegionToCopyTo, CodeRegionToCopyFrom, 512);

  DEBUG((DEBUG_ERROR, "%a writing to %p\n", __FUNCTION__, CodeRegionToCopyTo));

  ((DUMMY_VOID_FUNCTION_FOR_DATA_TEST)CodeRegionToCopyTo)();

  DEBUG((DEBUG_ERROR, "%a failure \n", __FUNCTION__));
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
  IN UNIT_TEST_CONTEXT           Context
  )
{
  DEBUG((DEBUG_ERROR, "%a\n", __FUNCTION__));
  if (PcdGetBool(PcdSetNxForStack) || PcdGet64(PcdDxeNxMemoryProtectionPolicy)) {
    return UNIT_TEST_PASSED;
  }
  return UNIT_TEST_SKIPPED;
}


UNIT_TEST_STATUS
EFIAPI
UefiNxStackPreReq (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  DEBUG((DEBUG_ERROR, "%a\n", __FUNCTION__));
  if (PcdGetBool(PcdSetNxForStack) == FALSE) {
    return UNIT_TEST_SKIPPED;
  }
  if (UefiHardwareNxProtectionEnabled(Context) != UNIT_TEST_PASSED) {
    UT_LOG_WARNING("HardwareNxProtection bit not on. NX Test would not be accurate.");
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // UefiNxStackPreReq()


UNIT_TEST_STATUS
EFIAPI
UefiNxProtectionPreReq (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  UINT64 TestBit = LShiftU64(1, HeapGuardContext.TargetMemoryType);
  DEBUG((DEBUG_ERROR, "%a\n", __FUNCTION__));
  if ((PcdGet64(PcdDxeNxMemoryProtectionPolicy) & TestBit) == 0) {
    UT_LOG_WARNING("PCD for this memory type is disabled: %a", MEMORY_TYPES[HeapGuardContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }
  if (UefiHardwareNxProtectionEnabled(Context) != UNIT_TEST_PASSED) {
    UT_LOG_WARNING("HardwareNxProtection bit not on. NX Test would not be accurate.");
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // UefiNxProtectionPreReq()


UNIT_TEST_STATUS
EFIAPI
UefiPageGuardPreReq (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  UINT64 TestBit = LShiftU64(1, HeapGuardContext.TargetMemoryType);
  if (((PcdGet8(PcdHeapGuardPropertyMask) & BIT0) == 0) || ((PcdGet64(PcdHeapGuardPageType) & TestBit) == 0)) {
    UT_LOG_WARNING("PCD for this memory type is disabled: %a", MEMORY_TYPES[HeapGuardContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // UefiPageGuardPreReq()


UNIT_TEST_STATUS
EFIAPI
UefiPoolGuardPreReq (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  UINT64 TestBit = LShiftU64(1, HeapGuardContext.TargetMemoryType);
  if (((PcdGet8(PcdHeapGuardPropertyMask) & BIT1) == 0) || ((PcdGet64(PcdHeapGuardPoolType) & TestBit) == 0)) {
    UT_LOG_WARNING("PCD for this memory type is disabled: %a", MEMORY_TYPES[HeapGuardContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // UefiPoolGuardPreReq()


UNIT_TEST_STATUS
EFIAPI
UefiStackGuardPreReq (
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
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  UINT64 TestBit = LShiftU64(1, HeapGuardContext.TargetMemoryType);
  if ((PcdGet64(PcdDxeNxMemoryProtectionPolicy) & TestBit) == 0) {
    UT_LOG_WARNING("PCD for this memory type is disabled: %a", MEMORY_TYPES[HeapGuardContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }
  if (UefiHardwareNxProtectionEnabled(Context) != UNIT_TEST_PASSED) {
    UT_LOG_WARNING("HardwareNxProtection bit not on. NX Test would not be accurate.");
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
} // SmmNxProtectionPreReq()


UNIT_TEST_STATUS
EFIAPI
SmmPageGuardPreReq (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  UINT64 TestBit = LShiftU64(1, HeapGuardContext.TargetMemoryType);
  if (((PcdGet8(PcdHeapGuardPropertyMask) & BIT2) == 0) || ((PcdGet64(PcdHeapGuardPageType) & TestBit) == 0)) {
    UT_LOG_WARNING("PCD for this memory type is disabled: %a", MEMORY_TYPES[HeapGuardContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // SmmPageGuardPreReq()


UNIT_TEST_STATUS
EFIAPI
SmmPoolGuardPreReq (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  UINT64 TestBit = LShiftU64(1, HeapGuardContext.TargetMemoryType);
  if (((PcdGet8(PcdHeapGuardPropertyMask) & BIT3) == 0) || ((PcdGet64(PcdHeapGuardPoolType) & TestBit) == 0)) {
    UT_LOG_WARNING("PCD for this memory type is disabled: %a", MEMORY_TYPES[HeapGuardContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // SmmPoolGuardPreReq()


UNIT_TEST_STATUS
EFIAPI
SmmNullPointerPreReq (
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
    SetBootNextDevice();
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

    Status = gBS->AllocatePages(AllocateAnyPages, (EFI_MEMORY_TYPE) HeapGuardContext.TargetMemoryType, 1, (EFI_PHYSICAL_ADDRESS *)&ptr);

    if (EFI_ERROR(Status))
    {
      UT_LOG_WARNING("Memory allocation failed for type %a - %r\n", MEMORY_TYPES[HeapGuardContext.TargetMemoryType], Status);
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
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == 2);

  return UNIT_TEST_PASSED;
} // UefiPageGuard()


UNIT_TEST_STATUS
EFIAPI
UefiPoolGuard (
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
    // The test progressively allocates larger areas to test the guard on.
    // These areas are defined in Pool.c as the 13 different sized chunks that are available
    // for pool allocation.
    //
    // We need to indicate we are working on the next part of the test and save our progress.
    //
    AllocationSize = mPoolSizeTable[HeapGuardContext.TestProgress];
    HeapGuardContext.TestProgress ++;
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
    SetBootNextDevice();

    //
    // Memory type rHardwareNxProtections to the bitmask for the PcdHeapGuardPoolType,
    // we need to RShift 1 to get it to reflect the correct EFI_MEMORY_TYPE.
    //
    Status = gBS->AllocatePool((EFI_MEMORY_TYPE) HeapGuardContext.TargetMemoryType, AllocationSize, (VOID **)&ptr);

    if (EFI_ERROR(Status)) {
      UT_LOG_WARNING("Memory allocation failed for type %a of size %x - %r\n", MEMORY_TYPES[HeapGuardContext.TargetMemoryType], AllocationSize, Status);
      return UNIT_TEST_SKIPPED;
    }
    else {
      PoolTest((UINT64 *) ptr, AllocationSize);

      //
      // At this point, the test has failed. Reset test progress so failure gets recorded.
      //
      HeapGuardContext.TestProgress = 0;
      SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
      UT_LOG_ERROR("Pool guard failed: %p", ptr);
    }
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == NUM_POOL_SIZES);

  return UNIT_TEST_PASSED;
} // UefiPoolGuard()


UNIT_TEST_STATUS
EFIAPI
UefiCpuStackGuard (
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
    SetBootNextDevice();
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

    Recursion(1);

    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    HeapGuardContext.TestProgress = 0;
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
    UT_LOG_ERROR( "System was expected to reboot, but didn't." );
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == 1);

  return UNIT_TEST_PASSED;
} // UefiCpuStackGuard()


volatile UNIT_TEST_FRAMEWORK       *mFw = NULL;
UNIT_TEST_STATUS
EFIAPI
UefiNullPointerDetection (
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
    SetBootNextDevice();
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

    if (HeapGuardContext.TestProgress == 1) {
      if (mFw->Title == NULL) {
        DEBUG((DEBUG_ERROR,  "%a should have failed \n", __FUNCTION__));
      }
      UT_LOG_ERROR( "Failed NULL pointer read test." );
    }
    else {
      mFw->Title = "Title";
      UT_LOG_ERROR( "Failed NULL pointer write test." );
    }

    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    HeapGuardContext.TestProgress = 0;
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == 2);

  return UNIT_TEST_PASSED;
} // UefiNullPointerDetection()


UNIT_TEST_STATUS
EFIAPI
UefiNxStackGuard (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  DEBUG((DEBUG_ERROR, "%a\n", __FUNCTION__));
  UINT8 CodeRegionToCopyTo[512];
  UINT8   *CodeRegionToCopyFrom = (UINT8*)DummyFunctionForCodeSelfTest;

  if (HeapGuardContext.TestProgress < 1) {
    //
    // Context.TestProgress 0 indicates the test hasn't started yet.
    //
    // We need to indicate we are working on the test and save our progress.
    //
    HeapGuardContext.TestProgress ++;
    SetBootNextDevice();
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

    CopyMem( CodeRegionToCopyTo, CodeRegionToCopyFrom, 512);
    ((DUMMY_VOID_FUNCTION_FOR_DATA_TEST)CodeRegionToCopyTo)();


    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    HeapGuardContext.TestProgress = 0;
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
    UT_LOG_ERROR("NX Test failed.");
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == 1);

  return UNIT_TEST_PASSED;
} // UefiNxStackGuard()


UNIT_TEST_STATUS
EFIAPI
UefiNxProtection (
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
    SetBootNextDevice();
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

    Status = gBS->AllocatePool((EFI_MEMORY_TYPE) HeapGuardContext.TargetMemoryType, 4096, (VOID **)&ptr);

    if (EFI_ERROR(Status)) {
      UT_LOG_WARNING("Memory allocation failed for type %a - %r\n", MEMORY_TYPES[HeapGuardContext.TargetMemoryType], Status);
      return UNIT_TEST_SKIPPED;
    }
    else {
      NxTest((UINT8 *) ptr);
    }

    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    HeapGuardContext.TestProgress = 0;
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
    UT_LOG_ERROR("NX Test failed.");
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == 1);

  return UNIT_TEST_PASSED;
} // UefiNxProtection()

UNIT_TEST_STATUS
EFIAPI
SmmPageGuard (
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
    SetBootNextDevice();
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

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
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == 2);

  return UNIT_TEST_PASSED;
} // SmmPageGuard()


UNIT_TEST_STATUS
EFIAPI
SmmPoolGuard (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  HEAP_GUARD_TEST_CONTEXT HeapGuardContext = (*(HEAP_GUARD_TEST_CONTEXT *)Context);
  EFI_STATUS Status;

  if (HeapGuardContext.TestProgress < NUM_POOL_SIZES) {
    //
    // Context.TestProgress indicates progress within this specific test.
    // The test progressively allocates larger areas to test the guard on.
    // These areas are defined in Pool.c as the 13 different sized chunks that are available
    // for pool allocation.
    //
    // We need to indicate we are working on the next part of the test and save our progress.
    //
    HeapGuardContext.TestProgress ++;
    SetBootNextDevice();
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

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
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
  }

  UT_ASSERT_TRUE(HeapGuardContext.TestProgress == 1);

  return UNIT_TEST_PASSED;
} // SmmPoolGuard()


UNIT_TEST_STATUS
EFIAPI
SmmNullPointerDetection (
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
    SetBootNextDevice();
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));

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
    SaveFrameworkState(&HeapGuardContext, sizeof(HEAP_GUARD_TEST_CONTEXT));
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
  UNIT_TEST_SUITE_HANDLE     TestSuite
  )
{
  HEAP_GUARD_TEST_CONTEXT *HeapGuardContext = NULL;
  UINT8 Index;
  CHAR8 NameStub[] = "Security.NxProtection.Uefi";
  CHAR8 DescriptionStub[] = "Accesses before/after the pool should hit a guard page. Memory type: ";
  CHAR8 *TestName = NULL;
  CHAR8 *TestDescription = NULL;
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
    HeapGuardContext =  (HEAP_GUARD_TEST_CONTEXT *)AllocateZeroPool(sizeof(HEAP_GUARD_TEST_CONTEXT));
    HeapGuardContext->TargetMemoryType = Index;

    TestNameSize = sizeof(CHAR8) * (1 + AsciiStrnLenS(NameStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName = AllocateZeroPool(TestNameSize);

    TestDescriptionSize = sizeof(CHAR8) * (1 + AsciiStrnLenS(DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription = (CHAR8 *)AllocateZeroPool(TestDescriptionSize);

    if (TestName != NULL && TestDescription != NULL && HeapGuardContext != NULL) {
      //
      // Name of the test is Security.PageGuard.Uefi + Memory Type Name (from MEMORY_TYPES)
      //
      AsciiStrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      AsciiStrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      //
      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)

      AsciiStrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      AsciiStrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      AddTestCase( TestSuite, TestDescription, TestName, UefiNxProtection, UefiPageGuardPreReq, NULL, HeapGuardContext);

      FreePool(TestName);
      FreePool(TestDescription);
    }
    else
    {
      DEBUG((DEBUG_ERROR, "%a allocating memory for test creation failed.\n", __FUNCTION__));
      return;
    }
  }
} // AddUefiNxTest()


VOID
AddUefiPoolTest(
  UNIT_TEST_SUITE_HANDLE     TestSuite
  )
{
  HEAP_GUARD_TEST_CONTEXT *HeapGuardContext = NULL;
  UINT8 Index;
  CHAR8 NameStub[] = "Security.PoolGuard.Uefi";
  CHAR8 DescriptionStub[] = "Accesses before/after the pool should hit a guard page. Memory type: ";
  CHAR8 *TestName = NULL;
  CHAR8 *TestDescription = NULL;
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
    HeapGuardContext =  (HEAP_GUARD_TEST_CONTEXT *)AllocateZeroPool(sizeof(HEAP_GUARD_TEST_CONTEXT));
    HeapGuardContext->TargetMemoryType = Index;

    //
    // Name of the test is Security.PoolGuard.Uefi + Memory Type Name (from MEMORY_TYPES)
    //
    TestNameSize = sizeof(CHAR8) * (1 + AsciiStrnLenS(NameStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName = (CHAR8 *)AllocateZeroPool(TestNameSize);

    TestDescriptionSize = sizeof(CHAR8) * (1 + AsciiStrnLenS(DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription = (CHAR8 *)AllocateZeroPool(TestDescriptionSize);

    if (TestName != NULL && TestDescription != NULL && HeapGuardContext != NULL) {
      AsciiStrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      AsciiStrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      //
      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)

      AsciiStrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      AsciiStrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      AddTestCase( TestSuite, TestDescription, TestName, UefiPoolGuard, UefiPoolGuardPreReq, NULL, HeapGuardContext);

      FreePool(TestName);
      FreePool(TestDescription);
    }
    else
    {
      DEBUG((DEBUG_ERROR, "%a allocating memory for test creation failed.\n", __FUNCTION__));
      return;
    }
  }
} // AddUefiPoolTest()


VOID
AddUefiPageTest(
  UNIT_TEST_SUITE_HANDLE     TestSuite
  )
{
  HEAP_GUARD_TEST_CONTEXT *HeapGuardContext = NULL;
  UINT8 Index;
  CHAR8 NameStub[] = "Security.PageGuard.Uefi";
  CHAR8 DescriptionStub[] = "Accesses before and after an allocated page should hit a guard page. Memory type: ";
  CHAR8 *TestName = NULL;
  CHAR8 *TestDescription = NULL;
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
    HeapGuardContext =  (HEAP_GUARD_TEST_CONTEXT *)AllocateZeroPool(sizeof(HEAP_GUARD_TEST_CONTEXT));
    HeapGuardContext->TargetMemoryType = Index;

    TestNameSize = sizeof(CHAR8) * (1 + AsciiStrnLenS(NameStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName = (CHAR8 *)AllocateZeroPool(TestNameSize);

    TestDescriptionSize = sizeof(CHAR8) * (1 + AsciiStrnLenS(DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription = (CHAR8 *)AllocateZeroPool(TestDescriptionSize);

    if (TestName != NULL && TestDescription != NULL && HeapGuardContext != NULL) {
      //
      // Name of the test is Security.PageGuard.Uefi + Memory Type Name (from MEMORY_TYPES)
      //
      AsciiStrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      AsciiStrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      //
      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)

      AsciiStrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      AsciiStrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      AddTestCase( TestSuite, TestDescription, TestName, UefiPageGuard, UefiPageGuardPreReq, NULL, HeapGuardContext);

      FreePool(TestName);
      FreePool(TestDescription);
    }
    else
    {
      DEBUG((DEBUG_ERROR, "%a allocating memory for test creation failed.\n", __FUNCTION__));
      return;
    }
  }
} // AddUefiPageTest()


VOID
AddSmmPoolTest(
  UNIT_TEST_SUITE_HANDLE     TestSuite
  )
{
  HEAP_GUARD_TEST_CONTEXT *HeapGuardContext = NULL;
  UINT8 Index;
  CHAR8 NameStub[] = "Security.PoolGuard.Smm";
  CHAR8 DescriptionStub[] = "Accesses before/after the pool should hit a guard page in SMM. Memory type: ";
  CHAR8 *TestName = NULL;
  CHAR8 *TestDescription = NULL;
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
    HeapGuardContext =  (HEAP_GUARD_TEST_CONTEXT *)AllocateZeroPool(sizeof(HEAP_GUARD_TEST_CONTEXT));
    HeapGuardContext->TargetMemoryType = Index;

    TestNameSize = sizeof(CHAR8) * (1 + AsciiStrnLenS(NameStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName = (CHAR8 *)AllocateZeroPool(TestNameSize);

    TestDescriptionSize = sizeof(CHAR8) * (1 + AsciiStrnLenS(DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription = (CHAR8 *)AllocateZeroPool(TestDescriptionSize);

    if (TestName != NULL && TestDescription != NULL && HeapGuardContext != NULL) {
      //
      // Name of the test is Security.PoolGuard.Smm + Memory Type Name (from MEMORY_TYPES)
      //
      AsciiStrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      AsciiStrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      //
      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)

      AsciiStrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      AsciiStrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      AddTestCase( TestSuite, TestDescription, TestName, SmmPoolGuard, SmmPoolGuardPreReq, NULL, HeapGuardContext);

      FreePool(TestName);
      FreePool(TestDescription);
    }
    else
    {
      DEBUG((DEBUG_ERROR, "%a allocating memory for test creation failed.\n", __FUNCTION__));
      return;
    }
  }
} // AddSmmPoolTest()


VOID
AddSmmPageTest(
  UNIT_TEST_SUITE_HANDLE     TestSuite
  )
{
  HEAP_GUARD_TEST_CONTEXT *HeapGuardContext = NULL;
  UINT8 Index;
  CHAR8 NameStub[] = "Security.PageGuard.Smm";
  CHAR8 DescriptionStub[] = "Accesses before and after an allocated page should hit a guard page in SMM. Memory type: ";
  CHAR8 *TestName = NULL;
  CHAR8 *TestDescription = NULL;
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
    HeapGuardContext =  (HEAP_GUARD_TEST_CONTEXT *)AllocateZeroPool(sizeof(HEAP_GUARD_TEST_CONTEXT));
    HeapGuardContext->TargetMemoryType = Index;

    TestNameSize = sizeof(CHAR8) * (1 + AsciiStrnLenS(NameStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName = (CHAR8 *)AllocateZeroPool(TestNameSize);

    TestDescriptionSize = sizeof(CHAR8) * (1 + AsciiStrnLenS(DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS(MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription = (CHAR8 *)AllocateZeroPool(TestDescriptionSize);

    if (TestName != NULL && TestDescription != NULL && HeapGuardContext != NULL) {
      //
      // Name of the test is Security.PageGuard.Smm + Memory Type Name (from MEMORY_TYPES)
      //
      AsciiStrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      AsciiStrCatS(TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      //
      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)

      AsciiStrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      AsciiStrCatS(TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      AddTestCase( TestSuite, TestDescription, TestName, SmmPageGuard, SmmPageGuardPreReq, NULL, HeapGuardContext);

      FreePool(TestName);
      FreePool(TestDescription);
    }
    else
    {
      DEBUG((DEBUG_ERROR, "%a allocating memory for test creation failed.\n", __FUNCTION__));
      return;
    }
  }
} // AddSmmPageTest()


/**
  HeapGuardTestAppEntryPoint

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
HeapGuardTestAppEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Fw = NULL;
  UNIT_TEST_SUITE_HANDLE      PageGuard = NULL;
  UNIT_TEST_SUITE_HANDLE      PoolGuard = NULL;
  UNIT_TEST_SUITE_HANDLE      NxProtection = NULL;
  UNIT_TEST_SUITE_HANDLE      Misc = NULL;
  HEAP_GUARD_TEST_CONTEXT     *HeapGuardContext;
  HeapGuardContext = (HEAP_GUARD_TEST_CONTEXT *)AllocateZeroPool(sizeof(HEAP_GUARD_TEST_CONTEXT));

  DEBUG((DEBUG_ERROR, "%a()\n", __FUNCTION__));

  DEBUG(( DEBUG_ERROR, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

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
  Status = InitUnitTestFramework( &Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Create separate test suites for Page, Pool, NX tests.
  // Misc test suite for stack guard and null pointer.
  //
  CreateUnitTestSuite( &Misc, Fw, "Stack Guard and Null Pointer Detection", "Security.HeapGuardMisc", NULL, NULL);
  CreateUnitTestSuite( &PageGuard, Fw, "Page Guard Tests", "Security.PageGuard", NULL, NULL);
  CreateUnitTestSuite( &PoolGuard, Fw, "Pool Guard Tests", "Security.PoolGuard", NULL, NULL);
  CreateUnitTestSuite( &NxProtection, Fw, "NX Protection Tests", "Security.NxProtection", NULL, NULL);

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

  AddTestCase( Misc, "Null pointer access should trigger a page fault", "Security.HeapGuardMisc.UefiNullPointerDetection", UefiNullPointerDetection, UefiNullPointerPreReq, NULL, HeapGuardContext );
  AddTestCase( Misc, "Null pointer access in SMM should trigger a page fault", "Security.HeapGuardMisc.SmmNullPointerDetection", SmmNullPointerDetection, SmmNullPointerPreReq, NULL, HeapGuardContext );
  AddTestCase( Misc, "Blowing the stack should trigger a page fault", "Security.HeapGuardMisc.UefiCpuStackGuard", UefiCpuStackGuard, UefiStackGuardPreReq, NULL, HeapGuardContext );
  AddTestCase( NxProtection, "Check hardware configuration of HardwareNxProtection bit", "Security.HeapGuardMisc.UefiHardwareNxProtectionEnabled", UefiHardwareNxProtectionEnabled, UefiHardwareNxProtectionEnabledPreReq, NULL, HeapGuardContext );
  AddTestCase( NxProtection, "Stack NX Protection", "Security.HeapGuardMisc.UefiNxStackGuard", UefiNxStackGuard, UefiNxStackPreReq, NULL, HeapGuardContext );

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
