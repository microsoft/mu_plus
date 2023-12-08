/** @file -- SmmMemoryProtectionTestApp.c

Tests for page guard, pool guard, NX protections, stack guard, and null pointer detection.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Protocol/DebugSupport.h>
#include <Protocol/SmmCommunication.h>
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
#include <Library/HobLib.h>
#include <Library/ExceptionPersistenceLib.h>

#include <Guid/PiSmmCommunicationRegionTable.h>
#include <Guid/DxeMemoryProtectionSettings.h>
#include <Guid/MmMemoryProtectionSettings.h>

#include "../MemoryProtectionTestCommon.h"
#include "ArchSpecificFunctions.h"

#define UNIT_TEST_APP_NAME     "SMM Memory Protection Test"
#define UNIT_TEST_APP_VERSION  "3.0"

#define DUMMY_FUNCTION_FOR_CODE_SELF_TEST_GENERIC_SIZE  512
#define ALIGN_ADDRESS(Address)  (((Address) / EFI_PAGE_SIZE) * EFI_PAGE_SIZE)

UINTN                           mPiSmmCommonCommBufferSize;
MM_MEMORY_PROTECTION_SETTINGS   mMmMps;
DXE_MEMORY_PROTECTION_SETTINGS  mDxeMps;
VOID                            *mPiSmmCommonCommBufferAddress = NULL;

/// ================================================================================================
/// ================================================================================================
///
/// HELPER FUNCTIONS
///
/// ================================================================================================
/// ================================================================================================

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
  Abstraction layer which fetches the DXE and MM memory protection HOBs.

  @retval EFI_SUCCESS             Both HOB entries have been fetched
  @retval EFI_INVALID_PARAMETER   A HOB entry could not be found
**/
EFI_STATUS
STATIC
FetchMemoryProtectionHobEntries (
  VOID
  )
{
  VOID  *Ptr1;
  VOID  *Ptr2;

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
      CopyMem (&mDxeMps, GET_GUID_HOB_DATA (Ptr2), sizeof (DXE_MEMORY_PROTECTION_SETTINGS));
    }
  }

  return (Ptr1 != NULL && Ptr2 != NULL) ? EFI_SUCCESS : EFI_INVALID_PARAMETER;
}

/**
  This helper function actually sends the requested communication
  to the SMM driver.

  @param[in]  RequestedFunction   The test function to request the SMM driver run.
  @param[in]  Context             The context of the test.

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
    DEBUG ((DEBUG_INFO, "%a - Communicate() = %r\n", __FUNCTION__, Status));
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
  This dummy function definition is used to test no-execute protection on allocated buffers
  and the stack.
**/
typedef
VOID
(*DUMMY_VOID_FUNCTION_FOR_DATA_TEST)(
  VOID
  );

/// ================================================================================================
/// ================================================================================================
///
/// PRE REQ FUNCTIONS
///
/// ================================================================================================
/// ================================================================================================

/**
  This function checks if the MM page guard policy is active for the target memory type within Context. Testing
  page guards currently requires that buffers of the relevant memory type can be allocated. If the memory type
  is Conventional, Persistent, or Unaccepted, the test will be skipped as we cannot allocate those types.

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
    UT_LOG_WARNING ("Protection for this memory type is disabled: %a\n", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  // Skip memory types which cannot be allocated
  if ((MemoryProtectionContext.TargetMemoryType == EfiConventionalMemory) ||
      (MemoryProtectionContext.TargetMemoryType == EfiPersistentMemory)   ||
      (MemoryProtectionContext.TargetMemoryType == EfiUnacceptedMemoryType))
  {
    UT_LOG_WARNING ("Skipping test of memory type %a -- memory type cannot be allocated\n", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  return UNIT_TEST_PASSED;
}

/**
  This function checks if the MM pool guard policy is active for the target memory type within Context. Testing
  pool guards currently requires that buffers of the relevant memory type can be allocated. If the memory type
  is Conventional, Persistent, or Unaccepted the test will be skipped as we cannot allocate those types.

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
    UT_LOG_WARNING ("Protection for this memory type is disabled: %a\n", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
    return UNIT_TEST_SKIPPED;
  }

  // Skip memory types which cannot be allocated
  if ((MemoryProtectionContext.TargetMemoryType == EfiConventionalMemory) ||
      (MemoryProtectionContext.TargetMemoryType == EfiPersistentMemory)   ||
      (MemoryProtectionContext.TargetMemoryType == EfiUnacceptedMemoryType))
  {
    UT_LOG_WARNING ("Skipping test of memory type %a -- memory type cannot be allocated\n", MEMORY_TYPES[MemoryProtectionContext.TargetMemoryType]);
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
    UT_LOG_WARNING ("This feature is disabled\n");
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
      UT_LOG_WARNING ("SMM test driver is not loaded.\n");
      return UNIT_TEST_SKIPPED;
    } else {
      UT_LOG_ERROR ("System was expected to reboot, but didn't.\n");
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
      UT_LOG_WARNING ("SMM test driver is not loaded.\n");
      return UNIT_TEST_SKIPPED;
    } else {
      UT_LOG_ERROR ("System was expected to reboot, but didn't.\n");
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
      UT_LOG_WARNING ("SMM test driver is not loaded.\n");
      return UNIT_TEST_SKIPPED;
    } else {
      UT_LOG_ERROR ("System was expected to reboot, but didn't. %r\n", Status);
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
  This function adds an MM test case for each memory type with pool guards enabled.

  Future Work:
  1. Update the SMM testing structure to allocate the tested pools in SMM so the MM guard
     alignment setting can be used.

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

  Future Work:
  1. Update the SMM testing structure to allocate the tested pages in SMM so the MM guard
     alignment setting can be used.

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

    // Because the pages/pools will be allocated in the UEFI context, use the DXE guard direction
    MemoryProtectionContext->GuardAlignment = mDxeMps.HeapGuardPolicy.Fields.Direction;

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
  MemoryProtectionTestAppEntryPoint

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
SmmMemoryProtectionTestAppEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                      Status;
  UNIT_TEST_FRAMEWORK_HANDLE      Fw        = NULL;
  UNIT_TEST_SUITE_HANDLE          PageGuard = NULL;
  UNIT_TEST_SUITE_HANDLE          PoolGuard = NULL;
  UNIT_TEST_SUITE_HANDLE          Misc      = NULL;
  MEMORY_PROTECTION_TEST_CONTEXT  *MemoryProtectionContext;

  DEBUG ((DEBUG_ERROR, "%a()\n", __FUNCTION__));

  DEBUG ((DEBUG_ERROR, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  MemoryProtectionContext = (MEMORY_PROTECTION_TEST_CONTEXT *)AllocateZeroPool (sizeof (MEMORY_PROTECTION_TEST_CONTEXT));
  if (MemoryProtectionContext == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Allocating memory for test context failed.\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  LocateSmmCommonCommBuffer ();

  Status = FetchMemoryProtectionHobEntries ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - FetchMemoryProtectionHobEntries() failed. %r\n", __FUNCTION__, Status));
    goto EXIT;
  }

  Status = RegisterMemoryProtectionTestAppInterruptHandler ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - RegisterMemoryProtectionTestAppInterruptHandler() failed. %r\n", __FUNCTION__, Status));
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

  if ((PageGuard == NULL) || (PoolGuard == NULL) || (Misc == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed in CreateUnitTestSuite for TestSuite\n", __func__));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddSmmPageTest (PageGuard);
  AddSmmPoolTest (PoolGuard);

  // Add NULL protection to the Misc test suite.
  AddTestCase (Misc, "Null pointer access in SMM should trigger a page fault", "Security.HeapGuardMisc.SmmNullPointerDetection", SmmNullPointerDetection, SmmNullPointerPreReq, NULL, MemoryProtectionContext);

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
