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

#define UNIT_TEST_APP_NAME     "Heap Guard Test"
#define UNIT_TEST_APP_VERSION  "0.5"

#define DUMMY_FUNCTION_FOR_CODE_SELF_TEST_GENERIC_SIZE  512

VOID                                     *mPiSmmCommonCommBufferAddress = NULL;
UINTN                                    mPiSmmCommonCommBufferSize;
EFI_CPU_ARCH_PROTOCOL                    *mCpu = NULL;
MM_MEMORY_PROTECTION_SETTINGS            mMmMps;
DXE_MEMORY_PROTECTION_SETTINGS           mDxeMps;
MEMORY_PROTECTION_NONSTOP_MODE_PROTOCOL  *mNonstopModeProtocol      = NULL;
MEMORY_PROTECTION_DEBUG_PROTOCOL         *mMemoryProtectionProtocol = NULL;
EFI_MEMORY_ATTRIBUTE_PROTOCOL            *mMemoryAttributeProtocol  = NULL;

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
  // We need to avoid using runtime services to reset the system due because doing so will raise the TPL level
  // when it is already on TPL_HIGH. HwResetSystemLib is used here instead to perform a bare-metal reset and sidestep
  // this issue.
  ResetWarm ();
} // InterruptHandler()

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
  if (mNonstopModeProtocol == NULL) {
    if (EFI_ERROR (
          gBS->LocateProtocol (
                 &gMemoryProtectionNonstopModeProtocolGuid,
                 NULL,
                 (VOID **)&mNonstopModeProtocol
                 )
          ))
    {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
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

  if (EFI_ERROR (
        gBS->LocateProtocol (
               &gMemoryProtectionExceptionHandlerGuid,
               NULL,
               (VOID **)&DummyProtocol
               )
        ))
  {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
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

  //
  // First, let's zero the comm buffer. Couldn't hurt.
  //
  CommHeader     = (EFI_SMM_COMMUNICATE_HEADER *)mPiSmmCommonCommBufferAddress;
  CommBufferSize = sizeof (MEMORY_PROTECTION_TEST_COMM_BUFFER) + OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data);
  if (CommBufferSize > mPiSmmCommonCommBufferSize) {
    DEBUG ((DEBUG_ERROR, "%a - Communication buffer is too small!\n", __FUNCTION__));
    return EFI_ABORTED;
  }

  ZeroMem (CommHeader, CommBufferSize);

  //
  // Update some parameters.
  //
  // SMM Communication Parameters
  CopyGuid (&CommHeader->HeaderGuid, &gMemoryProtectionTestSmiHandlerGuid);
  CommHeader->MessageLength = sizeof (MEMORY_PROTECTION_TEST_COMM_BUFFER);

  // Parameters Specific to this Implementation
  VerificationCommBuffer           = (MEMORY_PROTECTION_TEST_COMM_BUFFER *)CommHeader->Data;
  VerificationCommBuffer->Function = RequestedFunction;
  VerificationCommBuffer->Status   = EFI_NOT_FOUND;
  CopyMem (&VerificationCommBuffer->Context, Context, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

  //
  // Locate the protocol, if not done yet.
  //
  if (!SmmCommunication) {
    Status = gBS->LocateProtocol (&gEfiSmmCommunicationProtocolGuid, NULL, (VOID **)&SmmCommunication);
  }

  //
  // Signal SMM.
  //
  if (!EFI_ERROR (Status)) {
    Status = SmmCommunication->Communicate (
                                 SmmCommunication,
                                 CommHeader,
                                 &CommBufferSize
                                 );
    DEBUG ((DEBUG_VERBOSE, "%a - Communicate() = %r\n", __FUNCTION__, Status));
  }

  return VerificationCommBuffer->Status;
} // SmmMemoryProtectionsDxeToSmmCommunicate()

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

    // We only need a region large enough to hold a MEMORY_PROTECTION_TEST_COMM_BUFFER,
    // so this shouldn't be too hard.
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
} // LocateSmmCommonCommBuffer()

STATIC
UINT64
Recursion (
  UINT64  Count
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

STATIC
UINT64
RecursionDynamic (
  UINT64  Count
  )
{
  UINT64  Sum = 0;

  DEBUG ((DEBUG_ERROR, "%a - 0x%x\n", __FUNCTION__, Count));

  if (GetIgnoreNextEx ()) {
    Sum = RecursionDynamic (++Count);
  }

  return Sum + Count;
}

VOID
PoolTest (
  IN UINT64  *ptr,
  IN UINT64  AllocationSize
  )
{
  UINT64  *ptrLoc;

  DEBUG ((DEBUG_INFO, "%a - Allocated pool at 0x%p\n", __FUNCTION__, ptr));

  //
  // Check if guard page is going to be at the head or tail.
  //
  if (mDxeMps.HeapGuardPolicy.Fields.Direction == HEAP_GUARD_ALIGNED_TO_TAIL) {
    //
    // Get to the beginning of the page the pool tail is on.
    //
    ptrLoc = (UINT64 *)(((UINTN)ptr) + (UINTN)AllocationSize);
    ptrLoc = ALIGN_POINTER (ptrLoc, 0x1000);

    //
    // The guard page will be on the next page.
    //
    ptr = (UINT64 *)(((UINTN)ptr) + 0x1000);
  } else {
    //
    // Get to the beginning of the page the pool head is on.
    //
    ptrLoc = ALIGN_POINTER (ptr, 0x1000);

    //
    // The guard page will be immediately preceding the page the pool starts on.
    //
    ptrLoc = (UINT64 *)(((UINTN)ptrLoc) - 0x1);
  }

  DEBUG ((DEBUG_ERROR, "%a - Writing to 0x%p\n", __FUNCTION__, ptrLoc));
  *ptrLoc = 1;
} // PoolTest()

VOID
HeadPageTest (
  IN UINT64  *ptr
  )
{
  // Hit the head guard page
  ptr = (UINT64 *)(((UINTN)ptr) - 0x1);
  DEBUG ((DEBUG_ERROR, "%a - Writing to 0x%p\n", __FUNCTION__, ptr));
  *ptr = 1;
} // HeadPageTest()

VOID
TailPageTest (
  IN UINT64  *ptr
  )
{
  // Hit the tail guard page
  ptr = (UINT64 *)(((UINTN)ptr) + 0x1000);
  DEBUG ((DEBUG_ERROR, "%a - Writing to 0x%p\n", __FUNCTION__, ptr));
  *ptr = 1;
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
  volatile UINT8  DontCompileMeOut = 0;

  DontCompileMeOut++;
  return;
} // DummyFunctionForCodeSelfTest()

/// ================================================================================================
/// ================================================================================================
///
/// PRE REQ FUNCTIONS
///
/// ================================================================================================
/// ================================================================================================

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

UNIT_TEST_STATUS
EFIAPI
ImageProtectionPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  if (mDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromFv || mDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromUnknown) {
    return UNIT_TEST_PASSED;
  }

  return UNIT_TEST_SKIPPED;
}

/* SetNxForStack is deprecated. This function is left here in case the setting returns.
UNIT_TEST_STATUS
EFIAPI
UefiNxStackPreReq (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  if (!mDxeMps.SetNxForStack) {
    return UNIT_TEST_SKIPPED;
  }
  if (UefiHardwareNxProtectionEnabled(Context) != UNIT_TEST_PASSED) {
    UT_LOG_WARNING("HardwareNxProtection bit not on. NX Test would not be accurate.");
    return UNIT_TEST_SKIPPED;
  }
  return UNIT_TEST_PASSED;
} // UefiNxStackPreReq()
*/
UNIT_TEST_STATUS
EFIAPI
UefiNxProtectionPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);

  UT_ASSERT_TRUE (MemoryProtectionContext.TargetMemoryType < MAX_UINTN);
  if (!GetDxeMemoryTypeSettingFromBitfield ((EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, mDxeMps.NxProtectionPolicy)) {
    UT_LOG_WARNING ("Protection for this memory type is disabled: %a", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  // Skip memory types which cannot be allocated
  if ((MemoryProtectionContext.TargetMemoryType == MEMORY_TYPE_CONVENTIONAL) ||
      (MemoryProtectionContext.TargetMemoryType == MEMORY_TYPE_PERSISTENT))
  {
    UT_LOG_WARNING ("Skipping test of memory type %a -- memory type cannot be allocated", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  if (UefiHardwareNxProtectionEnabled (Context) != UNIT_TEST_PASSED) {
    UT_LOG_WARNING ("HardwareNxProtection bit not on. NX Test would not be accurate.");
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
} // UefiNxProtectionPreReq()

UNIT_TEST_STATUS
EFIAPI
UefiPageGuardPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);

  UT_ASSERT_TRUE (MemoryProtectionContext.TargetMemoryType < MAX_UINTN);
  if (!(mDxeMps.HeapGuardPolicy.Fields.UefiPageGuard &&
        GetDxeMemoryTypeSettingFromBitfield ((EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, mDxeMps.HeapGuardPageType)))
  {
    UT_LOG_WARNING ("Protection for this memory type is disabled: %a", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  // Skip memory types which cannot be allocated
  if ((MemoryProtectionContext.TargetMemoryType == MEMORY_TYPE_CONVENTIONAL) ||
      (MemoryProtectionContext.TargetMemoryType == MEMORY_TYPE_PERSISTENT))
  {
    UT_LOG_WARNING ("Skipping test of memory type %a -- memory type cannot be allocated", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
} // UefiPageGuardPreReq()

UNIT_TEST_STATUS
EFIAPI
UefiPoolGuardPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);

  UT_ASSERT_TRUE (MemoryProtectionContext.TargetMemoryType < MAX_UINTN);
  if (!(mDxeMps.HeapGuardPolicy.Fields.UefiPoolGuard &&
        GetDxeMemoryTypeSettingFromBitfield ((EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, mDxeMps.HeapGuardPoolType)))
  {
    UT_LOG_WARNING ("Protection for this memory type is disabled: %a", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  // Skip memory types which cannot be allocated
  if ((MemoryProtectionContext.TargetMemoryType == MEMORY_TYPE_CONVENTIONAL) ||
      (MemoryProtectionContext.TargetMemoryType == MEMORY_TYPE_PERSISTENT))
  {
    UT_LOG_WARNING ("Skipping test of memory type %a -- memory type cannot be allocated", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
} // UefiPoolGuardPreReq()

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
} // UefiStackGuardPreReq()

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
} // UefiNullPointerPreReq

// UNIT_TEST_STATUS
// EFIAPI
// SmmNxProtectionPreReq (
//   IN UNIT_TEST_CONTEXT  Context
//   )
// {
//   MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);

//   UT_ASSERT_TRUE (MemoryProtectionContext.TargetMemoryType < MAX_UINTN);
//   if (!GetMmMemoryTypeSettingFromBitfield ((EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, mMmMps.NxProtectionPolicy)) {
//     UT_LOG_WARNING ("Protection for this memory type is disabled: %a", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
//     return UNIT_TEST_SKIPPED;
//   }

//   if (UefiHardwareNxProtectionEnabled (Context) != UNIT_TEST_PASSED) {
//     UT_LOG_WARNING ("HardwareNxProtection bit not on. NX Test would not be accurate.");
//     return UNIT_TEST_SKIPPED;
//   }

//   return UNIT_TEST_PASSED;
// } // SmmNxProtectionPreReq()

UNIT_TEST_STATUS
EFIAPI
SmmPageGuardPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);

  UT_ASSERT_TRUE (MemoryProtectionContext.TargetMemoryType < MAX_UINTN);
  if (!(mMmMps.HeapGuardPolicy.Fields.MmPageGuard &&
        GetMmMemoryTypeSettingFromBitfield ((EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, mMmMps.HeapGuardPageType)))
  {
    UT_LOG_WARNING ("Protection for this memory type is disabled: %a", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  // Skip memory types which cannot be allocated
  if ((MemoryProtectionContext.TargetMemoryType == MEMORY_TYPE_CONVENTIONAL) ||
      (MemoryProtectionContext.TargetMemoryType == MEMORY_TYPE_PERSISTENT))
  {
    UT_LOG_WARNING ("Skipping test of memory type %a -- memory type cannot be allocated", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
} // SmmPageGuardPreReq()

UNIT_TEST_STATUS
EFIAPI
SmmPoolGuardPreReq (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);

  UT_ASSERT_TRUE (MemoryProtectionContext.TargetMemoryType < MAX_UINTN);
  if (!(mMmMps.HeapGuardPolicy.Fields.MmPoolGuard &&
        GetMmMemoryTypeSettingFromBitfield ((EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, mMmMps.HeapGuardPoolType)))
  {
    UT_LOG_WARNING ("Protection for this memory type is disabled: %a", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  // Skip memory types which cannot be allocated
  if ((MemoryProtectionContext.TargetMemoryType == MEMORY_TYPE_CONVENTIONAL) ||
      (MemoryProtectionContext.TargetMemoryType == MEMORY_TYPE_PERSISTENT))
  {
    UT_LOG_WARNING ("Skipping test of memory type %a -- memory type cannot be allocated", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
} // SmmPoolGuardPreReq()

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
} // SmmNullPointerPreReq()

/// ================================================================================================
/// ================================================================================================
///
/// TEST CASES
///
/// ================================================================================================
/// ================================================================================================

UNIT_TEST_STATUS
EFIAPI
UefiPageGuard (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);
  EFI_PHYSICAL_ADDRESS            ptr;
  EFI_STATUS                      Status;

  DEBUG ((DEBUG_INFO, "%a - Testing Type: %a\n", __FUNCTION__, MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]));

  if (MemoryProtectionContext.DynamicActive) {
    if (mNonstopModeProtocol == NULL) {
      UT_ASSERT_NOT_EFI_ERROR (GetNonstopProtocol ());
    }

    Status = gBS->AllocatePages (AllocateAnyPages, (EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, 1, (EFI_PHYSICAL_ADDRESS *)&ptr);
    if (EFI_ERROR (Status)) {
      UT_LOG_WARNING ("Memory allocation failed for type %a - %r\n", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType], Status);
      return UNIT_TEST_SKIPPED;
    }

    UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());

    // Hit the head guard page
    HeadPageTest ((UINT64 *)(UINTN)ptr);

    if (GetIgnoreNextEx ()) {
      UT_LOG_ERROR ("Head guard page failed: %p", ptr);
      UT_ASSERT_FALSE (GetIgnoreNextEx ());
    }

    UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());
    UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());

    // Hit the tail guard page
    TailPageTest ((UINT64 *)(UINTN)ptr);

    if (GetIgnoreNextEx ()) {
      UT_LOG_ERROR ("Tail guard page failed: %p", ptr);
      UT_ASSERT_FALSE (GetIgnoreNextEx ());
    }

    UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());

    return UNIT_TEST_PASSED;
  }

  if (MemoryProtectionContext.TestProgress < 2) {
    //
    // Context.TestProgress indicates progress within this specific test.
    // 0 - Just started.
    // 1 - Completed head guard test.
    // 2 - Completed tail guard test.
    //
    // We need to indicate we are working on the next part of the test and save our progress.
    //
    MemoryProtectionContext.TestProgress++;
    SetBootNextDevice ();
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

    Status = gBS->AllocatePages (AllocateAnyPages, (EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, 1, (EFI_PHYSICAL_ADDRESS *)&ptr);

    if (EFI_ERROR (Status)) {
      UT_LOG_WARNING ("Memory allocation failed for type %a - %r\n", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType], Status);
      return UNIT_TEST_SKIPPED;
    } else if (MemoryProtectionContext.TestProgress == 1) {
      DEBUG ((DEBUG_ERROR, "%a - Allocated page at 0x%p\n", __FUNCTION__, ptr));

      // Hit the head guard page
      HeadPageTest ((UINT64 *)(UINTN)ptr);

      //
      // Anything executing past this point indicates a failure.
      //
      UT_LOG_ERROR ("Head guard page failed: %p", ptr);
    } else {
      DEBUG ((DEBUG_ERROR, "%a - Allocated page at 0x%p\n", __FUNCTION__, ptr));

      // Hit the tail guard page
      TailPageTest ((UINT64 *)(UINTN)ptr);

      //
      // Anything executing past this point indicates a failure.
      //
      UT_LOG_ERROR ("Tail guard page failed: %p", ptr);
    }

    //
    // Reset test progress so failure gets recorded.
    //
    MemoryProtectionContext.TestProgress = 0;
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
  }

  UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == 2);

  return UNIT_TEST_PASSED;
} // UefiPageGuard()

UNIT_TEST_STATUS
EFIAPI
UefiPoolGuard (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);
  UINT64                          *ptr;
  EFI_STATUS                      Status;
  UINTN                           AllocationSize;
  UINT8                           Index = 0;

  DEBUG ((DEBUG_INFO, "%a - Testing Type: %a\n", __FUNCTION__, MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]));

  if (MemoryProtectionContext.DynamicActive) {
    if (mNonstopModeProtocol == NULL) {
      UT_ASSERT_NOT_EFI_ERROR (GetNonstopProtocol ());
    }

    for (Index = 0; Index < NUM_POOL_SIZES; Index++) {
      UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());

      //
      // Context.TestProgress indicates progress within this specific test.
      // The test progressively allocates larger areas to test the guard on.
      // These areas are defined in Pool.c as the 13 different sized chunks that are available
      // for pool allocation.
      //
      // We need to indicate we are working on the next part of the test and save our progress.
      //
      AllocationSize = mPoolSizeTable[Index];

      //
      // Memory type rHardwareNxProtections to the bitmask for the PcdHeapGuardPoolType,
      // we need to RShift 1 to get it to reflect the correct EFI_MEMORY_TYPE.
      //
      Status = gBS->AllocatePool ((EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, AllocationSize, (VOID **)&ptr);

      if (EFI_ERROR (Status)) {
        UT_LOG_WARNING ("Memory allocation failed for type %a of size %x - %r\n", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType], AllocationSize, Status);
        return UNIT_TEST_SKIPPED;
      }

      PoolTest ((UINT64 *)ptr, AllocationSize);

      UT_ASSERT_FALSE (GetIgnoreNextEx ());
      UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());
    }

    return UNIT_TEST_PASSED;
  }

  if (MemoryProtectionContext.TestProgress < NUM_POOL_SIZES) {
    //
    // Context.TestProgress indicates progress within this specific test.
    // The test progressively allocates larger areas to test the guard on.
    // These areas are defined in Pool.c as the 13 different sized chunks that are available
    // for pool allocation.
    //
    // We need to indicate we are working on the next part of the test and save our progress.
    //
    AllocationSize = mPoolSizeTable[MemoryProtectionContext.TestProgress];
    MemoryProtectionContext.TestProgress++;
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    SetBootNextDevice ();

    //
    // Memory type rHardwareNxProtections to the bitmask for the PcdHeapGuardPoolType,
    // we need to RShift 1 to get it to reflect the correct EFI_MEMORY_TYPE.
    //
    Status = gBS->AllocatePool ((EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, AllocationSize, (VOID **)&ptr);

    if (EFI_ERROR (Status)) {
      UT_LOG_WARNING ("Memory allocation failed for type %a of size %x - %r\n", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType], AllocationSize, Status);
      return UNIT_TEST_SKIPPED;
    } else {
      PoolTest ((UINT64 *)ptr, AllocationSize);

      //
      // At this point, the test has failed. Reset test progress so failure gets recorded.
      //
      MemoryProtectionContext.TestProgress = 0;
      SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
      UT_LOG_ERROR ("Pool guard failed: %p", ptr);
    }
  }

  UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == NUM_POOL_SIZES);

  return UNIT_TEST_PASSED;
} // UefiPoolGuard()

UNIT_TEST_STATUS
EFIAPI
UefiCpuStackGuard (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);

  DEBUG ((DEBUG_INFO, "%a - Testing CPU Stack Guard\n", __FUNCTION__));

  if (MemoryProtectionContext.DynamicActive) {
    if (mNonstopModeProtocol == NULL) {
      UT_ASSERT_NOT_EFI_ERROR (GetNonstopProtocol ());
    }

    UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());

    RecursionDynamic (1);

    UT_ASSERT_FALSE (GetIgnoreNextEx ());
    UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());

    return UNIT_TEST_PASSED;
  }

  if (MemoryProtectionContext.TestProgress < 1) {
    //
    // Context.TestProgress 0 indicates the test hasn't started yet.
    //
    // We need to indicate we are working on the test and save our progress.
    //
    MemoryProtectionContext.TestProgress++;
    SetBootNextDevice ();
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

    Recursion (1);

    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    MemoryProtectionContext.TestProgress = 0;
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    UT_LOG_ERROR ("System was expected to reboot, but didn't.");
  }

  UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == 1);

  return UNIT_TEST_PASSED;
} // UefiCpuStackGuard()

volatile UNIT_TEST_FRAMEWORK  *mFw = NULL;
UNIT_TEST_STATUS
EFIAPI
UefiNullPointerDetection (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8                           Index;
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);

  DEBUG ((DEBUG_INFO, "%a - Testing NULL Pointer Detection\n", __FUNCTION__));

  if (MemoryProtectionContext.DynamicActive) {
    if (mNonstopModeProtocol == NULL) {
      UT_ASSERT_NOT_EFI_ERROR (GetNonstopProtocol ());
    }

    for (Index = 0; Index < 2; Index++) {
      UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());
      UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());

      if (Index < 1) {
        if (mFw->Title == NULL) {
          continue;
        }
      } else {
        mFw->Title = "Title";
      }

      if (GetIgnoreNextEx ()) {
        if (Index < 1) {
          UT_LOG_ERROR ("Failed NULL pointer read test.");
        } else {
          UT_LOG_ERROR ("Failed NULL pointer write test.");
        }

        UT_ASSERT_FALSE (GetIgnoreNextEx ());
      }
    }

    UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());

    return UNIT_TEST_PASSED;
  }

  if (MemoryProtectionContext.TestProgress < 2) {
    //
    // Context.TestProgress indicates progress within this specific test.
    // 0 - Just started.
    // 1 - Completed NULL pointer read test.
    // 2 - Completed NULL pointer write test.
    //
    // We need to indicate we are working on the next part of the test and save our progress.
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

  return UNIT_TEST_PASSED;
} // UefiNullPointerDetection()

UNIT_TEST_STATUS
EFIAPI
UefiNxStackGuard (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);
  UINT8                           CodeRegionToCopyTo[DUMMY_FUNCTION_FOR_CODE_SELF_TEST_GENERIC_SIZE];
  UINT8                           *CodeRegionToCopyFrom = (UINT8 *)DummyFunctionForCodeSelfTest;

  DEBUG ((DEBUG_INFO, "%a - NX Stack Guard\n", __FUNCTION__));

  if (MemoryProtectionContext.DynamicActive) {
    if (mNonstopModeProtocol == NULL) {
      UT_ASSERT_NOT_EFI_ERROR (GetNonstopProtocol ());
    }

    UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());

    CopyMem (CodeRegionToCopyTo, CodeRegionToCopyFrom, DUMMY_FUNCTION_FOR_CODE_SELF_TEST_GENERIC_SIZE);
    ((DUMMY_VOID_FUNCTION_FOR_DATA_TEST)CodeRegionToCopyTo)();

    UT_ASSERT_FALSE (GetIgnoreNextEx ());
    UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());

    return UNIT_TEST_PASSED;
  }

  if (MemoryProtectionContext.TestProgress < 1) {
    //
    // Context.TestProgress 0 indicates the test hasn't started yet.
    //
    // We need to indicate we are working on the test and save our progress.
    //
    MemoryProtectionContext.TestProgress++;
    SetBootNextDevice ();
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

    CopyMem (CodeRegionToCopyTo, CodeRegionToCopyFrom, DUMMY_FUNCTION_FOR_CODE_SELF_TEST_GENERIC_SIZE);
    ((DUMMY_VOID_FUNCTION_FOR_DATA_TEST)CodeRegionToCopyTo)();

    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    MemoryProtectionContext.TestProgress = 0;
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    UT_LOG_ERROR ("NX Stack Guard Test failed.");
  }

  UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == 1);

  return UNIT_TEST_PASSED;
} // UefiNxStackGuard()

UNIT_TEST_STATUS
EFIAPI
UefiNxProtection (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);
  UINT64                          *ptr;
  EFI_STATUS                      Status;
  UINT8                           *CodeRegionToCopyFrom = (UINT8 *)DummyFunctionForCodeSelfTest;

  DEBUG ((DEBUG_INFO, "%a - Testing Type: %a\n", __FUNCTION__, MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]));

  if (MemoryProtectionContext.DynamicActive) {
    if (mNonstopModeProtocol == NULL) {
      UT_ASSERT_NOT_EFI_ERROR (GetNonstopProtocol ());
    }

    UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());
    UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());

    Status = gBS->AllocatePool ((EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, EFI_PAGE_SIZE, (VOID **)&ptr);

    if (EFI_ERROR (Status)) {
      UT_LOG_ERROR ("Memory allocation failed for type %a - %r\n", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType], Status);
      return UNIT_TEST_SKIPPED;
    } else {
      CopyMem (ptr, CodeRegionToCopyFrom, DUMMY_FUNCTION_FOR_CODE_SELF_TEST_GENERIC_SIZE);
      ((DUMMY_VOID_FUNCTION_FOR_DATA_TEST)ptr)();
    }

    FreePool (ptr);

    UT_ASSERT_FALSE (GetIgnoreNextEx ());
    UT_ASSERT_NOT_EFI_ERROR (mNonstopModeProtocol->ResetPageAttributes ());

    return UNIT_TEST_PASSED;
  }

  if (MemoryProtectionContext.TestProgress < 1) {
    //
    // Context.TestProgress 0 indicates the test hasn't started yet.
    //
    // We need to indicate we are working on the test and save our progress.
    //
    MemoryProtectionContext.TestProgress++;
    SetBootNextDevice ();
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

    Status = gBS->AllocatePool ((EFI_MEMORY_TYPE)MemoryProtectionContext.TargetMemoryType, EFI_PAGE_SIZE, (VOID **)&ptr);

    if (EFI_ERROR (Status)) {
      UT_LOG_WARNING ("Memory allocation failed for type %a - %r\n", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType], Status);
      return UNIT_TEST_SKIPPED;
    } else {
      CopyMem (ptr, CodeRegionToCopyFrom, DUMMY_FUNCTION_FOR_CODE_SELF_TEST_GENERIC_SIZE);
      ((DUMMY_VOID_FUNCTION_FOR_DATA_TEST)ptr)();
    }

    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    MemoryProtectionContext.TestProgress = 0;
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    UT_LOG_ERROR ("NX Test failed.");
  }

  UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == 1);

  return UNIT_TEST_PASSED;
} // UefiNxProtection()

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

  UT_ASSERT_NOT_EFI_ERROR (PopulateMemoryProtectionDebugProtocol ());
  if (mMemoryProtectionProtocol != NULL) {
    UT_ASSERT_NOT_EFI_ERROR (mMemoryProtectionProtocol->GetImageList (&ImageRangeDescriptorHead, Protected));
  }

  UT_ASSERT_NOT_EFI_ERROR (PopulateMemoryAttributeProtocol ());

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
      Status = mMemoryAttributeProtocol->GetMemoryAttributes (mMemoryAttributeProtocol, ImageRangeDescriptor->Base, ImageRangeDescriptor->Length, &Attributes);

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

  UT_ASSERT_FALSE (TestFailed);

  return UNIT_TEST_PASSED;
}

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
    // We need to indicate we are working on the next part of the test and save our progress.
    //
    MemoryProtectionContext.TestProgress++;
    SetBootNextDevice ();
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

    Status = SmmMemoryProtectionsDxeToSmmCommunicate (MEMORY_PROTECTION_TEST_PAGE, &MemoryProtectionContext);
    if (Status == EFI_NOT_FOUND) {
      UT_LOG_WARNING ("SMM test driver is not loaded.");
      return UNIT_TEST_SKIPPED;
    } else {
      UT_LOG_ERROR ("System was expected to reboot, but didn't.");
    }

    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    MemoryProtectionContext.TestProgress = 0;
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
  }

  UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == 2);

  return UNIT_TEST_PASSED;
} // SmmPageGuard()

UNIT_TEST_STATUS
EFIAPI
SmmPoolGuard (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MEMORY_PROTECTION_TEST_CONTEXT  MemoryProtectionContext = (*(MEMORY_PROTECTION_TEST_CONTEXT *)Context);
  EFI_STATUS                      Status;

  if (MemoryProtectionContext.TestProgress < NUM_POOL_SIZES) {
    //
    // Context.TestProgress indicates progress within this specific test.
    // The test progressively allocates larger areas to test the guard on.
    // These areas are defined in Pool.c as the 13 different sized chunks that are available
    // for pool allocation.
    //
    // We need to indicate we are working on the next part of the test and save our progress.
    //
    MemoryProtectionContext.TestProgress++;
    SetBootNextDevice ();
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

    Status = SmmMemoryProtectionsDxeToSmmCommunicate (MEMORY_PROTECTION_TEST_POOL, &MemoryProtectionContext);

    if (Status == EFI_NOT_FOUND) {
      UT_LOG_WARNING ("SMM test driver is not loaded.");
      return UNIT_TEST_SKIPPED;
    } else {
      UT_LOG_ERROR ("System was expected to reboot, but didn't.");
    }

    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    MemoryProtectionContext.TestProgress = 0;
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
  }

  UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == 1);

  return UNIT_TEST_PASSED;
} // SmmPoolGuard()

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
    // We need to indicate we are working on the test and save our progress.
    //
    MemoryProtectionContext.TestProgress++;
    SetBootNextDevice ();
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));

    Status = SmmMemoryProtectionsDxeToSmmCommunicate (MEMORY_PROTECTION_TEST_NULL_POINTER, &MemoryProtectionContext);

    if (Status == EFI_NOT_FOUND) {
      UT_LOG_WARNING ("SMM test driver is not loaded.");
      return UNIT_TEST_SKIPPED;
    } else {
      UT_LOG_ERROR ("System was expected to reboot, but didn't. %r", Status);
    }

    //
    // At this point, the test has failed. Reset test progress so failure gets recorded.
    //
    MemoryProtectionContext.TestProgress = 0;
    SaveFrameworkState (&MemoryProtectionContext, sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
  }

  UT_ASSERT_TRUE (MemoryProtectionContext.TestProgress == 1);

  return UNIT_TEST_PASSED;
} // SmmNullPointerDetection()

/// ================================================================================================
/// ================================================================================================
///
/// TEST ENGINE
///
/// ================================================================================================
/// ================================================================================================
VOID
AddUefiNxTest (
  UNIT_TEST_SUITE_HANDLE  TestSuite
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
  BOOLEAN                         Dynamic = FALSE;

  DEBUG ((DEBUG_INFO, "%a() - Enter\n", __FUNCTION__));

  if (!EFI_ERROR (CheckMemoryProtectionExceptionHandlerInstallation ()) && !EFI_ERROR (GetNonstopProtocol ())) {
    Dynamic = TRUE;
  }

  //
  // Need to generate a test case for each type of memory.
  //
  for (Index = 0; Index < NUM_MEMORY_TYPES; Index++) {
    //
    // Set memory type according to the bitmask for Dxe NX Memory Protection Policy.
    // The test progress should start at 0.
    //
    MemoryProtectionContext =  (MEMORY_PROTECTION_TEST_CONTEXT *)AllocateZeroPool (sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    if (MemoryProtectionContext == NULL) {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test context failed.\n", __FUNCTION__));
      return;
    }

    MemoryProtectionContext->TargetMemoryType = Index;
    MemoryProtectionContext->GuardAlignment   = mDxeMps.HeapGuardPolicy.Fields.Direction;
    MemoryProtectionContext->DynamicActive    = Dynamic;

    TestNameSize = sizeof (CHAR8) * (1 + AsciiStrnLenS (NameStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName     = AllocateZeroPool (TestNameSize);

    TestDescriptionSize = sizeof (CHAR8) * (1 + AsciiStrnLenS (DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription     = (CHAR8 *)AllocateZeroPool (TestDescriptionSize);

    if ((TestName != NULL) && (TestDescription != NULL) && (MemoryProtectionContext != NULL)) {
      //
      // Name of the test is Security.PageGuard.Uefi + Memory Type Name (from MEMORY_TYPES)
      //
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      //
      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)

      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      AddTestCase (TestSuite, TestDescription, TestName, UefiNxProtection, UefiNxProtectionPreReq, NULL, MemoryProtectionContext);

      FreePool (TestName);
      FreePool (TestDescription);
    } else {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test creation failed.\n", __FUNCTION__));
      return;
    }
  }
} // AddUefiNxTest()

VOID
AddUefiPoolTest (
  UNIT_TEST_SUITE_HANDLE  TestSuite
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
  BOOLEAN                         Dynamic = FALSE;

  DEBUG ((DEBUG_INFO, "%a() - Enter\n", __FUNCTION__));

  if (!EFI_ERROR (CheckMemoryProtectionExceptionHandlerInstallation ()) && !EFI_ERROR (GetNonstopProtocol ())) {
    Dynamic = TRUE;
  }

  //
  // Need to generate a test case for each type of memory.
  //
  for (Index = 0; Index < NUM_MEMORY_TYPES; Index++) {
    //
    // Set memory type according to the Heap Guard Pool Type setting.
    // The test progress should start at 0.
    //
    MemoryProtectionContext =  (MEMORY_PROTECTION_TEST_CONTEXT *)AllocateZeroPool (sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    if (MemoryProtectionContext == NULL) {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test context failed.\n", __FUNCTION__));
      return;
    }

    MemoryProtectionContext->TargetMemoryType = Index;
    MemoryProtectionContext->GuardAlignment   = mDxeMps.HeapGuardPolicy.Fields.Direction;
    MemoryProtectionContext->DynamicActive    = Dynamic;

    //
    // Name of the test is Security.PoolGuard.Uefi + Memory Type Name (from MEMORY_TYPES)
    //
    TestNameSize = sizeof (CHAR8) * (1 + AsciiStrnLenS (NameStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName     = (CHAR8 *)AllocateZeroPool (TestNameSize);

    TestDescriptionSize = sizeof (CHAR8) * (1 + AsciiStrnLenS (DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription     = (CHAR8 *)AllocateZeroPool (TestDescriptionSize);

    if ((TestName != NULL) && (TestDescription != NULL) && (MemoryProtectionContext != NULL)) {
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      //
      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)

      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      AddTestCase (TestSuite, TestDescription, TestName, UefiPoolGuard, UefiPoolGuardPreReq, NULL, MemoryProtectionContext);

      FreePool (TestName);
      FreePool (TestDescription);
    } else {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test creation failed.\n", __FUNCTION__));
      return;
    }
  }
} // AddUefiPoolTest()

VOID
AddUefiPageTest (
  UNIT_TEST_SUITE_HANDLE  TestSuite
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
  BOOLEAN                         Dynamic = FALSE;

  DEBUG ((DEBUG_INFO, "%a() - Enter\n", __FUNCTION__));

  if (!EFI_ERROR (CheckMemoryProtectionExceptionHandlerInstallation ()) && !EFI_ERROR (GetNonstopProtocol ())) {
    Dynamic = TRUE;
  }

  //
  // Need to generate a test case for each type of memory.
  //
  for (Index = 0; Index < NUM_MEMORY_TYPES; Index++) {
    //
    // Set memory type according to the Heap Guard Page Type setting.
    // The test progress should start at 0.
    //
    MemoryProtectionContext =  (MEMORY_PROTECTION_TEST_CONTEXT *)AllocateZeroPool (sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    if (MemoryProtectionContext == NULL) {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test context failed.\n", __FUNCTION__));
      return;
    }

    MemoryProtectionContext->TargetMemoryType = Index;
    MemoryProtectionContext->GuardAlignment   = mDxeMps.HeapGuardPolicy.Fields.Direction;
    MemoryProtectionContext->DynamicActive    = Dynamic;

    TestNameSize = sizeof (CHAR8) * (1 + AsciiStrnLenS (NameStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName     = (CHAR8 *)AllocateZeroPool (TestNameSize);

    TestDescriptionSize = sizeof (CHAR8) * (1 + AsciiStrnLenS (DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription     = (CHAR8 *)AllocateZeroPool (TestDescriptionSize);

    if ((TestName != NULL) && (TestDescription != NULL) && (MemoryProtectionContext != NULL)) {
      //
      // Name of the test is Security.PageGuard.Uefi + Memory Type Name (from MEMORY_TYPES)
      //
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      //
      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)

      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      AddTestCase (TestSuite, TestDescription, TestName, UefiPageGuard, UefiPageGuardPreReq, NULL, MemoryProtectionContext);

      FreePool (TestName);
      FreePool (TestDescription);
    } else {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test creation failed.\n", __FUNCTION__));
      return;
    }
  }
} // AddUefiPageTest()

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

  //
  // Need to generate a test case for each type of memory.
  //
  for (Index = 0; Index < NUM_MEMORY_TYPES; Index++) {
    //
    // Set memory type according to the Heap Guard Pool Type setting.
    // The test progress should start at 0.
    //
    MemoryProtectionContext =  (MEMORY_PROTECTION_TEST_CONTEXT *)AllocateZeroPool (sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    if (MemoryProtectionContext == NULL) {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test context failed.\n", __FUNCTION__));
      return;
    }

    MemoryProtectionContext->TargetMemoryType = Index;
    MemoryProtectionContext->GuardAlignment   = mDxeMps.HeapGuardPolicy.Fields.Direction;

    TestNameSize = sizeof (CHAR8) * (1 + AsciiStrnLenS (NameStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName     = (CHAR8 *)AllocateZeroPool (TestNameSize);

    TestDescriptionSize = sizeof (CHAR8) * (1 + AsciiStrnLenS (DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription     = (CHAR8 *)AllocateZeroPool (TestDescriptionSize);

    if ((TestName != NULL) && (TestDescription != NULL) && (MemoryProtectionContext != NULL)) {
      //
      // Name of the test is Security.PoolGuard.Smm + Memory Type Name (from MEMORY_TYPES)
      //
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      //
      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)

      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      AddTestCase (TestSuite, TestDescription, TestName, SmmPoolGuard, SmmPoolGuardPreReq, NULL, MemoryProtectionContext);

      FreePool (TestName);
      FreePool (TestDescription);
    } else {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test creation failed.\n", __FUNCTION__));
      return;
    }
  }
} // AddSmmPoolTest()

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

  //
  // Need to generate a test case for each type of memory.
  //
  for (Index = 0; Index < NUM_MEMORY_TYPES; Index++) {
    //
    // Set memory type according to the Heap Guard Page Type setting.
    // The test progress should start at 0.
    //
    MemoryProtectionContext =  (MEMORY_PROTECTION_TEST_CONTEXT *)AllocateZeroPool (sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
    if (MemoryProtectionContext == NULL) {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test context failed.\n", __FUNCTION__));
      return;
    }

    MemoryProtectionContext->TargetMemoryType = Index;
    MemoryProtectionContext->GuardAlignment   = mDxeMps.HeapGuardPolicy.Fields.Direction;

    TestNameSize = sizeof (CHAR8) * (1 + AsciiStrnLenS (NameStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestName     = (CHAR8 *)AllocateZeroPool (TestNameSize);

    TestDescriptionSize = sizeof (CHAR8) * (1 + AsciiStrnLenS (DescriptionStub, UNIT_TEST_MAX_STRING_LENGTH) + AsciiStrnLenS (MEMORY_TYPES[Index], UNIT_TEST_MAX_STRING_LENGTH));
    TestDescription     = (CHAR8 *)AllocateZeroPool (TestDescriptionSize);

    if ((TestName != NULL) && (TestDescription != NULL) && (MemoryProtectionContext != NULL)) {
      //
      // Name of the test is Security.PageGuard.Smm + Memory Type Name (from MEMORY_TYPES)
      //
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, NameStub);
      AsciiStrCatS (TestName, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      //
      // Description of this test is DescriptionStub + Memory Type Name (from MEMORY_TYPES)

      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, DescriptionStub);
      AsciiStrCatS (TestDescription, UNIT_TEST_MAX_STRING_LENGTH, MEMORY_TYPES[Index]);

      AddTestCase (TestSuite, TestDescription, TestName, SmmPageGuard, SmmPageGuardPreReq, NULL, MemoryProtectionContext);

      FreePool (TestName);
      FreePool (TestDescription);
    } else {
      DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test creation failed.\n", __FUNCTION__));
      return;
    }
  }
} // AddSmmPageTest()

/**
  MemoryProtectionTestAppEntryPoint

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
  EFI_STATUS                      Status;
  UNIT_TEST_FRAMEWORK_HANDLE      Fw           = NULL;
  UNIT_TEST_SUITE_HANDLE          PageGuard    = NULL;
  UNIT_TEST_SUITE_HANDLE          PoolGuard    = NULL;
  UNIT_TEST_SUITE_HANDLE          NxProtection = NULL;
  UNIT_TEST_SUITE_HANDLE          Misc         = NULL;
  MEMORY_PROTECTION_TEST_CONTEXT  *MemoryProtectionContext;

  MemoryProtectionContext = (MEMORY_PROTECTION_TEST_CONTEXT *)AllocateZeroPool (sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
  if (MemoryProtectionContext == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test context failed.\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  MemoryProtectionContext->DynamicActive = FALSE;

  DEBUG ((DEBUG_ERROR, "%a()\n", __FUNCTION__));

  DEBUG ((DEBUG_ERROR, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  LocateSmmCommonCommBuffer ();

  Status = FetchMemoryProtectionHobEntries ();
  ASSERT_EFI_ERROR (Status);

  //
  // Find the CPU Arch protocol, we're going to install our own interrupt handler
  // with it later.
  //
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&mCpu);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate gEfiCpuArchProtocolGuid. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Create separate test suites for Page, Pool, NX tests.
  // Misc test suite for stack guard and null pointer.
  //
  CreateUnitTestSuite (&Misc, Fw, "Stack Guard and Null Pointer Detection", "Security.HeapGuardMisc", NULL, NULL);
  CreateUnitTestSuite (&PageGuard, Fw, "Page Guard Tests", "Security.PageGuard", NULL, NULL);
  CreateUnitTestSuite (&PoolGuard, Fw, "Pool Guard Tests", "Security.PoolGuard", NULL, NULL);
  CreateUnitTestSuite (&NxProtection, Fw, "NX Protection Tests", "Security.NxProtection", NULL, NULL);

  if ((PageGuard == NULL) || (PoolGuard == NULL) || (NxProtection == NULL) || (Misc == NULL)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for TestSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  // Check if the Project Mu page fault handler is installed. This handler will warm-reset on page faults
  // unless the Nonstop Protocol is installed to clear intentional page faults.
  if (!EFI_ERROR (CheckMemoryProtectionExceptionHandlerInstallation ())) {
    // Clear the memory protection early store in case a fault was previously tripped and was not cleared
    ExPersistClearAll ();

    // Check if the nonstop protocol is active
    if (!EFI_ERROR (GetNonstopProtocol ())) {
      MemoryProtectionContext->DynamicActive = TRUE;
    } else {
      DEBUG ((
        DEBUG_WARN,
        "MEMORY_PROTECTION_NONSTOP_MODE_PROTOCOL is not installed! Note that using "
        "the protocol with this test can reduce execution time by over 98%%.\n"
        ));
    }
  } else {
    // Uninstall the existing page fault handler
    mCpu->RegisterInterruptHandler (mCpu, EXCEPT_IA32_PAGE_FAULT, NULL);

    // Install an interrupt handler to reboot on page faults.
    if (EFI_ERROR (mCpu->RegisterInterruptHandler (mCpu, EXCEPT_IA32_PAGE_FAULT, InterruptHandler))) {
      DEBUG ((DEBUG_ERROR, "Failed to install interrupt handler. Status = %r\n", Status));
      goto EXIT;
    }
  }

  AddUefiPoolTest (PoolGuard);
  AddUefiPageTest (PageGuard);
  AddSmmPageTest (PageGuard);
  AddSmmPoolTest (PoolGuard);
  AddUefiNxTest (NxProtection);

  AddTestCase (Misc, "Null pointer access should trigger a page fault", "Security.HeapGuardMisc.UefiNullPointerDetection", UefiNullPointerDetection, UefiNullPointerPreReq, NULL, MemoryProtectionContext);
  AddTestCase (Misc, "Null pointer access in SMM should trigger a page fault", "Security.HeapGuardMisc.SmmNullPointerDetection", SmmNullPointerDetection, SmmNullPointerPreReq, NULL, MemoryProtectionContext);
  AddTestCase (Misc, "Blowing the stack should trigger a page fault", "Security.HeapGuardMisc.UefiCpuStackGuard", UefiCpuStackGuard, UefiStackGuardPreReq, NULL, MemoryProtectionContext);
  AddTestCase (Misc, "Check that loaded images have proper attributes set", "Security.HeapGuardMisc.ImageProtectionEnabled", ImageProtection, ImageProtectionPreReq, NULL, MemoryProtectionContext);
  AddTestCase (NxProtection, "Check hardware configuration of HardwareNxProtection bit", "Security.HeapGuardMisc.UefiHardwareNxProtectionEnabled", UefiHardwareNxProtectionEnabled, UefiHardwareNxProtectionEnabledPreReq, NULL, MemoryProtectionContext);
  AddTestCase (NxProtection, "Stack NX Protection", "Security.HeapGuardMisc.UefiNxStackGuard", UefiNxStackGuard, NULL, NULL, MemoryProtectionContext);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Fw);

EXIT:

  if (Fw) {
    FreeUnitTestFramework (Fw);
  }

  return Status;
} // MemoryProtectionTestAppEntryPoint()
