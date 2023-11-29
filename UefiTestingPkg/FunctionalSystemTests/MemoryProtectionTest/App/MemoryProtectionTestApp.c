/** @file -- MemoryProtectionTestApp.c

Tests for page guard, pool guard, NX protections, stack guard, and null pointer detection.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Protocol/Cpu.h>
#include <Protocol/SmmCommunication.h>
#include <Protocol/MemoryProtectionNonstopMode.h>
#include <Protocol/MemoryProtectionDebug.h>
#include <Protocol/MemoryAttribute.h>
#include <Protocol/CpuMpDebug.h>
#include <Protocol/ShellParameters.h>
#include <Uefi/UefiMultiPhase.h>
#include <Pi/PiMultiPhase.h>

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
#include <Register/ArchitecturalMsr.h>
#include <Library/ResetSystemLib.h>
#include <Library/HobLib.h>
#include <Library/ExceptionPersistenceLib.h>

#include <Guid/PiSmmCommunicationRegionTable.h>
#include <Guid/DxeMemoryProtectionSettings.h>
#include <Guid/MmMemoryProtectionSettings.h>

#include "../MemoryProtectionTestCommon.h"
#include "UefiHardwareNxProtectionStub.h"

#define UNIT_TEST_APP_NAME                 "Memory Protection Test"
#define UNIT_TEST_APP_VERSION              "2.0"
#define UNIT_TEST_WARM_RESET_STRING        L"--Reset"
#define UNIT_TEST_MEMORY_ATTRIBUTE_STRING  L"--MemoryAttribute"
#define UNIT_TEST_CLEAR_FAULTS_STRING      L"--ClearFaults"

#define DUMMY_FUNCTION_FOR_CODE_SELF_TEST_GENERIC_SIZE  512
#define ALIGN_ADDRESS(Address)  (((Address) / EFI_PAGE_SIZE) * EFI_PAGE_SIZE)

UINTN                                    mPiSmmCommonCommBufferSize;
MM_MEMORY_PROTECTION_SETTINGS            mMmMps;
DXE_MEMORY_PROTECTION_SETTINGS           mDxeMps;
VOID                                     *mPiSmmCommonCommBufferAddress = NULL;
EFI_CPU_ARCH_PROTOCOL                    *mCpu                          = NULL;
MEMORY_PROTECTION_NONSTOP_MODE_PROTOCOL  *mNonstopModeProtocol          = NULL;
MEMORY_PROTECTION_DEBUG_PROTOCOL         *mMemoryProtectionProtocol     = NULL;
EFI_MEMORY_ATTRIBUTE_PROTOCOL            *mMemoryAttributeProtocol      = NULL;
CPU_MP_DEBUG_PROTOCOL                    *mCpuMpDebugProtocol           = NULL;

/// ================================================================================================
/// ================================================================================================
///
/// HELPER FUNCTIONS
///
/// ================================================================================================
/// ================================================================================================

/**
  Gets the input EFI_MEMORY_TYPE from the input DXE_HEAP_GUARD_MEMORY_TYPES bitfield

  @param[in]  MemoryType            Memory type to check.
  @param[in]  HeapGuardMemoryType   DXE_HEAP_GUARD_MEMORY_TYPES bitfield

  @return TRUE  The given EFI_MEMORY_TYPE is TRUE in the given DXE_HEAP_GUARD_MEMORY_TYPES
  @return FALSE The given EFI_MEMORY_TYPE is FALSE in the given DXE_HEAP_GUARD_MEMORY_TYPES
**/
BOOLEAN
STATIC
GetDxeMemoryTypeSettingFromBitfield (
  IN EFI_MEMORY_TYPE              MemoryType,
  IN DXE_HEAP_GUARD_MEMORY_TYPES  HeapGuardMemoryType
  )
{
  switch (MemoryType) {
    case EfiReservedMemoryType:
      return HeapGuardMemoryType.Fields.EfiReservedMemoryType;
    case EfiLoaderCode:
      return HeapGuardMemoryType.Fields.EfiLoaderCode;
    case EfiLoaderData:
      return HeapGuardMemoryType.Fields.EfiLoaderData;
    case EfiBootServicesCode:
      return HeapGuardMemoryType.Fields.EfiBootServicesCode;
    case EfiBootServicesData:
      return HeapGuardMemoryType.Fields.EfiBootServicesData;
    case EfiRuntimeServicesCode:
      return HeapGuardMemoryType.Fields.EfiRuntimeServicesCode;
    case EfiRuntimeServicesData:
      return HeapGuardMemoryType.Fields.EfiRuntimeServicesData;
    case EfiConventionalMemory:
      return HeapGuardMemoryType.Fields.EfiConventionalMemory;
    case EfiUnusableMemory:
      return HeapGuardMemoryType.Fields.EfiUnusableMemory;
    case EfiACPIReclaimMemory:
      return HeapGuardMemoryType.Fields.EfiACPIReclaimMemory;
    case EfiACPIMemoryNVS:
      return HeapGuardMemoryType.Fields.EfiACPIMemoryNVS;
    case EfiMemoryMappedIO:
      return HeapGuardMemoryType.Fields.EfiMemoryMappedIO;
    case EfiMemoryMappedIOPortSpace:
      return HeapGuardMemoryType.Fields.EfiMemoryMappedIOPortSpace;
    case EfiPalCode:
      return HeapGuardMemoryType.Fields.EfiPalCode;
    case EfiPersistentMemory:
      return HeapGuardMemoryType.Fields.EfiPersistentMemory;
    case EfiUnacceptedMemoryType:
      return HeapGuardMemoryType.Fields.EfiUnacceptedMemoryType;
    default:
      return FALSE;
  }
}

/**
  Gets the input EFI_MEMORY_TYPE from the input MM_HEAP_GUARD_MEMORY_TYPES bitfield

  @param[in]  MemoryType            Memory type to check.
  @param[in]  HeapGuardMemoryType   MM_HEAP_GUARD_MEMORY_TYPES bitfield

  @return TRUE  The given EFI_MEMORY_TYPE is TRUE in the given MM_HEAP_GUARD_MEMORY_TYPES
  @return FALSE The given EFI_MEMORY_TYPE is FALSE in the given MM_HEAP_GUARD_MEMORY_TYPES
**/
BOOLEAN
STATIC
GetMmMemoryTypeSettingFromBitfield (
  IN EFI_MEMORY_TYPE             MemoryType,
  IN MM_HEAP_GUARD_MEMORY_TYPES  HeapGuardMemoryType
  )
{
  switch (MemoryType) {
    case EfiReservedMemoryType:
      return HeapGuardMemoryType.Fields.EfiReservedMemoryType;
    case EfiLoaderCode:
      return HeapGuardMemoryType.Fields.EfiLoaderCode;
    case EfiLoaderData:
      return HeapGuardMemoryType.Fields.EfiLoaderData;
    case EfiBootServicesCode:
      return HeapGuardMemoryType.Fields.EfiBootServicesCode;
    case EfiBootServicesData:
      return HeapGuardMemoryType.Fields.EfiBootServicesData;
    case EfiRuntimeServicesCode:
      return HeapGuardMemoryType.Fields.EfiRuntimeServicesCode;
    case EfiRuntimeServicesData:
      return HeapGuardMemoryType.Fields.EfiRuntimeServicesData;
    case EfiConventionalMemory:
      return HeapGuardMemoryType.Fields.EfiConventionalMemory;
    case EfiUnusableMemory:
      return HeapGuardMemoryType.Fields.EfiUnusableMemory;
    case EfiACPIReclaimMemory:
      return HeapGuardMemoryType.Fields.EfiACPIReclaimMemory;
    case EfiACPIMemoryNVS:
      return HeapGuardMemoryType.Fields.EfiACPIMemoryNVS;
    case EfiMemoryMappedIO:
      return HeapGuardMemoryType.Fields.EfiMemoryMappedIO;
    case EfiMemoryMappedIOPortSpace:
      return HeapGuardMemoryType.Fields.EfiMemoryMappedIOPortSpace;
    case EfiPalCode:
      return HeapGuardMemoryType.Fields.EfiPalCode;
    case EfiPersistentMemory:
      return HeapGuardMemoryType.Fields.EfiPersistentMemory;
    case EfiUnacceptedMemoryType:
      return HeapGuardMemoryType.Fields.EfiUnacceptedMemoryType;
    default:
      return FALSE;
  }
}

/**
  Abstraction layer which fetches the MM memory protection HOB.

  @retval EFI_SUCCESS   The HOB entry has been fetched
  @retval EFI_INVALID_PARAMETER The HOB entry could not be found
**/
EFI_STATUS
STATIC
FetchMemoryProtectionHobEntries (
  VOID
  )
{
  EFI_STATUS  Status = EFI_INVALID_PARAMETER;
  VOID        *Ptr1;
  VOID        *Ptr2;

  ZeroMem (&mMmMps, sizeof (mMmMps));
  ZeroMem (&mDxeMps, sizeof (mDxeMps));

  Ptr1 = GetFirstGuidHob (&gMmMemoryProtectionSettingsGuid);
  Ptr2 = GetFirstGuidHob (&gDxeMemoryProtectionSettingsGuid);

  if (Ptr1 != NULL) {
    if (*((UINT8 *)GET_GUID_HOB_DATA (Ptr1)) != (UINT8)MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION) {
      DEBUG ((
        DEBUG_INFO,
        "%a: - Version number of the MM Memory Protection Settings HOB is invalid.\n",
        __FUNCTION__
        ));
    } else {
      Status = EFI_SUCCESS;
      CopyMem (&mMmMps, GET_GUID_HOB_DATA (Ptr1), sizeof (MM_MEMORY_PROTECTION_SETTINGS));
    }
  }

  if (Ptr2 != NULL) {
    if (*((UINT8 *)GET_GUID_HOB_DATA (Ptr2)) != (UINT8)DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION) {
      DEBUG ((
        DEBUG_INFO,
        "%a: - Version number of the DXE Memory Protection Settings HOB is invalid.\n",
        __FUNCTION__
        ));
    } else {
      Status = EFI_SUCCESS;
      CopyMem (&mDxeMps, GET_GUID_HOB_DATA (Ptr2), sizeof (DXE_MEMORY_PROTECTION_SETTINGS));
    }
  }

  return Status;
}

/**
  Populates the heap guard protocol global

  @retval EFI_SUCCESS Protocol is already populated or was successfully populated
  @retval other       Return value of LocateProtocol
**/
STATIC
EFI_STATUS
PopulateMemoryProtectionDebugProtocol (
  VOID
  )
{
  if (mMemoryProtectionProtocol != NULL) {
    return EFI_SUCCESS;
  }

  return gBS->LocateProtocol (&gMemoryProtectionDebugProtocolGuid, NULL, (VOID **)&mMemoryProtectionProtocol);
}

/**
  Populates the memory attribute protocol

  @retval EFI_SUCCESS Protocol is already populated or was successfully populated
  @retval other       Return value of LocateProtocol
**/
STATIC
EFI_STATUS
PopulateMemoryAttributeProtocol (
  VOID
  )
{
  if (mMemoryAttributeProtocol != NULL) {
    return EFI_SUCCESS;
  }

  return gBS->LocateProtocol (&gEfiMemoryAttributeProtocolGuid, NULL, (VOID **)&mMemoryAttributeProtocol);
}

/**
  Populates the CPU MP debug protocol global

  @retval EFI_SUCCESS Protocol is already populated or was successfully populated
  @retval other       Return value of LocateProtocol
**/
STATIC
EFI_STATUS
PopulateCpuMpDebugProtocol (
  VOID
  )
{
  if (mCpuMpDebugProtocol != NULL) {
    return EFI_SUCCESS;
  }

  return gBS->LocateProtocol (&gCpuMpDebugProtocolGuid, NULL, (VOID **)&mCpuMpDebugProtocol);
}

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
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  // Avoid using runtime services to reset the system because doing so will raise the TPL level
  // when it is already on TPL_HIGH. HwResetSystemLib is used here instead to perform a bare-metal reset
  // and sidestep this issue.
  ResetWarm ();
}

/**
  This helper function returns EFI_SUCCESS if the Nonstop protocol is installed.

  @retval     EFI_SUCCESS         Nonstop protocol installed
  @retval     other               retval of LocateProtocol()
**/
STATIC
EFI_STATUS
GetNonstopProtocol (
  VOID
  )
{
  if (mNonstopModeProtocol != NULL) {
    return EFI_SUCCESS;
  }

  return gBS->LocateProtocol (&gMemoryProtectionNonstopModeProtocolGuid, NULL, (VOID **)&mNonstopModeProtocol);
}

/**
  This helper function returns EFI_SUCCESS if the memory protection exception handler is installed.

  @retval     EFI_SUCCESS         Memory protection exception handler installed
  @retval     other               retval of LocateProtocol()
**/
STATIC
EFI_STATUS
CheckMemoryProtectionExceptionHandlerInstallation (
  VOID
  )
{
  VOID  *DummyProtocol = NULL;

  return gBS->LocateProtocol (&gMemoryProtectionExceptionHandlerGuid, NULL, (VOID **)&DummyProtocol);
}

/**
  This helper function returns TRUE if the MemProtExGetIgnoreNextException() returns TRUE.

  @retval     TRUE                IgnoreNextException set
  @retval     FALSE               Otherwise
**/
STATIC
BOOLEAN
GetIgnoreNextEx (
  VOID
  )
{
  BOOLEAN  Result = FALSE;

  ExPersistGetIgnoreNextPageFault (&Result);

  return Result;
}

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
  IN  UINT16                          RequestedFunction,
  IN  MEMORY_PROTECTION_TEST_CONTEXT  *Context
  )
{
  EFI_STATUS                             Status = EFI_SUCCESS;
  EFI_SMM_COMMUNICATE_HEADER             *CommHeader;
  MEMORY_PROTECTION_TEST_COMM_BUFFER     *VerificationCommBuffer;
  static EFI_SMM_COMMUNICATION_PROTOCOL  *SmmCommunication = NULL;
  UINTN                                  CommBufferSize;

  if (mPiSmmCommonCommBufferAddress == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Communication buffer not found!\n", __FUNCTION__));
    return EFI_ABORTED;
  }

  // Zero the comm buffer
  CommHeader     = (EFI_SMM_COMMUNICATE_HEADER *)mPiSmmCommonCommBufferAddress;
  CommBufferSize = sizeof (MEMORY_PROTECTION_TEST_COMM_BUFFER) + OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data);
  if (CommBufferSize > mPiSmmCommonCommBufferSize) {
    DEBUG ((DEBUG_ERROR, "%a - Communication buffer is too small!\n", __FUNCTION__));
    return EFI_ABORTED;
  }

  ZeroMem (CommHeader, CommBufferSize);

  // Update the SMM communication parameters
  CopyGuid (&CommHeader->HeaderGuid, &gMemoryProtectionTestSmiHandlerGuid);
  CommHeader->MessageLength = sizeof (MEMORY_PROTECTION_TEST_COMM_BUFFER);

  // Update parameters Specific to this implementation
  VerificationCommBuffer           = (MEMORY_PROTECTION_TEST_COMM_BUFFER *)CommHeader->Data;
  VerificationCommBuffer->Function = RequestedFunction;
  VerificationCommBuffer->Status   = EFI_NOT_FOUND;
  CopyMem (&VerificationCommBuffer->Context, Context, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

  // Locate the protocol if necessary
  if (!SmmCommunication) {
    Status = gBS->LocateProtocol (&gEfiSmmCommunicationProtocolGuid, NULL, (VOID **)&SmmCommunication);
  }

  // Signal MM
  if (!EFI_ERROR (Status)) {
    Status = SmmCommunication->Communicate (
                                 SmmCommunication,
                                 CommHeader,
                                 &CommBufferSize
                                 );
    DEBUG ((DEBUG_VERBOSE, "%a - Communicate() = %r\n", __FUNCTION__, Status));
  }

  return VerificationCommBuffer->Status;
}

/**
  Locates and stores address of comm buffer
**/
STATIC
VOID
LocateSmmCommonCommBuffer (
  VOID
  )
{
  EFI_STATUS                               Status;
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE  *PiSmmCommunicationRegionTable;
  EFI_MEMORY_DESCRIPTOR                    *SmmCommMemRegion;
  UINTN                                    Index, BufferSize;

  if (mPiSmmCommonCommBufferAddress == NULL) {
    Status = EfiGetSystemConfigurationTable (&gEdkiiPiSmmCommunicationRegionTableGuid, (VOID **)&PiSmmCommunicationRegionTable);
    if (EFI_ERROR (Status)) {
      return;
    }

    // Only need a region large enough to hold a MEMORY_PROTECTION_TEST_COMM_BUFFER
    BufferSize       = 0;
    SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR *)(PiSmmCommunicationRegionTable + 1);
    for (Index = 0; Index < PiSmmCommunicationRegionTable->NumberOfEntries; Index++) {
      if (SmmCommMemRegion->Type == EfiConventionalMemory) {
        BufferSize = EFI_PAGES_TO_SIZE ((UINTN)SmmCommMemRegion->NumberOfPages);
        if (BufferSize >= (sizeof (MEMORY_PROTECTION_TEST_COMM_BUFFER) + OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data))) {
          break;
        }
      }

      SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)SmmCommMemRegion + PiSmmCommunicationRegionTable->DescriptorSize);
    }

    if (SmmCommMemRegion->PhysicalStart > MAX_UINTN) {
      return;
    }

    mPiSmmCommonCommBufferAddress = (VOID *)(UINTN)SmmCommMemRegion->PhysicalStart;
    mPiSmmCommonCommBufferSize    = BufferSize;
  }
}

/**
  The recursion loop for testing stack overflow protection. This function will
  recurse until it overflows the stack at which point it's expected that a switch
  stack is used and an interrupt is generated.

  @param[in]  Count   The current recursion depth.

  @retval             The sum of Count and the return value of the next recursive call.
**/
STATIC
UINT64
Recursion (
  IN UINT64  Count
  )
{
  UINT64            Sum            = 0;
  volatile BOOLEAN  AlwaysTrueBool = TRUE;

  DEBUG ((DEBUG_INFO, "%a - %x\n", __FUNCTION__, Count));
  // This code is meant to be an infinite recursion to trip a page fault. Some compilers will catch
  // infinite recursions, so to sidestep those warnings, we block the next recursive call behind
  // a boolean check.
  if (AlwaysTrueBool) {
    Sum = Recursion (++Count);
  }

  return Sum + Count;
}

/**
  A recursive stack overflow function which at every recursion level checks if the interrupt handler
  has signaled that it ran and cleared the faulting region at which point we unwind the recursion.

  @param[in]  Count   The current recursion depth.

  @retval             The sum of Count and the return value of the next recursive call.
**/
STATIC
UINT64
RecursionDynamic (
  IN UINT64  Count
  )
{
  UINT64  Sum = 0;

  DEBUG ((DEBUG_ERROR, "%a - 0x%x\n", __FUNCTION__, Count));

  if (GetIgnoreNextEx ()) {
    Sum = RecursionDynamic (++Count);
  }

  return Sum + Count;
}

/**
  Tests the pool guards by allocating a pool and then writing to the guard page. If the testing
  method is via reset, this function is expected to fault and reset the system. If the testing
  method is via exception clearing, it's expected that this function will return and the value
  of ExPersistGetIgnoreNextPageFault() will be FALSE.

  @param[in]  ptr               The pointer to the pool.
  @param[in]  AllocationSize    The size of the pool.
**/
VOID
PoolTest (
  IN UINT64  *ptr,
  IN UINT64  AllocationSize
  )
{
  UINT64  *ptrLoc;

  DEBUG ((DEBUG_INFO, "%a - Allocated pool at 0x%p\n", __FUNCTION__, ptr));

  // Check if guard page is going to be at the head or tail
  if (mDxeMps.HeapGuardPolicy.Fields.Direction == HEAP_GUARD_ALIGNED_TO_TAIL) {
    // Get to the beginning of the page containing the pool
    ptrLoc = (UINT64 *)(((UINTN)ptr) + (UINTN)AllocationSize);
    ptrLoc = ALIGN_POINTER (ptrLoc, 0x1000);

    // The guard page will be on the next page
    ptr = (UINT64 *)(((UINTN)ptr) + 0x1000);
  } else {
    // Get to the beginning of the page containing the pool
    ptrLoc = ALIGN_POINTER (ptr, 0x1000);

    // The guard page will be immediately preceding the page the pool starts on.
    ptrLoc = (UINT64 *)(((UINTN)ptrLoc) - 0x1);
  }

  DEBUG ((DEBUG_ERROR, "%a - Writing to 0x%p\n", __FUNCTION__, ptrLoc));
  *ptrLoc = 1;
}

/**
  Test the head guard page of the input pointer by writing to the page immediately preceding it. The input
  pointer is expected to be page aligned so subtracting 1 from will get the page immediately preceding it.

  @param[in]  ptr   The pointer to the page expected to have a guard page immediately preceding it.
**/
VOID
HeadPageTest (
  IN UINT64  *ptr
  )
{
  // Write to the head guard page
  ptr = (UINT64 *)(((UINTN)ptr) - 0x1);
  DEBUG ((DEBUG_ERROR, "%a - Writing to 0x%p\n", __FUNCTION__, ptr));
  *ptr = 1;
}

/**
  Test the tail guard page of the input pointer by writing to the page immediately following it. The input
  pointer can be anywhere within the page with a guard page immediately following it.

  @param[in]  ptr   The pointer to the page expected to have a guard page immediately following it.
**/
VOID
TailPageTest (
  IN UINT64  *ptr
  )
{
  // Write to the tail guard page
  ptr = (UINT64 *)(((UINTN)ptr) + 0x1000);
  DEBUG ((DEBUG_ERROR, "%a - Writing to 0x%p\n", __FUNCTION__, ptr));
  *ptr = 1;
}

/**
  This dummy function definition is used to test no-execute protection on allocated buffers
  and the stack.
**/
typedef
VOID
(*DUMMY_VOID_FUNCTION_FOR_DATA_TEST)(
  VOID
  );

/**
  This function serves as a placeholder in the driver code region.
  This function address will be written to by the SmmMemoryProtectionsSelfTestCode()
  test.
**/
STATIC
VOID
DummyFunctionForCodeSelfTest (
  VOID
  )
{
  volatile UINT8  DontCompileMeOut = 0;

  DontCompileMeOut++;
  return;
}

/// ================================================================================================
/// ================================================================================================
///
/// PRE REQ FUNCTIONS
///
/// ================================================================================================
/// ================================================================================================

/**
  Checks if any NX protection policy is active.

  @param[in]  Context           The context of the test.

  @retval UNIT_TEST_PASSED    NX protection is active.
  @retval UNIT_TEST_SKIPPED   NX protection is not active.
**/
UNIT_TEST_STATUS
EFIAPI
UefiHardwareNxProtectionEnabledPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  if (mDxeMps.NxProtectionPolicy.Data) {
    return UNIT_TEST_PASSED;
  }

  return UNIT_TEST_SKIPPED;
}

/**
  Image protection testing currently requires the Memory Attribute Protocol and the Memory Protection
  Debug Protocol to be present. The Memory Protection Debug protocol allows us to retrieve a list of
  currently protected images and the Memory Attribute Protocol allows us to check the memory attributes.

  @param[in]  Context           The context of the test.

  @retval UNIT_TEST_PASSED    The pre-reqs for image protection testing are met.
  @retval UNIT_TEST_SKIPPED   The pre-reqs for image protection testing are not met.
**/
UNIT_TEST_STATUS
EFIAPI
ImageProtectionPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  if (((mDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromFv != 0) || (mDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromUnknown != 0)) &&
      (!EFI_ERROR (PopulateMemoryAttributeProtocol ())) &&
      (!EFI_ERROR (PopulateMemoryProtectionDebugProtocol ())))
  {
    return UNIT_TEST_PASSED;
  }

  return UNIT_TEST_SKIPPED;
}

/**
  This function checks if the NX protection policy is active for the target memory type within Context. Testing
  NX protection currently requires that buffers of the relevant memory type can be allocated. If the memory type
  is Conventional or Persistent, the test will be skipped as we cannot allocate those types.

  The function also checks if hardware enforced NX protection is active. If it is not, the test will be skipped.

  @param[in]  Context           The context of the test.

  @retval UNIT_TEST_PASSED    The pre-reqs for image protection testing are met.
  @retval UNIT_TEST_SKIPPED   The pre-reqs for image protection testing are not met.
**/
UNIT_TEST_STATUS
EFIAPI
UefiNxProtectionPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);

  UT_ASSERT_TRUE (MemoryProtectionContext.TargetMemoryType < EfiMaxMemoryType);
  if (!GetDxeMemoryTypeSettingFromBitfield ((EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, mDxeMps.NxProtectionPolicy)) {
    UT_LOG_WARNING ("Protection for this memory type is disabled: %a", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  // Skip memory types which cannot be allocated
  if ((MemoryProtectionContext.TargetMemoryType == EfiConventionalMemory) ||
      (MemoryProtectionContext.TargetMemoryType == EfiPersistentMemory)   ||
      (MemoryProtectionContext.TargetMemoryType == EfiUnacceptedMemoryType))
  {
    UT_LOG_WARNING ("Skipping test of memory type %a -- memory type cannot be allocated", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  // Ensure no-execute protection is possible
  if (UefiHardwareNxProtectionEnabled (Context) != UNIT_TEST_PASSED) {
    UT_LOG_WARNING ("HardwareNxProtection bit not on. NX Test would not be accurate.");
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
}

/**
  This function checks if the page protection policy is active for the target memory type within Context. Testing
  page guards currently requires that buffers of the relevant memory type can be allocated. If the memory type
  is Conventional or Persistent, the test will be skipped as we cannot allocate those types.

  @param[in]  Context           The context of the test.

  @retval UNIT_TEST_PASSED    The pre-reqs for page guard testing are met.
  @retval UNIT_TEST_SKIPPED   The pre-reqs for page guard testing are not met.
**/
UNIT_TEST_STATUS
EFIAPI
UefiPageGuardPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);

  UT_ASSERT_TRUE (MemoryProtectionContext.TargetMemoryType < EfiMaxMemoryType);
  if (!(mDxeMps.HeapGuardPolicy.Fields.UefiPageGuard &&
        GetDxeMemoryTypeSettingFromBitfield ((EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, mDxeMps.HeapGuardPageType)))
  {
    UT_LOG_WARNING ("Protection for this memory type is disabled: %a", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  // Skip memory types which cannot be allocated
  if ((MemoryProtectionContext.TargetMemoryType == EfiConventionalMemory) ||
      (MemoryProtectionContext.TargetMemoryType == EfiPersistentMemory)   ||
      (MemoryProtectionContext.TargetMemoryType == EfiUnacceptedMemoryType))
  {
    UT_LOG_WARNING ("Skipping test of memory type %a -- memory type cannot be allocated", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
}

/**
  This function checks if the pool guard policy is active for the target memory type within Context. Testing
  pool guards currently requires that buffers of the relevant memory type can be allocated. If the memory type
  is Conventional or Persistent, the test will be skipped as we cannot allocate those types.

  @param[in]  Context           The context of the test.

  @retval UNIT_TEST_PASSED    The pre-reqs for pool guard testing are met.
  @retval UNIT_TEST_SKIPPED   The pre-reqs for pool guard testing are not met.
**/
UNIT_TEST_STATUS
EFIAPI
UefiPoolGuardPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);

  UT_ASSERT_TRUE (MemoryProtectionContext.TargetMemoryType < EfiMaxMemoryType);
  if (!(mDxeMps.HeapGuardPolicy.Fields.UefiPoolGuard &&
        GetDxeMemoryTypeSettingFromBitfield ((EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, mDxeMps.HeapGuardPoolType)))
  {
    UT_LOG_WARNING ("Protection for this memory type is disabled: %a", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  // Skip memory types which cannot be allocated
  if ((MemoryProtectionContext.TargetMemoryType == EfiConventionalMemory) ||
      (MemoryProtectionContext.TargetMemoryType == EfiPersistentMemory)   ||
      (MemoryProtectionContext.TargetMemoryType == EfiUnacceptedMemoryType))
  {
    UT_LOG_WARNING ("Skipping test of memory type %a -- memory type cannot be allocated", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
}

/**
  This function checks if the stack guard policy is active.

  @param[in]  Context           The context of the test.

  @retval UNIT_TEST_PASSED    The pre-reqs for stack guard testing are met.
  @retval UNIT_TEST_SKIPPED   The pre-reqs for stack guard testing are not met.
**/
UNIT_TEST_STATUS
EFIAPI
UefiStackGuardPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  if (!mDxeMps.CpuStackGuard) {
    UT_LOG_WARNING ("This feature is disabled");
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
}

/**
  This function checks if the NULL pointer detection policy is active.

  @param[in]  Context           The context of the test.

  @retval UNIT_TEST_PASSED    The pre-reqs for NULL pointer detection testing are met.
  @retval UNIT_TEST_SKIPPED   The pre-reqs for NULL pointer detection testing are not met.
**/
UNIT_TEST_STATUS
EFIAPI
UefiNullPointerPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  if (!mDxeMps.NullPointerDetectionPolicy.Fields.UefiNullDetection) {
    UT_LOG_WARNING ("This feature is disabled");
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
}

/**
  This function checks if the MM page guard policy is active for the target memory type within Context. Testing
  page guards currently requires that buffers of the relevant memory type can be allocated. If the memory type
  is Conventional or Persistent, the test will be skipped as we cannot allocate those types.

  @param[in]  Context           The context of the test.

  @retval UNIT_TEST_PASSED    The pre-reqs for page guard testing are met.
  @retval UNIT_TEST_SKIPPED   The pre-reqs for page guard testing are not met.
**/
UNIT_TEST_STATUS
EFIAPI
SmmPageGuardPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);

  UT_ASSERT_TRUE (MemoryProtectionContext.TargetMemoryType < EfiMaxMemoryType);
  if (!(mMmMps.HeapGuardPolicy.Fields.MmPageGuard &&
        GetMmMemoryTypeSettingFromBitfield ((EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, mMmMps.HeapGuardPageType)))
  {
    UT_LOG_WARNING ("Protection for this memory type is disabled: %a", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  // Skip memory types which cannot be allocated
  if ((MemoryProtectionContext.TargetMemoryType == EfiConventionalMemory) ||
      (MemoryProtectionContext.TargetMemoryType == EfiPersistentMemory)   ||
      (MemoryProtectionContext.TargetMemoryType == EfiUnacceptedMemoryType))
  {
    UT_LOG_WARNING ("Skipping test of memory type %a -- memory type cannot be allocated", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
}

/**
  This function checks if the MM pool guard policy is active for the target memory type within Context. Testing
  pool guards currently requires that buffers of the relevant memory type can be allocated. If the memory type
  is Conventional or Persistent, the test will be skipped as we cannot allocate those types.

  @param[in]  Context           The context of the test.

  @retval UNIT_TEST_PASSED    The pre-reqs for pool guard testing are met.
  @retval UNIT_TEST_SKIPPED   The pre-reqs for pool guard testing are not met.
**/
UNIT_TEST_STATUS
EFIAPI
SmmPoolGuardPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);

  UT_ASSERT_TRUE (MemoryProtectionContext.TargetMemoryType < EfiMaxMemoryType);
  if (!(mMmMps.HeapGuardPolicy.Fields.MmPoolGuard &&
        GetMmMemoryTypeSettingFromBitfield ((EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, mMmMps.HeapGuardPoolType)))
  {
    UT_LOG_WARNING ("Protection for this memory type is disabled: %a", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  // Skip memory types which cannot be allocated
  if ((MemoryProtectionContext.TargetMemoryType == EfiConventionalMemory) ||
      (MemoryProtectionContext.TargetMemoryType == EfiPersistentMemory)   ||
      (MemoryProtectionContext.TargetMemoryType == EfiUnacceptedMemoryType))
  {
    UT_LOG_WARNING ("Skipping test of memory type %a -- memory type cannot be allocated", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
}

/**
  This function checks if the NULL pointer detection policy for MM is active.

  @param[in]  Context           The context of the test.

  @retval UNIT_TEST_PASSED    The pre-reqs for NULL pointer testing are met.
  @retval UNIT_TEST_SKIPPED   The pre-reqs for NULL pointer testing are not met.
**/
UNIT_TEST_STATUS
EFIAPI
SmmNullPointerPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  if (!mMmMps.NullPointerDetectionPolicy) {
    UT_LOG_WARNING ("This feature is disabled");
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
}

/// ================================================================================================
/// ================================================================================================
///
/// TEST CASES
///
/// ================================================================================================
/// ================================================================================================

/**
  This test case checks that page guards are present for the target memory type within Context.

  The test can be run in 3 ways:
  1. Using the Memory Attribute Protocol: This version of the test will allocate a page of the target
     memory type and check that the page preceding and succeeding the allocated page have the EFI_MEMORY_RP
     attribute active and fail otherwise.
  2. By intentionally causing and clearing faults: This version of the test will allocate a page of the
     target memory type and write to the page preceding and succeeding the allocated page. Before writing,
     the test will set the IgnoreNextPageFault flag using ExceptionPersistenceLib with the expectation that
     the interrupt handler will clear the intentional fault, unset the flag, and return. If the flag is still
     set after the write, the test will fail. Note that if the handler does not handle the flag and fault, the
     interrupt handler will deadloop or reset. This test will simply hang in the case of a deadloop and will
     fail upon returning to the test case if the system resets.
  3. By intentionally causing faults and resetting the system: This case is similar to the previous case
     except that the system will be reset after the intentional fault is triggered. Prior to causing a fault,
     the test will update a counter and save the framework state so when the test resumes after reset it can
     move on to the next phase of testing instead of repeating the same test.

  Future Work:
  1. Use the test context to ensure that if the testing method is MemoryProtectionTestClearFaults and the
     system still resets that the test will not be attempted again.

  @param[in]  Context           The context of the test.

  @retval UNIT_TEST_PASSED              The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED   A condition outlined in the test description was not met.
**/
UNIT_TEST_STATUS
EFIAPI
UefiPageGuard (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);
  EFI_PHYSICAL_ADDRESS            ptr                     = (EFI_PHYSICAL_ADDRESS)(UINTN)NULL;
  UINT64                          Attributes;

  DEBUG ((DEBUG_INFO, "%a - Testing Type: %a\n", __FUNCTION__, MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]));

  // Test using the Memory Attribute Protocol
  if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestMemoryAttributeProtocol) {
    UT_ASSERT_NOT_NULL (mMemoryAttributeProtocol);

    // Allocate a page of the target memory type
    gBS->AllocatePages (
           AllocateAnyPages,
           (EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType,
           1,
           (EFI_PHYSICAL_ADDRESS *)&ptr
           );
    UT_ASSERT_NOT_EQUAL (ptr, (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

    Attributes = 0;
    // Check that the page preceding the allocated page has the EFI_MEMORY_RP attribute set
    UT_ASSERT_NOT_EFI_ERROR (
      mMemoryAttributeProtocol->GetMemoryAttributes (
                                  mMemoryAttributeProtocol,
                                  ALIGN_ADDRESS (ptr) - EFI_PAGE_SIZE,
                                  EFI_PAGE_SIZE,
                                  &Attributes
                                  )
      );
    UT_ASSERT_NOT_EQUAL (Attributes & EFI_MEMORY_RP, 0);
    Attributes = 0;
    // Check that the page succeeding the allocated page has the EFI_MEMORY_RP attribute set
    UT_ASSERT_NOT_EFI_ERROR (
      mMemoryAttributeProtocol->GetMemoryAttributes (
                                  mMemoryAttributeProtocol,
                                  ALIGN_ADDRESS (ptr) + EFI_PAGE_SIZE,
                                  EFI_PAGE_SIZE,
                                  &Attributes
                                  )
      );
    UT_ASSERT_NOT_EQUAL (Attributes & EFI_MEMORY_RP, 0);

    // Test by intentionally causing and clearing faults
  } else if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestClearFaults) {
    UT_ASSERT_NOT_NULL (mNonstopModeProtocol);
    // Allocate a page of the target memory type
    gBS->AllocatePages (
           AllocateAnyPages,
           (EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType,
           1,
           (EFI_PHYSICAL_ADDRESS *)&ptr
           );
    UT_ASSERT_NOT_EQUAL (ptr, (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

    // Set the IgnoreNextPageFault flag
    UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());

    // Write to the head guard page
    HeadPageTest ((UINT64 *)(UINTN)ptr);

    // Check that the IgnoreNextPageFault flag was cleared
    if (GetIgnoreNextEx ()) {
      UT_LOG_ERROR ("Head guard page failed: %p", ptr);
      UT_ASSERT_FALSE (GetIgnoreNextEx ());
    }

    // Reset the page attributes of the faulted page(s) to their original attributes.
    UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());
    UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());

    // Write to the tail guard page
    TailPageTest ((UINT64 *)(UINTN)ptr);

    // Check that the IgnoreNextPageFault flag was cleared
    if (GetIgnoreNextEx ()) {
      UT_LOG_ERROR ("Tail guard page failed: %p", ptr);
      UT_ASSERT_FALSE (GetIgnoreNextEx ());
    }

    // Reset the page attributes of the faulted page(s) to their original attributes.
    UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());

    FreePages ((VOID *)(UINTN)ptr, 1);

    // Test by intentionally causing faults and resetting the system
  } else if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestReset) {
    if (MemoryProtectionContext.TestProgress < 2) {
      //
      // Context.TestProgress indicates progress within this specific test.
      // 0 - Just started.
      // 1 - Completed head guard test.
      // 2 - Completed tail guard test.
      //
      // Indicate the test is in progress and save state.
      //
      MemoryProtectionContext.TestProgress++;
      SetBootNextDevice ();
      SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

      // Allocate a page of the target memory type
      gBS->AllocatePages (
             AllocateAnyPages,
             (EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType,
             1,
             (EFI_PHYSICAL_ADDRESS *)&ptr
             );
      UT_ASSERT_NOT_EQUAL (ptr, (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

      // If TestProgress == 1, we are testing the head guard
      if (MemoryProtectionContext.TestProgress == 1) {
        DEBUG ((DEBUG_ERROR, "%a - Allocated page at 0x%p\n", __FUNCTION__, ptr));

        // Write to the head guard page
        HeadPageTest ((UINT64 *)(UINTN)ptr);

        // Anything executing past this point indicates a failure.
        UT_LOG_ERROR ("Head guard page failed: %p", ptr);
        // If TestProgress == 2, we are testing the tail guard
      } else {
        DEBUG ((DEBUG_ERROR, "%a - Allocated page at 0x%p\n", __FUNCTION__, ptr));

        // Write to the tail guard page
        TailPageTest ((UINT64 *)(UINTN)ptr);

        // Anything executing past this point indicates a failure.
        UT_LOG_ERROR ("Tail guard page failed: %p", ptr);
      }

      // Reset test progress so failure gets recorded.
      MemoryProtectionContext.TestProgress = 0;
      SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    }

    // TestProgress == 2 indicates we successfully tested the head and tail guard pages.
    UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == 2);
  } else {
    UT_LOG_ERROR ("Invalid testing method specified: %d\n", MemoryProtectionContext.TestingMethod);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

/**
  This test case checks that pool guards are present for the target memory type within Context. This
  test does not currently check that the allocated pool is properly aligned with the head or tail guard
  page.

  The test can be run in 3 ways:
  1. Using the Memory Attribute Protocol: This version of the test will allocate a pools of various sizes
     of the target memory type and check that the page preceding OR succeeding the page containing the
     allocated pool have the EFI_MEMORY_RP attribute active and fail otherwise. Only the head or tail
     page will be tested depending on the heap guard direction.
  2. By intentionally causing and clearing faults: This version of the test will allocate a pool of the
     target memory type and write to the page preceding OR succeeding the page containing the allocated
     pool. Before writing, the test will set the IgnoreNextPageFault flag using ExceptionPersistenceLib
     with the expectation that the interrupt handler will clear the intentional fault, unset the flag,
     and return. If the flag is still set after the write, the test will fail. Note that if the handler
     does not handle the flag and fault, the interrupt handler will deadloop or reset. This test will
     simply hang in the case of a deadloop and will fail upon returning to the test case if the system resets.
  3. By intentionally causing faults and resetting the system: This case is similar to the previous case
     except that the system will be reset after the intentional fault is triggered. Prior to causing a fault,
     the test will update a counter and save the framework state so when the test resumes after reset it can
     move on to the next phase of testing instead of repeating the same test.

  Future Work:
  1. Check that the allocated pool is properly aligned with the head or tail guard page.
  2. Use the test context to ensure that if the testing method is MemoryProtectionTestClearFaults and the
     system still resets that the test will not be attempted again.

  @param[in]  Context           The context of the test.

  @retval UNIT_TEST_PASSED              The test completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED   A condition outlined in the test description was not met.
**/
UNIT_TEST_STATUS
EFIAPI
UefiPoolGuard (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);
  UINT64                          *ptr                    = NULL;
  UINTN                           AllocationSize;
  UINT8                           Index = 0;
  UINT64                          Attributes;
  EFI_PHYSICAL_ADDRESS            PoolGuard;

  DEBUG ((DEBUG_INFO, "%a - Testing Type: %a\n", __FUNCTION__, MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]));

  // Test using the Memory Attribute Protocol.
  if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestMemoryAttributeProtocol) {
    UT_ASSERT_NOT_NULL (mMemoryAttributeProtocol);

    // Test each pool size in the pool size table.
    for (Index = 0; Index < ARRAY_SIZE (mPoolSizeTable); Index++) {
      AllocationSize = mPoolSizeTable[Index];

      // Allocate a pool of the target memory type.
      gBS->AllocatePool (
             (EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType,
             AllocationSize,
             (VOID **)&ptr
             );
      UT_ASSERT_NOT_NULL (ptr);

      Attributes = 0;
      // Check the head or tail guard page depending on the heap guard direction.
      if (mDxeMps.HeapGuardPolicy.Fields.Direction == HEAP_GUARD_ALIGNED_TO_TAIL) {
        PoolGuard = ALIGN_ADDRESS (((UINTN)ptr) + AllocationSize + (EFI_PAGE_SIZE - 1));
      } else {
        PoolGuard = ALIGN_ADDRESS (((UINTN)ptr) - (EFI_PAGE_SIZE - 1));
      }

      // Check that the guard page is active.
      UT_ASSERT_NOT_EFI_ERROR (
        mMemoryAttributeProtocol->GetMemoryAttributes (
                                    mMemoryAttributeProtocol,
                                    PoolGuard,
                                    EFI_PAGE_SIZE,
                                    &Attributes
                                    )
        );

      // Check that the guard page has the EFI_MEMORY_RP attribute set.
      UT_ASSERT_NOT_EQUAL (Attributes & EFI_MEMORY_RP, 0);
    }

    // Test by intentionally causing and clearing faults
  } else if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestClearFaults) {
    UT_ASSERT_NOT_NULL (mNonstopModeProtocol);

    // Test each pool size in the pool size table.
    for (Index = 0; Index < ARRAY_SIZE (mPoolSizeTable); Index++) {
      // Set the IgnoreNextPageFault flag.
      UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());

      AllocationSize = mPoolSizeTable[Index];

      gBS->AllocatePool (
             (EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType,
             AllocationSize,
             (VOID **)&ptr
             );
      UT_ASSERT_NOT_NULL (ptr);

      // Check the head OR tail guard page depending on the heap guard direction.
      PoolTest ((UINT64 *)ptr, AllocationSize);

      // Check that the IgnoreNextPageFault flag was cleared.
      UT_ASSERT_FALSE (GetIgnoreNextEx ());
      // Reset the attributes of the faulting page(s) to their original attributes.
      UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());
    }

    // Test by intentionally causing faults and resetting the system.
  } else if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestReset) {
    // If TestProgress == ARRAY_SIZE (mPoolSizeTable), we have completed all tests.
    if (MemoryProtectionContext.TestProgress < ARRAY_SIZE (mPoolSizeTable)) {
      //
      // Context.TestProgress indicates progress within this specific test.
      // The test progressively allocates larger areas to test the guard on.
      // These areas are defined in Pool.c as the 13 different sized chunks that are available
      // for pool allocation.
      //
      // Indicate the test is in progress and save state.
      //
      AllocationSize = mPoolSizeTable[MemoryProtectionContext.TestProgress];
      MemoryProtectionContext.TestProgress++;
      SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
      SetBootNextDevice ();

      // Allocate a pool of the target memory type.
      gBS->AllocatePool (
             (EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType,
             AllocationSize,
             (VOID **)&ptr
             );
      UT_ASSERT_NOT_NULL (ptr);

      // Write to the pool guard (should cause a fault and reset the system)
      PoolTest ((UINT64 *)ptr, AllocationSize);

      // If we reach this point, the fault did not occur and the test has failed.
      // Reset test progress so failure gets recorded.
      MemoryProtectionContext.TestProgress = 0;
      SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
      UT_LOG_ERROR ("Pool guard failed: %p", ptr);
    }

    UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == ARRAY_SIZE (mPoolSizeTable));
  } else {
    UT_LOG_ERROR ("Invalid testing method specified: %d\n", MemoryProtectionContext.TestingMethod);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

/**
  Test the stack guard.

  The test can be run in 3 ways:
  1. Using the Memory Attribute Protocol: This version of the test will fetch the HOB list and attempt to
     find the stack information identified by gEfiHobMemoryAllocStackGuid. If the HOB is found, the test
     will use the Memory Attribute Protocol to check that the page containing the stack base has the
     EFI_MEMORY_RP attribute. If the Memory Protection Debug Protocol is also available, the test will
     also check the multi-processor stacks using the information collected by the protocol.
  2. By intentionally causing and clearing a fault: This version will test stack guard by overflowing the
     stack with an infinite loop. Each loop iteration causes another stack frame to be pushed onto the
     stack. Eventually, the stack guard page should be hit and a hardware mechanism will switch to a safe
     stack to execute the interrupt handler. Prior to overflowing the stack, the IgnoreNextPageFault flag
     will be set using ExceptionPersistenceLib with the expectation that the interrupt handler will clear the
     fault, unset the flag, and return. If the stack guard is not properly configured or the handler does not
     unset the flag, the test will infinitely recurse and exhibit unknown behavior dependent on the system
     configuration.
  3. By intentionally causing a fault and resetting the system: This case is similar to the previous case
     except that the system will be reset after the intentional fault is triggered. Prior to causing a fault,
     the test will update a counter and save the framework state so when the test resumes after reset it can
     move on to the next phase of testing instead of repeating the same test. This version of the test will
     also exhibit unknown behavior if the stack guard is not properly configured.


  Future Work:
  1. Add support for testing the AP stacks without the Memory Attribute Protocol by switching the BSP stack
     using MP services and overflowing it.
  2. Use the test context to ensure that if the testing method is MemoryProtectionTestClearFaults and the
     system still resets that the test will not be attempted again.

  @param[in] Context  The test context.

  @retval UNIT_TEST_PASSED              The test was executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED   A condition outlined in the test description was not met.
**/
UNIT_TEST_STATUS
EFIAPI
UefiCpuStackGuard (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);
  EFI_PHYSICAL_ADDRESS            StackBase;
  EFI_PEI_HOB_POINTERS            Hob;
  EFI_HOB_MEMORY_ALLOCATION       *MemoryHob;
  UINT64                          Attributes;
  LIST_ENTRY                      *List;
  CPU_MP_DEBUG_PROTOCOL           *Entry;

  DEBUG ((DEBUG_INFO, "%a - Testing CPU Stack Guard\n", __FUNCTION__));

  // Test using the Memory Attribute Protocol.
  if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestMemoryAttributeProtocol) {
    UT_ASSERT_NOT_NULL (mMemoryAttributeProtocol);

    StackBase = 0;
    Hob.Raw   = GetHobList ();
    // Find the BSP stack info from the HOB list.
    while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw)) != NULL) {
      MemoryHob = Hob.MemoryAllocation;
      if (CompareGuid (&gEfiHobMemoryAllocStackGuid, &MemoryHob->AllocDescriptor.Name)) {
        StackBase = MemoryHob->AllocDescriptor.MemoryBaseAddress;
        UT_ASSERT_EQUAL (StackBase & EFI_PAGE_MASK, 0);
        break;
      }

      Hob.Raw = GET_NEXT_HOB (Hob);
    }

    // If StackBase == 0, we did not find the stack HOB.
    UT_ASSERT_NOT_EQUAL (StackBase, 0);
    Attributes = 0;

    // Check that the stack base has the EFI_MEMORY_RP attribute.
    UT_ASSERT_NOT_EFI_ERROR (
      mMemoryAttributeProtocol->GetMemoryAttributes (
                                  mMemoryAttributeProtocol,
                                  StackBase,
                                  EFI_PAGE_SIZE,
                                  &Attributes
                                  )
      );
    UT_ASSERT_NOT_EQUAL (Attributes & EFI_MEMORY_RP, 0);

    // If the Multi-Processor Debug Protocol is available, check the AP stacks.
    if (!EFI_ERROR (PopulateCpuMpDebugProtocol ())) {
      for (List = mCpuMpDebugProtocol->Link.ForwardLink; List != &mCpuMpDebugProtocol->Link; List = List->ForwardLink) {
        Entry = CR (
                  List,
                  CPU_MP_DEBUG_PROTOCOL,
                  Link,
                  CPU_MP_DEBUG_SIGNATURE
                  );

        // Skip the switch stack (the stack used when a stack overflow occurs).
        if (Entry->IsSwitchStack) {
          continue;
        }

        StackBase  = ALIGN_ADDRESS (Entry->ApStackBuffer);
        Attributes = 0;

        // Check that the stack base has the EFI_MEMORY_RP attribute.
        UT_ASSERT_NOT_EFI_ERROR (
          mMemoryAttributeProtocol->GetMemoryAttributes (
                                      mMemoryAttributeProtocol,
                                      StackBase,
                                      EFI_PAGE_SIZE,
                                      &Attributes
                                      )
          );
        UT_ASSERT_NOT_EQUAL (Attributes & EFI_MEMORY_RP, 0);
      }
    }

    // Test by intentionally causing and clearing faults.
  } else if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestClearFaults) {
    UT_ASSERT_NOT_NULL (mNonstopModeProtocol);
    // Set the IgnoreNextPageFault flag.
    UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());

    // Overflow the stack, checking at each level of recursion if the IgnoreNextPageFault
    // flag is still set.
    RecursionDynamic (1);

    // If the IgnoreNextPageFault flag is still set, the test failed. It's unlikely that
    // we'd reach this point in the test if the flag is still set as it implies that the
    // interrupt handler did not clear the stack overflow.
    UT_ASSERT_FALSE (GetIgnoreNextEx ());

    // Reset the page attributes to their original attributes.
    UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());

    // Test by intentionally causing a fault and resetting the system.
  } else if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestReset) {
    if (MemoryProtectionContext.TestProgress < 1) {
      //
      // Context.TestProgress 0 indicates the test hasn't started yet.
      //
      // Indicate the test is in progress and save state.
      //
      MemoryProtectionContext.TestProgress++;
      SetBootNextDevice ();
      SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

      // Overflow the stack.
      Recursion (1);

      // If we reach this point, the stack overflow did not cause a system reset and the test
      // has failed. Note that it's unlikely that we'd reach this point in the test if the
      // stack overflow did not cause a system reset.
      MemoryProtectionContext.TestProgress = 0;
      SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
      UT_LOG_ERROR ("System was expected to reboot but didn't.");
    }

    UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == 1);
  } else {
    UT_LOG_ERROR ("Invalid testing method specified: %d\n", MemoryProtectionContext.TestingMethod);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

volatile UNIT_TEST_FRAMEWORK  *mFw = NULL;

/**
  Test NULL pointer detection.

  The test can be run in 3 ways:
  1. Using the Memory Attribute Protocol: This version of the test will use the Memory Attribute Protocol
     to verify that the NULL page has the EFI_MEMORY_RP attribute.
  2. By intentionally causing and clearing a fault: This version will intentionally cause a fault by
     dereferencing a NULL pointer (both with a read and a write). Prior to dereferencing, the IgnoreNextPageFault flag
     will be set using ExceptionPersistenceLib with the expectation that the interrupt handler will clear the
     fault, unset the flag, and return. If the flag is still set after dereferencing NULL, the handler either
     was not invoked (implying NULL protection is not active) or the handler did not properly handle the flag.
     Both of these cases result in a test failure.
  3. By intentionally causing a fault and resetting the system: This case is similar to the previous case
     except that the system will be reset after the intentional fault is triggered. Prior to causing a fault,
     the test will update a counter and save the framework state so when the test resumes after reset it can
     move on to the next phase of testing instead of repeating the same test. If a reset does not occur, the
     test will fail.

  Future Work:
  1. Use the test context to ensure that if the testing method is MemoryProtectionTestClearFaults and the
     system still resets that the test will not be attempted again.

  @param[in] Context  The test context.

  @retval UNIT_TEST_PASSED              The test was executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED   A condition outlined in the test description was not met.
**/
UNIT_TEST_STATUS
EFIAPI
UefiNullPointerDetection (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);
  UINT64                          Attributes;

  DEBUG ((DEBUG_INFO, "%a - Testing NULL Pointer Detection\n", __FUNCTION__));

  // Test using the Memory Attribute Protocol.
  if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestMemoryAttributeProtocol) {
    UT_ASSERT_NOT_NULL (mMemoryAttributeProtocol);

    // Check that the NULL page has the EFI_MEMORY_RP attribute.
    UT_ASSERT_NOT_EFI_ERROR (
      mMemoryAttributeProtocol->GetMemoryAttributes (
                                  mMemoryAttributeProtocol,
                                  (UINTN)NULL,
                                  EFI_PAGE_SIZE,
                                  &Attributes
                                  )
      );
    UT_ASSERT_NOT_EQUAL (Attributes & EFI_MEMORY_RP, 0);

    // Test by intentionally causing and clearing faults.
  } else if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestClearFaults) {
    UT_ASSERT_NOT_NULL (mNonstopModeProtocol);

    // Set the IgnoreNextPageFault flag.
    UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());

    // Read from NULL.
    if (mFw->Title == NULL) {
      DEBUG ((DEBUG_INFO, "NULL pointer read test complete\n"));
    }

    // If the IgnoreNextPageFault flag is still set, the read test failed.
    if (GetIgnoreNextEx ()) {
      UT_LOG_ERROR ("Failed NULL pointer read test.");
      UT_ASSERT_FALSE (GetIgnoreNextEx ());
    }

    // Reset the page attributes to their original attributes.
    UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());

    // Set the IgnoreNextPageFault flag.
    UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());

    // Write to NULL.
    mFw->Title = "Title";

    // If the IgnoreNextPageFault flag is still set, the write test failed.
    if (GetIgnoreNextEx ()) {
      UT_LOG_ERROR ("Failed NULL pointer write test.");
      UT_ASSERT_FALSE (GetIgnoreNextEx ());
    }

    // Reset the page attributes to their original attributes.
    UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());

    // Test by intentionally causing a fault and resetting the system.
  } else if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestReset) {
    if (MemoryProtectionContext.TestProgress < 2) {
      //
      // Context.TestProgress indicates progress within this specific test.
      // 0 - Just started.
      // 1 - Completed NULL pointer read test.
      // 2 - Completed NULL pointer write test.
      //
      // Indicate the test is in progress and save state.
      //
      MemoryProtectionContext.TestProgress++;
      SetBootNextDevice ();
      SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

      if (MemoryProtectionContext.TestProgress == 1) {
        if (mFw->Title == NULL) {
          DEBUG ((DEBUG_ERROR, "%a - Should have failed \n", __FUNCTION__));
        }

        UT_LOG_ERROR ("Failed NULL pointer read test.");
      } else {
        mFw->Title = "Title";
        UT_LOG_ERROR ("Failed NULL pointer write test.");
      }

      //
      // At this point, the test has failed. Reset test progress so failure gets recorded.
      //
      MemoryProtectionContext.TestProgress = 0;
      SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    }

    UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == 2);
  } else {
    UT_LOG_ERROR ("Invalid testing method specified: %d\n", MemoryProtectionContext.TestingMethod);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

/**
  Test stack no-execute protection.

  The test can be run in 3 ways:
  1. Using the Memory Attribute Protocol: This version of the test will use the Memory Attribute Protocol
     to verify that the page containing the stack (identified by getting the address of a stack variable)
     has the EFI_MEMORY_XP attribute.
  2. By intentionally causing and clearing a fault: This version will intentionally cause a fault by
     copying a dummy function into a statically allocated buffer on the stack and executing that function.
     Prior to execution, the IgnoreNextPageFault flag will be set using ExceptionPersistenceLib with the
     expectation that the interrupt handler will clear the fault, unset the flag, and return.
     If the flag is still set after executing from the stack, the handler either was not invoked
     (implying stack no-execute protection is not active) or the handler did not properly handle the flag.
     Both of these cases result in a test failure.
  3. By intentionally causing a fault and resetting the system: This case is similar to the previous case
     except that the system will be reset after the intentional fault is triggered. Prior to causing a fault,
     the test will update a counter and save the framework state so when the test resumes after reset it can
     move on to the next phase of testing instead of repeating the same test. If a reset does not occur, the
     test will fail.

  Future Work:
  1. Use the test context to ensure that if the testing method is MemoryProtectionTestClearFaults and the
     system still resets that the test will not be attempted again.

  @param[in] Context  The test context.

  @retval UNIT_TEST_PASSED              The test was executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED   A condition outlined in the test description was not met.
**/
UNIT_TEST_STATUS
EFIAPI
UefiNxStackGuard (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);
  UINT8                           CodeRegionToCopyTo[DUMMY_FUNCTION_FOR_CODE_SELF_TEST_GENERIC_SIZE];
  UINT8                           *CodeRegionToCopyFrom = (UINT8 *)DummyFunctionForCodeSelfTest;
  UINT64                          Attributes;

  DEBUG ((DEBUG_INFO, "%a - NX Stack Guard\n", __FUNCTION__));

  // Test using the Memory Attribute Protocol.
  if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestMemoryAttributeProtocol) {
    UT_ASSERT_NOT_NULL (mMemoryAttributeProtocol);

    Attributes = 0;
    // Attributes is a stack variable, so get the attributes of the page containing it.
    UT_ASSERT_NOT_EFI_ERROR (
      mMemoryAttributeProtocol->GetMemoryAttributes (
                                  mMemoryAttributeProtocol,
                                  ALIGN_ADDRESS ((UINTN)&Attributes),
                                  EFI_PAGE_SIZE,
                                  &Attributes
                                  )
      );

    // Verify the page containing Attributes is non-executable.
    UT_ASSERT_NOT_EQUAL (Attributes & EFI_MEMORY_XP, 0);

    // Test by intentionally causing and clearing faults.
  } else if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestClearFaults) {
    UT_ASSERT_NOT_NULL (mNonstopModeProtocol);

    // Set the IgnoreNextPageFault flag.
    UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());

    // Copy the dummy function to a stack variable and execute it.
    CopyMem (CodeRegionToCopyTo, CodeRegionToCopyFrom, DUMMY_FUNCTION_FOR_CODE_SELF_TEST_GENERIC_SIZE);
    ((DUMMY_VOID_FUNCTION_FOR_DATA_TEST)CodeRegionToCopyTo)();

    // If the IgnoreNextPageFault flag is still set, the interrupt handler was not invoked or did not handle
    // the flag properly.
    UT_ASSERT_FALSE (GetIgnoreNextEx ());

    // Reset the page attributes to their original attributes.
    UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());

    // Test by intentionally causing a fault and resetting the system.
  } else if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestReset) {
    if (MemoryProtectionContext.TestProgress < 1) {
      //
      // Context.TestProgress 0 indicates the test hasn't started yet.
      //
      // Indicate the test is in progress by updating the context and saving state.
      //
      MemoryProtectionContext.TestProgress++;
      SetBootNextDevice ();
      SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

      // Copy the dummy function to a stack variable and execute it.
      CopyMem (CodeRegionToCopyTo, CodeRegionToCopyFrom, DUMMY_FUNCTION_FOR_CODE_SELF_TEST_GENERIC_SIZE);
      ((DUMMY_VOID_FUNCTION_FOR_DATA_TEST)CodeRegionToCopyTo)();

      // If we reach this point, the stack is executable. Log the test failure.
      MemoryProtectionContext.TestProgress = 0;
      SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
      UT_LOG_ERROR ("NX Stack Guard Test failed.");
    }

    UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == 1);
  } else {
    UT_LOG_ERROR ("Invalid testing method specified: %d\n", MemoryProtectionContext.TestingMethod);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

/**
  Test no-execute protection.

  The test can be run in 3 ways:
  1. Using the Memory Attribute Protocol: This version of the test will allocate memory of the type defined
     in Context and use the Memory Attribute Protocol to verify that allocated page the EFI_MEMORY_XP attribute.
  2. By intentionally causing and clearing a fault: This version of the test will allocate memory of the
     type defined in Context and intentionally cause a fault by copying a dummy function into the allocated
     buffer and executing that function. Prior to execution, the IgnoreNextPageFault flag will be set using
     ExceptionPersistenceLib with the expectation that the interrupt handler will clear the fault, unset the
     flag, and return. If the flag is still set after executing from the stack, the handler either was not
     invoked (implying no-execute protection is not active) or the handler did not properly handle the flag.
     Both of these cases result in a test failure.
  3. By intentionally causing a fault and resetting the system: This case is similar to the previous case
     except that the system will be reset after the intentional fault is triggered. Prior to causing a fault,
     the test will update a counter and save the framework state so when the test resumes after reset it can
     move on to the next phase of testing instead of repeating the same test. If a reset does not occur, the
     test will fail.

  Future Work:
  1. Use the test context to ensure that if the testing method is MemoryProtectionTestClearFaults and the
     system still resets that the test will not be attempted again.

  @param[in] Context  The test context.

  @retval UNIT_TEST_PASSED              The test was executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED   A condition outlined in the test description was not met.
**/
UNIT_TEST_STATUS
EFIAPI
UefiNxProtection (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);
  UINT64                          *ptr                    = NULL;
  UINT8                           *CodeRegionToCopyFrom   = (UINT8 *)DummyFunctionForCodeSelfTest;
  UINT64                          Attributes;

  DEBUG ((DEBUG_INFO, "%a - Testing Type: %a\n", __FUNCTION__, MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]));

  // Test using the Memory Attribute Protocol.
  if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestMemoryAttributeProtocol) {
    UT_ASSERT_NOT_NULL (mMemoryAttributeProtocol);

    // Allocate a page of memory of the type specified in Context.
    gBS->AllocatePool (
           (EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType,
           EFI_PAGE_SIZE,
           (VOID **)&ptr
           );
    UT_ASSERT_NOT_NULL (ptr);

    Attributes = 0;
    // Verify the allocated page is non-executable.
    UT_ASSERT_NOT_EFI_ERROR (
      mMemoryAttributeProtocol->GetMemoryAttributes (
                                  mMemoryAttributeProtocol,
                                  ALIGN_ADDRESS ((UINTN)ptr),
                                  EFI_PAGE_SIZE,
                                  &Attributes
                                  )
      );
    FreePool (ptr);
    UT_ASSERT_NOT_EQUAL (Attributes & EFI_MEMORY_XP, 0);

    // Test by intentionally causing and clearing faults.
  } else if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestClearFaults) {
    UT_ASSERT_NOT_NULL (mNonstopModeProtocol);

    // Set the IgnoreNextPageFault flag.
    UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());

    // Allocate a page of memory of the type specified in Context.
    gBS->AllocatePool (
           (EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType,
           EFI_PAGE_SIZE,
           (VOID **)&ptr
           );
    UT_ASSERT_NOT_NULL (ptr);

    // Copy the dummy function to the allocated buffer and execute it.
    CopyMem (ptr, CodeRegionToCopyFrom, DUMMY_FUNCTION_FOR_CODE_SELF_TEST_GENERIC_SIZE);
    ((DUMMY_VOID_FUNCTION_FOR_DATA_TEST)ptr)();

    FreePool (ptr);

    // Verify the IgnoreNextPageFault flag was cleared.
    UT_ASSERT_FALSE (GetIgnoreNextEx ());

    // Reset the page attributes to their original attributes.
    UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());

    // Test by intentionally causing a fault and resetting the system.
  } else if (MemoryProtectionContext.TestingMethod == MemoryProtectionTestReset) {
    if (MemoryProtectionContext.TestProgress < 1) {
      //
      // Context.TestProgress == 0 indicates the test hasn't started yet.
      //
      // Indicate the test is in progress and save state.
      //
      MemoryProtectionContext.TestProgress++;
      SetBootNextDevice ();
      SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

      // Allocate a page of memory of the type specified in Context.
      gBS->AllocatePool (
             (EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType,
             EFI_PAGE_SIZE,
             (VOID **)&ptr
             );
      UT_ASSERT_NOT_NULL (ptr);

      // Copy the dummy function to the allocated buffer and execute it.
      CopyMem (ptr, CodeRegionToCopyFrom, DUMMY_FUNCTION_FOR_CODE_SELF_TEST_GENERIC_SIZE);
      ((DUMMY_VOID_FUNCTION_FOR_DATA_TEST)ptr)();

      // If the test reaches this point, the above function invocation did not cause a fault.
      // The test has failed.
      MemoryProtectionContext.TestProgress = 0;
      SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
      UT_LOG_ERROR ("NX Test failed.");
    }

    UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == 1);
  } else {
    UT_LOG_ERROR ("Invalid testing method specified: %d\n", MemoryProtectionContext.TestingMethod);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

/**
  Test image protection by using the Memory Protection Debug Protocol to get a list of currently
  protected images and using the Memory Attribute Protocol to check that the code sections of the
  image have the EFI_MEMORY_RP attribute and the data sections have the EFI_MEMORY_XP attribute.

  @param[in] Context  The test context.

  @retval UNIT_TEST_PASSED              The test was executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED   Failed to fetch the image list
  @retval UNIT_TEST_ERROR_TEST_FAILED   A condition outlined in the test description was not met.
**/
UNIT_TEST_STATUS
EFIAPI
ImageProtection (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS              Status;
  IMAGE_RANGE_DESCRIPTOR  *ImageRangeDescriptorHead = NULL;
  LIST_ENTRY              *ImageRangeDescriptorLink = NULL;
  IMAGE_RANGE_DESCRIPTOR  *ImageRangeDescriptor     = NULL;
  BOOLEAN                 TestFailed                = FALSE;
  UINT64                  Attributes                = 0;

  DEBUG ((DEBUG_INFO, "%a() - Enter\n", __FUNCTION__));

  // Ensure the Memory Protection Protocol and Memory Attribute Protocol are available.
  UT_ASSERT_NOT_NULL (mMemoryProtectionProtocol);
  UT_ASSERT_NOT_NULL (mMemoryAttributeProtocol);

  // Use the Memory Protection Protocol to get a list of protected images. Each descriptor in the
  // output list will be a code or data section of a protected image.
  UT_ASSERT_NOT_EFI_ERROR (mMemoryProtectionProtocol->GetImageList (&ImageRangeDescriptorHead, Protected));

  // Walk through each image
  for (ImageRangeDescriptorLink = ImageRangeDescriptorHead->Link.ForwardLink;
       ImageRangeDescriptorLink != &ImageRangeDescriptorHead->Link;
       ImageRangeDescriptorLink = ImageRangeDescriptorLink->ForwardLink)
  {
    ImageRangeDescriptor = CR (
                             ImageRangeDescriptorLink,
                             IMAGE_RANGE_DESCRIPTOR,
                             Link,
                             IMAGE_RANGE_DESCRIPTOR_SIGNATURE
                             );
    if (ImageRangeDescriptor != NULL) {
      // Get the attributes of the image range.
      Status = mMemoryAttributeProtocol->GetMemoryAttributes (
                                           mMemoryAttributeProtocol,
                                           ImageRangeDescriptor->Base,
                                           ImageRangeDescriptor->Length,
                                           &Attributes
                                           );

      if (EFI_ERROR (Status)) {
        UT_LOG_ERROR (
          "Unable to get attributes of memory range 0x%llx - 0x%llx! Status: %r",
          ImageRangeDescriptor->Base,
          ImageRangeDescriptor->Base + ImageRangeDescriptor->Length,
          Status
          );
        TestFailed = TRUE;
        continue;
      }

      // Check that the code sections have the EFI_MEMORY_RO attribute and the data sections have
      // the EFI_MEMORY_XP attribute.
      if ((ImageRangeDescriptor->Type == Code) && ((Attributes & EFI_MEMORY_RO) == 0)) {
        TestFailed = TRUE;
        UT_LOG_ERROR (
          "Memory Range 0x%llx - 0x%llx should be non-writeable!",
          ImageRangeDescriptor->Base,
          ImageRangeDescriptor->Base + ImageRangeDescriptor->Length
          );
      } else if ((ImageRangeDescriptor->Type == Data) && ((Attributes & EFI_MEMORY_XP) == 0)) {
        TestFailed = TRUE;
        UT_LOG_ERROR (
          "Memory Range 0x%llx - 0x%llx should be non-executable!",
          ImageRangeDescriptor->Base,
          ImageRangeDescriptor->Base + ImageRangeDescriptor->Length
          );
      }
    }
  }

  // Free the list of image range descriptors.
  while (!IsListEmpty (&ImageRangeDescriptorHead->Link)) {
    ImageRangeDescriptor = CR (
                             ImageRangeDescriptorHead->Link.ForwardLink,
                             IMAGE_RANGE_DESCRIPTOR,
                             Link,
                             IMAGE_RANGE_DESCRIPTOR_SIGNATURE
                             );

    RemoveEntryList (&ImageRangeDescriptor->Link);
    FreePool (ImageRangeDescriptor);
  }

  FreePool (ImageRangeDescriptorHead);

  // If TestFailed is TRUE, the test has failed.
  UT_ASSERT_FALSE (TestFailed);

  return UNIT_TEST_PASSED;
}

/**
  This test requires that the MM memory protection driver is present. This test will use the mailbox
  to pass the test context to the MM driver. The MM driver will allocate a page of the target memory
  type described in Context and attempt to write to the page immediately preceding and succeeding
  the allocated page. Prior to communicating with the MM driver, this test will update a counter
  and save the framework state so when the test resumes after reset it can move on to the next phase
  of testing instead of repeating the same test. If a reset does not occur, the test will fail.

  @param[in] Context  The test context.

  @retval UNIT_TEST_PASSED              The test was executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED   A condition outlined in the test description was not met.
**/
UNIT_TEST_STATUS
EFIAPI
SmmPageGuard (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);
  EFI_STATUS                      Status;

  if (MemoryProtectionContext.TestProgress < 2) {
    //
    // Context.TestProgress indicates progress within this specific test.
    // 0 - Just started.
    // 1 - Completed head guard test.
    // 2 - Completed tail guard test.
    //
    // Indicate the test is in progress and save state.
    //
    MemoryProtectionContext.TestProgress++;
    SetBootNextDevice ();
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

    // Communicate to the MM driver to run the page guard test based on MemoryProtectionContext.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate (MEMORY_PROTECTION_TEST_PAGE, &MemoryProtectionContext);
    if (Status == EFI_NOT_FOUND) {
      UT_LOG_WARNING ("SMM test driver is not loaded.");
      return UNIT_TEST_SKIPPED;
    } else {
      UT_LOG_ERROR ("System was expected to reboot, but didn't.");
    }

    // If the test reaches this point, the MM driver did not not cause a fault and reset.
    // The test has failed.
    MemoryProtectionContext.TestProgress = 0;
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
  }

  // TestProgress will be 2 if the test has completed successfully.
  UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == 2);

  return UNIT_TEST_PASSED;
}

/**
  This test requires that the MM memory protection driver is present. This test will use the mailbox
  to pass the test context to the MM driver. The MM driver will allocate a pool of the target memory
  type described in Context and attempt to write to the page immediately preceding and succeeding
  the page containing the allocated pool which should cause the system to reset. The MM driver does
  not test that the pool is properly aligned to the head or tail of the guard. Prior to communicating
  with the MM driver, this test will update a counter and save the framework state so when the test
  resumes after reset it can move on to the next phase of testing instead of repeating the same test.
  If a reset does not occur, the test will fail.

  @param[in] Context  The test context.

  @retval UNIT_TEST_PASSED              The test was executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED   A condition outlined in the test description was not met.
**/
UNIT_TEST_STATUS
EFIAPI
SmmPoolGuard (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);
  EFI_STATUS                      Status;

  if (MemoryProtectionContext.TestProgress < ARRAY_SIZE (mPoolSizeTable)) {
    //
    // Context.TestProgress indicates progress within this specific test.
    // The test progressively allocates larger areas to test the guard on.
    // These areas are defined in Pool.c as the 13 different sized chunks that are available
    // for pool allocation.
    //
    // Indicate the test is in progress and save state.
    //
    MemoryProtectionContext.TestProgress++;
    SetBootNextDevice ();
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

    // Communicate to the MM driver to run the pool guard test based on MemoryProtectionContext.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate (MEMORY_PROTECTION_TEST_POOL, &MemoryProtectionContext);

    if (Status == EFI_NOT_FOUND) {
      UT_LOG_WARNING ("SMM test driver is not loaded.");
      return UNIT_TEST_SKIPPED;
    } else {
      UT_LOG_ERROR ("System was expected to reboot, but didn't.");
    }

    // If the test reaches this point, the MM driver did not not cause a fault and reset.
    // The test has failed.
    MemoryProtectionContext.TestProgress = 0;
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
  }

  // TestProgress will be 1 if the test has completed successfully.
  UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == 1);

  return UNIT_TEST_PASSED;
}

/**
  This test requires that the MM memory protection driver is present. This test will use the mailbox
  to pass the test context to the MM driver. The MM driver will dereference NULL via write and read which
  should cause a fault and reset. Prior to communicating with the MM driver, this test will update a counter
  and save the framework state so when the test resumes after reset it can move on to the next phase
  of testing instead of repeating the same test. If a reset does not occur, the test will fail.

  @param[in] Context  The test context.

  @retval UNIT_TEST_PASSED              The test was executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED   A condition outlined in the test description was not met.
**/
UNIT_TEST_STATUS
EFIAPI
SmmNullPointerDetection (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);
  EFI_STATUS                      Status;

  if (MemoryProtectionContext.TestProgress < 1) {
    //
    // Context.TestProgress 0 indicates the test hasn't started yet.
    //
    // Indicate the test is in progress and save state.
    //
    MemoryProtectionContext.TestProgress++;
    SetBootNextDevice ();
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

    // Communicate to the MM driver to run the NULL pointer test based on MemoryProtectionContext.
    Status = SmmMemoryProtectionsDxeToSmmCommunicate (MEMORY_PROTECTION_TEST_NULL_POINTER, &MemoryProtectionContext);

    if (Status == EFI_NOT_FOUND) {
      UT_LOG_WARNING ("SMM test driver is not loaded.");
      return UNIT_TEST_SKIPPED;
    } else {
      UT_LOG_ERROR ("System was expected to reboot, but didn't. %r", Status);
    }

    // If the test reaches this point, the MM driver did not not cause a fault and reset.
    // The test has failed.
    MemoryProtectionContext.TestProgress = 0;
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
  }

  // TestProgress will be 1 if the test has completed successfully.
  UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == 1);

  return UNIT_TEST_PASSED;
}

/// ================================================================================================
/// ================================================================================================
///
/// TEST ENGINE
///
/// ================================================================================================
/// ================================================================================================

/**
  This function adds a test case for each memory type with no-execute protection enabled.

  @param[in] TestSuite       The test suite to add the test cases to.
  @param[in] TestingMethod   The method to use for testing (Memory Attribute, Clear Faults, etc.)
**/
VOID
AddUefiNxTest (
  UNIT_TEST_SUITE_HANDLE            TestSuite,
  MEMORY_PROTECTION_TESTING_METHOD  TestingMethod
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  *MemoryProtectionContext = NULL;
  UINT8                           Index;
  CHAR8                           NameStub[]        = "Security.NxProtection.Uefi";
  CHAR8                           DescriptionStub[] = "Execution of a page of the following memory type should fail. Memory type: ";
  CHAR8                           *TestName         = NULL;
  CHAR8                           *TestDescription  = NULL;
  UINTN                           TestNameSize;
  UINTN                           TestDescriptionSize;

  DEBUG ((DEBUG_INFO, "%a() - Enter\n", __FUNCTION__));

  // Need to generate a test case for each memory type.
  for (Index = 0; Index < EfiMaxMemoryType; Index++) {
    MemoryProtectionContext =  (MEMORY_PROTECTION_TEST_CONTEXT *)AllocateZeroPool (sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    if (MemoryProtectionContext == NULL) {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test context failed.\n", __FUNCTION__));
      return;
    }

    // Set the context for this test case.
    MemoryProtectionContext->TargetMemoryType = Index;
    MemoryProtectionContext->GuardAlignment   = mDxeMps.HeapGuardPolicy.Fields.Direction;
    MemoryProtectionContext->TestingMethod    = TestingMethod;

    // Set the test name and description.
    TestNameSize        = sizeof (CHAR8) * (1 + AsciiStrnLenS (NameStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName            = AllocateZeroPool (TestNameSize);
    TestDescriptionSize = sizeof (CHAR8) * (1 + AsciiStrnLenS (DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription     = (CHAR8 *)AllocateZeroPool (TestDescriptionSize);

    if ((TestName != NULL) && (TestDescription != NULL) && (MemoryProtectionContext != NULL)) {
      // Name of the test is Security.PageGuard.Uefi + Memory Type Name (from MEMORY_TYPES)
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)
      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      // Add the test case. This test case will only run if UefiNxProtectionPreReq passes (which checks the protection policy for
      // the memory type).
      AddTestCase (TestSuite, TestDescription, TestName, UefiNxProtection, UefiNxProtectionPreReq, NULL, MemoryProtectionContext);

      // Free the memory allocated for the test name and description.
      FreePool (TestName);
      FreePool (TestDescription);
    } else {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test creation failed.\n", __FUNCTION__));
      return;
    }
  }
}

/**
  This function adds a test case for each memory type with pool guards enabled.

  @param[in] TestSuite       The test suite to add the test cases to.
  @param[in] TestingMethod   The method to use for testing (Memory Attribute, Clear Faults, etc.)
**/
VOID
AddUefiPoolTest (
  UNIT_TEST_SUITE_HANDLE            TestSuite,
  MEMORY_PROTECTION_TESTING_METHOD  TestingMethod
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  *MemoryProtectionContext = NULL;
  UINT8                           Index;
  CHAR8                           NameStub[]        = "Security.PoolGuard.Uefi";
  CHAR8                           DescriptionStub[] = "Accesses before/after the pool should hit a guard page. Memory type: ";
  CHAR8                           *TestName         = NULL;
  CHAR8                           *TestDescription  = NULL;
  UINTN                           TestNameSize;
  UINTN                           TestDescriptionSize;

  DEBUG ((DEBUG_INFO, "%a() - Enter\n", __FUNCTION__));

  // Need to generate a test case for each memory type.
  for (Index = 0; Index < EfiMaxMemoryType; Index++) {
    MemoryProtectionContext =  (MEMORY_PROTECTION_TEST_CONTEXT *)AllocateZeroPool (sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    if (MemoryProtectionContext == NULL) {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test context failed.\n", __FUNCTION__));
      return;
    }

    // Set the context for this test case.
    MemoryProtectionContext->TargetMemoryType = Index;
    MemoryProtectionContext->GuardAlignment   = mDxeMps.HeapGuardPolicy.Fields.Direction;
    MemoryProtectionContext->TestingMethod    = TestingMethod;

    // Set the test name and description.
    TestNameSize        = sizeof (CHAR8) * (1 + AsciiStrnLenS (NameStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName            = (CHAR8 *)AllocateZeroPool (TestNameSize);
    TestDescriptionSize = sizeof (CHAR8) * (1 + AsciiStrnLenS (DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription     = (CHAR8 *)AllocateZeroPool (TestDescriptionSize);

    if ((TestName != NULL) && (TestDescription != NULL) && (MemoryProtectionContext != NULL)) {
      // Name of the test is Security.PoolGuard.Uefi + Memory Type Name (from MEMORY_TYPES)
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)
      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      // Add the test case. This test case will only run if UefiPoolGuardPreReq passes (which checks the protection policy for
      // the memory type).
      AddTestCase (TestSuite, TestDescription, TestName, UefiPoolGuard, UefiPoolGuardPreReq, NULL, MemoryProtectionContext);

      FreePool (TestName);
      FreePool (TestDescription);
    } else {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test creation failed.\n", __FUNCTION__));
      return;
    }
  }
}

/**
  This function adds a test case for each memory type with page guards enabled.

  @param[in] TestSuite       The test suite to add the test cases to.
  @param[in] TestingMethod   The method to use for testing (Memory Attribute, Clear Faults, etc.)
**/
VOID
AddUefiPageTest (
  UNIT_TEST_SUITE_HANDLE            TestSuite,
  MEMORY_PROTECTION_TESTING_METHOD  TestingMethod
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  *MemoryProtectionContext = NULL;
  UINT8                           Index;
  CHAR8                           NameStub[]        = "Security.PageGuard.Uefi";
  CHAR8                           DescriptionStub[] = "Accesses before and after an allocated page should hit a guard page. Memory type: ";
  CHAR8                           *TestName         = NULL;
  CHAR8                           *TestDescription  = NULL;
  UINTN                           TestNameSize;
  UINTN                           TestDescriptionSize;

  DEBUG ((DEBUG_INFO, "%a() - Enter\n", __FUNCTION__));

  // Need to generate a test case for each memory type.
  for (Index = 0; Index < EfiMaxMemoryType; Index++) {
    MemoryProtectionContext =  (MEMORY_PROTECTION_TEST_CONTEXT *)AllocateZeroPool (sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    if (MemoryProtectionContext == NULL) {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test context failed.\n", __FUNCTION__));
      return;
    }

    // Set the context for this test case.
    MemoryProtectionContext->TargetMemoryType = Index;
    MemoryProtectionContext->GuardAlignment   = mDxeMps.HeapGuardPolicy.Fields.Direction;
    MemoryProtectionContext->TestingMethod    = TestingMethod;

    // Set the test name and description.
    TestNameSize        = sizeof (CHAR8) * (1 + AsciiStrnLenS (NameStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName            = (CHAR8 *)AllocateZeroPool (TestNameSize);
    TestDescriptionSize = sizeof (CHAR8) * (1 + AsciiStrnLenS (DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription     = (CHAR8 *)AllocateZeroPool (TestDescriptionSize);

    if ((TestName != NULL) && (TestDescription != NULL) && (MemoryProtectionContext != NULL)) {
      // Name of the test is Security.PageGuard.Uefi + Memory Type Name (from MEMORY_TYPES)
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)
      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      // Add the test case. This test case will only run if UefiPageGuardPreReq passes (which checks the protection policy for
      // the memory type).
      AddTestCase (TestSuite, TestDescription, TestName, UefiPageGuard, UefiPageGuardPreReq, NULL, MemoryProtectionContext);

      FreePool (TestName);
      FreePool (TestDescription);
    } else {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test creation failed.\n", __FUNCTION__));
      return;
    }
  }
}

/**
  This function adds an MM test case for each memory type with pool guards enabled.

  @param[in] TestSuite       The test suite to add the test cases to.
  @param[in] TestingMethod   The method to use for testing (Memory Attribute, Clear Faults, etc.)
**/
VOID
AddSmmPoolTest (
  UNIT_TEST_SUITE_HANDLE  TestSuite
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  *MemoryProtectionContext = NULL;
  UINT8                           Index;
  CHAR8                           NameStub[]        = "Security.PoolGuard.Smm";
  CHAR8                           DescriptionStub[] = "Accesses before/after the pool should hit a guard page in SMM. Memory type: ";
  CHAR8                           *TestName         = NULL;
  CHAR8                           *TestDescription  = NULL;
  UINTN                           TestNameSize;
  UINTN                           TestDescriptionSize;

  // Need to generate a test case for each memory type.
  for (Index = 0; Index < EfiMaxMemoryType; Index++) {
    MemoryProtectionContext =  (MEMORY_PROTECTION_TEST_CONTEXT *)AllocateZeroPool (sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    if (MemoryProtectionContext == NULL) {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test context failed.\n", __FUNCTION__));
      return;
    }

    // Set the context for this test case.
    MemoryProtectionContext->TargetMemoryType = Index;
    MemoryProtectionContext->GuardAlignment   = mDxeMps.HeapGuardPolicy.Fields.Direction;

    // Set the test name and description.
    TestNameSize        = sizeof (CHAR8) * (1 + AsciiStrnLenS (NameStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName            = (CHAR8 *)AllocateZeroPool (TestNameSize);
    TestDescriptionSize = sizeof (CHAR8) * (1 + AsciiStrnLenS (DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription     = (CHAR8 *)AllocateZeroPool (TestDescriptionSize);

    if ((TestName != NULL) && (TestDescription != NULL) && (MemoryProtectionContext != NULL)) {
      // Name of the test is Security.PoolGuard.Smm + Memory Type Name (from MEMORY_TYPES)
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)
      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      // Add the test case. This test case will only run if SmmPoolGuardPreReq passes (which checks the protection policy for
      // the memory type).
      AddTestCase (TestSuite, TestDescription, TestName, SmmPoolGuard, SmmPoolGuardPreReq, NULL, MemoryProtectionContext);

      FreePool (TestName);
      FreePool (TestDescription);
    } else {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test creation failed.\n", __FUNCTION__));
      return;
    }
  }
}

/**
  This function adds an MM test case for each memory type with page guards enabled.

  @param[in] TestSuite       The test suite to add the test cases to.
  @param[in] TestingMethod   The method to use for testing (Memory Attribute, Clear Faults, etc.)
**/
VOID
AddSmmPageTest (
  UNIT_TEST_SUITE_HANDLE  TestSuite
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  *MemoryProtectionContext = NULL;
  UINT8                           Index;
  CHAR8                           NameStub[]        = "Security.PageGuard.Smm";
  CHAR8                           DescriptionStub[] = "Accesses before and after an allocated page should hit a guard page in SMM. Memory type: ";
  CHAR8                           *TestName         = NULL;
  CHAR8                           *TestDescription  = NULL;
  UINTN                           TestNameSize;
  UINTN                           TestDescriptionSize;

  // Need to generate a test case for each memory type.
  for (Index = 0; Index < EfiMaxMemoryType; Index++) {
    MemoryProtectionContext =  (MEMORY_PROTECTION_TEST_CONTEXT *)AllocateZeroPool (sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    if (MemoryProtectionContext == NULL) {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test context failed.\n", __FUNCTION__));
      return;
    }

    // Set the context for this test case.
    MemoryProtectionContext->TargetMemoryType = Index;
    MemoryProtectionContext->GuardAlignment   = mDxeMps.HeapGuardPolicy.Fields.Direction;

    // Set the test name and description.
    TestNameSize        = sizeof (CHAR8) * (1 + AsciiStrnLenS (NameStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName            = (CHAR8 *)AllocateZeroPool (TestNameSize);
    TestDescriptionSize = sizeof (CHAR8) * (1 + AsciiStrnLenS (DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription     = (CHAR8 *)AllocateZeroPool (TestDescriptionSize);

    if ((TestName != NULL) && (TestDescription != NULL) && (MemoryProtectionContext != NULL)) {
      // Name of the test is Security.PageGuard.Smm + Memory Type Name (from MEMORY_TYPES)
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)
      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      // Add the test case. This test case will only run if SmmPageGuardPreReq passes (which checks the protection policy for
      // the memory type).
      AddTestCase (TestSuite, TestDescription, TestName, SmmPageGuard, SmmPageGuardPreReq, NULL, MemoryProtectionContext);

      FreePool (TestName);
      FreePool (TestDescription);
    } else {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test creation failed.\n", __FUNCTION__));
      return;
    }
  }
}

/**
  Determine the test method which will be used to run this unit test. If a preferred test method is specified,
  that test method MUST be usable or this function will return an error. If no preferred test method is
  specified, the test will run with the first available test method in the following order:
  1. Memory Attribute Protocol
  2. Clear Faults
  3. Reset System

  @param[in]  PreferredTestingMethod  The preferred testing method to use. If this method is not usable, this
                                      function will return an error.
  @param[out] TestingMethod           The testing method which will be used to run this test.

  @retval EFI_SUCCESS                 The testing method was successfully determined.
  @retval EFI_UNSUPPORTED             None of the testing methods were usable.
  @retval EFI_INVALID_PARAMETER       The preferred testing method could not be used.
**/
STATIC
EFI_STATUS
DetermineTestMethod (
  IN  MEMORY_PROTECTION_TESTING_METHOD  PreferredTestingMethod,
  OUT MEMORY_PROTECTION_TESTING_METHOD  *TestingMethod
  )
{
  MEMORY_PROTECTION_TESTING_METHOD  DeterminedTestingMethod = MemoryProtectionTestMax;

  // Use a switch based on the preferred testing method. MemoryProtectionTestMax implies that there
  // is no perferred testing method in which case we will fall through the switch to find the first
  // available testing method based on the order in the description above. Otherwise, we will check
  // the testing method specified by PreferredTestingMethod.
  switch (PreferredTestingMethod) {
    default:
    case MemoryProtectionTestMax:
    case MemoryProtectionTestMemoryAttributeProtocol:
      // Check if the Memory Attribute Protocol is installed
      if (!EFI_ERROR (PopulateMemoryAttributeProtocol ())) {
        DeterminedTestingMethod = MemoryProtectionTestMemoryAttributeProtocol;
        break;
      }

    case MemoryProtectionTestClearFaults:
      // Check if the Project Mu page fault handler is installed. This handler will warm-reset on page faults
      // unless the Nonstop Protocol is installed to clear intentional page faults.
      if (!EFI_ERROR (CheckMemoryProtectionExceptionHandlerInstallation ())) {
        // Clear the memory protection early store in case a fault was previously tripped and was not cleared
        ExPersistClearAll ();

        // Check if a read/write to the early store works and the Nonstop Protocol is installed
        if (!EFI_ERROR (ExPersistSetIgnoreNextPageFault ()) &&
            !EFI_ERROR (ExPersistClearIgnoreNextPageFault ()) &&
            !EFI_ERROR (GetNonstopProtocol ()))
        {
          DeterminedTestingMethod = MemoryProtectionTestClearFaults;
          break;
        }
      }

    case MemoryProtectionTestReset:
      // Uninstall the existing page fault handler
      mCpu->RegisterInterruptHandler (mCpu, EXCEPT_IA32_PAGE_FAULT, NULL);

      // Install an interrupt handler to reboot on page faults.
      if (!EFI_ERROR (mCpu->RegisterInterruptHandler (mCpu, EXCEPT_IA32_PAGE_FAULT, InterruptHandler))) {
        DeterminedTestingMethod = MemoryProtectionTestReset;
        break;
      }
  }

  // DeterminedTestingMethod will be MemoryProtectionTestMax if none of the testing
  // methods were usable.
  if (DeterminedTestingMethod == MemoryProtectionTestMax) {
    DEBUG ((DEBUG_ERROR, "Could not find a suitable testing method.\n"));
    return EFI_UNSUPPORTED;
  }

  // If a preferred testing method was specified, make sure that the determined testing method
  // matches. Otherwise, return an error.
  if ((PreferredTestingMethod != MemoryProtectionTestMax) && (PreferredTestingMethod != DeterminedTestingMethod)) {
    DEBUG ((DEBUG_ERROR, "Could not use desired testing method.\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Print the testing method that will be used
  switch (DeterminedTestingMethod) {
    case MemoryProtectionTestReset:
      DEBUG ((DEBUG_INFO, "Testing with a reset after each protection violation.\n"));
      break;

    case MemoryProtectionTestMemoryAttributeProtocol:
      DEBUG ((DEBUG_INFO, "Testing with the Memory Attribute Protocol.\n"));
      break;

    case MemoryProtectionTestClearFaults:
      DEBUG ((DEBUG_INFO, "Testing with the Nonstop Protocol.\n"));
      break;

    default:
      // Should never get here
      DEBUG ((DEBUG_ERROR, "Invalid testing method.\n"));
      return EFI_INVALID_PARAMETER;
  }

  // Set the output parameter and return
  *TestingMethod = DeterminedTestingMethod;
  return EFI_SUCCESS;
}

/**
  MemoryProtectionTestAppEntryPoint

  Future Work:
  1. Enable running the reset method on ARM platforms by installing a synchronous handler.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
MemoryProtectionTestAppEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                        Status;
  UNIT_TEST_FRAMEWORK_HANDLE        Fw           = NULL;
  UNIT_TEST_SUITE_HANDLE            PageGuard    = NULL;
  UNIT_TEST_SUITE_HANDLE            PoolGuard    = NULL;
  UNIT_TEST_SUITE_HANDLE            NxProtection = NULL;
  UNIT_TEST_SUITE_HANDLE            Misc         = NULL;
  MEMORY_PROTECTION_TEST_CONTEXT    *MemoryProtectionContext;
  MEMORY_PROTECTION_TESTING_METHOD  TestingMethod;
  MEMORY_PROTECTION_TESTING_METHOD  PreferredTestingMethod;
  EFI_SHELL_PARAMETERS_PROTOCOL     *ShellParams;

  DEBUG ((DEBUG_ERROR, "%a()\n", __FUNCTION__));

  DEBUG ((DEBUG_ERROR, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  MemoryProtectionContext = (MEMORY_PROTECTION_TEST_CONTEXT *)AllocateZeroPool (sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
  if (MemoryProtectionContext == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test context failed.\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiShellParametersProtocolGuid,
                  (VOID **)&ShellParams
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Could not retrieve command line args!\n", __FUNCTION__));
    goto EXIT;
  }

  LocateSmmCommonCommBuffer ();

  Status = FetchMemoryProtectionHobEntries ();
  ASSERT_EFI_ERROR (Status);

  // Find the CPU Architecture Protocol
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&mCpu);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate gEfiCpuArchProtocolGuid. Status = %r\n", Status));
    goto EXIT;
  }

  // Set up the test framework for running the tests.
  Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  // Create separate test suites for Page, Pool, and NX tests. The Misc test suite is for stack guard
  // and null pointer testing.
  CreateUnitTestSuite (&Misc, Fw, "Stack Guard and Null Pointer Detection", "Security.HeapGuardMisc", NULL, NULL);
  CreateUnitTestSuite (&PageGuard, Fw, "Page Guard Tests", "Security.PageGuard", NULL, NULL);
  CreateUnitTestSuite (&PoolGuard, Fw, "Pool Guard Tests", "Security.PoolGuard", NULL, NULL);
  CreateUnitTestSuite (&NxProtection, Fw, "NX Protection Tests", "Security.NxProtection", NULL, NULL);

  if ((PageGuard == NULL) || (PoolGuard == NULL) || (NxProtection == NULL) || (Misc == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed in CreateUnitTestSuite for TestSuite\n", __FUNCTION__));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  PreferredTestingMethod = MemoryProtectionTestMax;

  // Check the command line arguments to see if a preferred testing method was specified.
  if (ShellParams->Argc > 1) {
    if (StrnCmp (ShellParams->Argv[1], UNIT_TEST_WARM_RESET_STRING, StrLen (UNIT_TEST_WARM_RESET_STRING)) == 0) {
      PreferredTestingMethod = MemoryProtectionTestReset;
    } else if (StrnCmp (ShellParams->Argv[1], UNIT_TEST_MEMORY_ATTRIBUTE_STRING, StrLen (UNIT_TEST_MEMORY_ATTRIBUTE_STRING)) == 0) {
      PreferredTestingMethod = MemoryProtectionTestMemoryAttributeProtocol;
    } else if (StrnCmp (ShellParams->Argv[1], UNIT_TEST_CLEAR_FAULTS_STRING, StrLen (UNIT_TEST_CLEAR_FAULTS_STRING)) == 0) {
      PreferredTestingMethod = MemoryProtectionTestClearFaults;
    } else {
      if (StrnCmp (ShellParams->Argv[1], L"-h", 4) != 0) {
        DEBUG ((DEBUG_INFO, "Invalid argument!\n\n"));
      }

      DEBUG ((DEBUG_INFO, "--Reset : Attempt to run the test by violating memory protections and performing a warm reset on faults.\n"));
      DEBUG ((DEBUG_INFO, "--MemoryAttribute : Attempt to run the test by using the memory attribute protocol to check attributes.\n"));
      DEBUG ((DEBUG_INFO, "--ClearFaults : Attempt to run the test by violating memory protections and expecting the exception handler to clear the faults.\n"));

      Status = EFI_ABORTED;
      goto EXIT;
    }
  }

  // Determine the testing method to use.
  if (EFI_ERROR (DetermineTestMethod (PreferredTestingMethod, &TestingMethod))) {
    goto EXIT;
  }

  // Set the testing method in the test context.
  MemoryProtectionContext->TestingMethod = TestingMethod;

  // Add a unit test for each memory type for pool, page, and NX protection.
  AddUefiPoolTest (PoolGuard, TestingMethod);
  AddUefiPageTest (PageGuard, TestingMethod);
  AddSmmPageTest (PageGuard);
  AddSmmPoolTest (PoolGuard);
  AddUefiNxTest (NxProtection, TestingMethod);

  // Add NULL protection, stack protection, and Image protection tests to the Misc test suite.
  AddTestCase (Misc, "Null pointer access should trigger a page fault", "Security.HeapGuardMisc.UefiNullPointerDetection", UefiNullPointerDetection, UefiNullPointerPreReq, NULL, MemoryProtectionContext);
  AddTestCase (Misc, "Null pointer access in SMM should trigger a page fault", "Security.HeapGuardMisc.SmmNullPointerDetection", SmmNullPointerDetection, SmmNullPointerPreReq, NULL, MemoryProtectionContext);
  AddTestCase (Misc, "Blowing the stack should trigger a page fault", "Security.HeapGuardMisc.UefiCpuStackGuard", UefiCpuStackGuard, UefiStackGuardPreReq, NULL, MemoryProtectionContext);
  AddTestCase (Misc, "Check that loaded images have proper attributes set", "Security.HeapGuardMisc.ImageProtectionEnabled", ImageProtection, ImageProtectionPreReq, NULL, MemoryProtectionContext);
  AddTestCase (NxProtection, "Check hardware configuration of HardwareNxProtection bit", "Security.HeapGuardMisc.UefiHardwareNxProtectionEnabled", UefiHardwareNxProtectionEnabled, UefiHardwareNxProtectionEnabledPreReq, NULL, MemoryProtectionContext);
  AddTestCase (NxProtection, "Stack NX Protection", "Security.HeapGuardMisc.UefiNxStackGuard", UefiNxStackGuard, NULL, NULL, MemoryProtectionContext);

  // Execute the tests.
  Status = RunAllTestSuites (Fw);

EXIT:

  if (Fw) {
    FreeUnitTestFramework (Fw);
  }

  if (MemoryProtectionContext) {
    FreePool (MemoryProtectionContext);
  }

  return Status;
}
