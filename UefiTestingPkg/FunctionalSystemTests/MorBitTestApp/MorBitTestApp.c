/** @file 
MorBitTestApp.c
This application will test the MOR (Memory Overwrite Request) feature.

Copyright (c) 2016, Microsoft Corporation

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

#include <PiDxe.h>
#include <Uefi.h>
#include <UnitTestTypes.h>
#include <Private/Library/TestCapsuleHelperLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/UnitTestLogLib.h>
#include <Library/UnitTestAssertLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Guid/MemoryOverwriteControl.h>
#include <IndustryStandard/MemoryOverwriteRequestControlLock.h>

#define UNIT_TEST_APP_NAME        L"MOR Bit Test"
#define UNIT_TEST_APP_SHORT_NAME  L"MOR_Bit_Test"
#define UNIT_TEST_APP_VERSION     L"0.1"

#define TEST_CAPSULE_SIZE (0x1000)

///============================================================================
///============================================================================
///
/// HELPER FUNCTIONS
///
///============================================================================
///============================================================================


//
// Anything you think might be helpful that isn't a test itself.
//

EFI_STATUS
GetMorControlVariable (
  OUT UINT8* MorControl
  )
{

  UINT8 Data;
  UINTN DataSize;
  EFI_STATUS Status;

  DataSize = sizeof(Data);
  Status = gRT->GetVariable(MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                           &gEfiMemoryOverwriteControlDataGuid,
                           NULL,
                           &DataSize,
                           &Data);

  if (!EFI_ERROR(Status)) {
    if (DataSize != sizeof(*MorControl)) {
      Status = EFI_BAD_BUFFER_SIZE;

    } else {
      *MorControl = Data;
    }
  }

  return Status;
}

EFI_STATUS
SetMorControlVariable (
  IN UINT8* MorControl
  )
{

  EFI_STATUS Status;

  Status = gRT->SetVariable(MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                            &gEfiMemoryOverwriteControlDataGuid,
                            (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS),
                            sizeof(*MorControl),
                            MorControl);

  return Status;
}

///============================================================================
///============================================================================
///
/// TEST CASES
///
///============================================================================
///============================================================================

UNIT_TEST_STATUS
EFIAPI
TestSetMorBitNone (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

  EFI_CAPSULE_HEADER* CapsuleHeaderArray[2];
  UINT64 MaxCapsuleSize;
  UINT8 MorControl;
  UINT32 Phase;
  EFI_RESET_TYPE ResetType;
  EFI_CAPSULE_BLOCK_DESCRIPTOR* SgList;
  EFI_STATUS Status;
  UINTN Layout[] = { TEST_CAPSULE_SIZE, 0x0 };

  if (Context != NULL) {
    Phase = *(UINT32*)Context;

  } else {
    Phase = 0;
  }

  DEBUG((DEBUG_INFO, "Test set MOR Bit none... Phase=%d\n", Phase));
  UT_LOG_INFO("Test set MOR Bit none... Phase=%d\n", Phase);

  if (Phase == 0) {

    //
    // Advance to the next phase.
    //
    Phase = 1;

    SgList = NULL;
    //
    // Build the capsule that we will supply to the UpdateCapsule routine.
    //
    Status = BuildTestCapsule((CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE | CAPSULE_FLAGS_PERSIST_ACROSS_RESET), &SgList, ARRAY_SIZE(Layout), Layout);

    UT_ASSERT_NOT_EFI_ERROR(Status);
    UT_ASSERT_NOT_NULL(SgList);


    //
    // Initialize the capsule header array. We're passing in one capsule.
    // the array must be NULL-terminated.
    //
    DEBUG((DEBUG_INFO, "   stuff the capsule array...\n"));
    CapsuleHeaderArray[0] = (EFI_CAPSULE_HEADER*)(UINTN)(SgList[0].Union.DataBlock);
    CapsuleHeaderArray[1] = NULL;

    //
    // Inquire about the platform capability of UpdateCapsule.
    //
    DEBUG((DEBUG_INFO, "   get capsule capabilities...\n"));
    Status = gRT->QueryCapsuleCapabilities(CapsuleHeaderArray,
                                           1,
                                           &MaxCapsuleSize,
                                           &ResetType);

    UT_ASSERT_NOT_EFI_ERROR(Status);

    //
    // Check that the capsule we've created is not too large.
    //
    DEBUG((DEBUG_INFO, "   verify capsule against capabilities...\n"));
    UT_LOG_INFO("   verify capsule against capabilities...\n");
    UT_ASSERT_FALSE(GetLayoutTotalSize(ARRAY_SIZE(Layout), Layout) > MaxCapsuleSize);

    //
    // Call update capsule and reset the system.
    //
    DEBUG((DEBUG_INFO, "   call update capsule...\n"));
    UT_LOG_INFO("   call update capsule...\n");
    Status = gRT->UpdateCapsule(CapsuleHeaderArray, 1, (UINTN)SgList);
    UT_ASSERT_NOT_EFI_ERROR(Status);

    //
    // Get the current MOR control setting.
    //
    UT_LOG_INFO("   get MOR control variable...\n");
    MorControl = 0;
    GetMorControlVariable(&MorControl);
    UT_LOG_INFO(__FUNCTION__ ": MorControl:0x%02x\n", MorControl);

    //
    // Set the MOR Bit.
    //

    //
    // Reboot the system.
    //
    Status = SaveFrameworkStateAndReboot(Framework,
                                         &Phase,
                                         sizeof(Phase),
                                         ResetType);

    UT_LOG_ERROR("   should not have gotten here (%r)\n", Status);
    UT_ASSERT_TRUE(FALSE);
    return UNIT_TEST_ERROR_TEST_FAILED;

  } else if (Phase == 1) {

    UT_ASSERT_EQUAL(GetTestCapsuleCountFromSystemTable(), 1);
    return UNIT_TEST_PASSED;

  } else {
    UT_LOG_ERROR("   unexpected Phase (%d)\n", Phase);
    UT_ASSERT_TRUE(FALSE);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }
}

UNIT_TEST_STATUS
EFIAPI
TestSetMorBitZero (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

  EFI_CAPSULE_HEADER* CapsuleHeaderArray[2];
  UINT64 MaxCapsuleSize;
  UINT8 MorControl;
  UINT32 Phase;
  EFI_RESET_TYPE ResetType;
  EFI_CAPSULE_BLOCK_DESCRIPTOR* SgList;
  EFI_STATUS Status;
  UINTN Layout[] = { 0x1000, 0x0 };

  if (Context != NULL) {
    Phase = *(UINT32*)Context;
  } else {
    Phase = 0;
  }

  DEBUG((DEBUG_INFO, "Test set MOR Bit zero... Phase=%d\n", Phase));
  UT_LOG_INFO("Test set MOR Bit zero... Phase=%d\n", Phase);

  if (Phase == 0) {

    //
    // Advance to the next phase.
    //
    Phase = 1;
    SgList = NULL;
    //
    // Build the capsule that we will supply to the UpdateCapsule routine.
    //
    Status = BuildTestCapsule((CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE | CAPSULE_FLAGS_PERSIST_ACROSS_RESET), &SgList, ARRAY_SIZE(Layout), Layout);

    UT_ASSERT_NOT_EFI_ERROR(Status);
    UT_ASSERT_NOT_NULL(SgList);

    //
    // Initialize the capsule header array. We're passing in one capsule.
    // the array must be NULL-terminated.
    //
    DEBUG((DEBUG_INFO, "   stuff the capsule array...\n"));
    CapsuleHeaderArray[0] = (EFI_CAPSULE_HEADER*)(UINTN)(SgList[0].Union.DataBlock);
    CapsuleHeaderArray[1] = NULL;

    //
    // Inquire about the platform capability of UpdateCapsule.
    //
    DEBUG((DEBUG_INFO, "   get capsule capabilities...\n"));
    UT_LOG_INFO("   get capsule capabilities...\n");
    Status = gRT->QueryCapsuleCapabilities(CapsuleHeaderArray,
                                           1,
                                           &MaxCapsuleSize,
                                           &ResetType);

    UT_ASSERT_NOT_EFI_ERROR(Status);

    //
    // Check that the capsule we've created is not too large.
    //
    DEBUG((DEBUG_INFO, "   verify capsule against capabilities...\n"));
    UT_LOG_INFO("   verify capsule against capabilities...\n");
    UT_ASSERT_FALSE(GetLayoutTotalSize(ARRAY_SIZE(Layout), Layout) > MaxCapsuleSize);

    //
    // Call update capsule and reset the system.
    //
    DEBUG((DEBUG_INFO, "   call update capsule...\n"));
    UT_LOG_INFO("   call update capsule...\n");
    Status = gRT->UpdateCapsule(CapsuleHeaderArray, 1, (UINTN)SgList);
    UT_ASSERT_NOT_EFI_ERROR(Status);

    //
    // Set the MOR Bit zero.
    //
    UT_LOG_INFO("   set MOR control bit zero...\n");
    MorControl = 1;
    Status = SetMorControlVariable(&MorControl);
    UT_ASSERT_NOT_EFI_ERROR(Status);

    //
    // Get the current MOR control setting.
    //
    MorControl = 0;
    Status = GetMorControlVariable(&MorControl);
    UT_ASSERT_NOT_EFI_ERROR(Status);
    UT_LOG_INFO("   MorControl:0x%02x\n", MorControl);
    UT_ASSERT_EQUAL(MorControl, 1);

    //
    // Reboot the system.
    //
    UT_LOG_INFO("resetting system\n");
    Status = SaveFrameworkStateAndReboot(Framework,
                                         &Phase,
                                         sizeof(Phase),
                                         ResetType);

    UT_ASSERT_TRUE(FALSE);

    DEBUG((DEBUG_INFO, "   failed to save state and reboot (%r)", Status));
    UT_LOG_ERROR("   should not have gotten here (%r)\n", Status);
    UT_ASSERT_TRUE(FALSE);
    return UNIT_TEST_ERROR_TEST_FAILED;

  } else if (Phase == 1) {

    //
    // Try to find the capsule in the EFI system table.
    //
    UT_LOG_INFO("   verify that capsule was not processed because memory was cleared...\n")
      UT_ASSERT_EQUAL(GetTestCapsuleCountFromSystemTable(), 1);
    return UNIT_TEST_PASSED;

  } else {
    UT_LOG_ERROR("   unexpected Phase (%d)\n", Phase);
    UT_ASSERT_TRUE(FALSE);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }
}

///============================================================================
///============================================================================
///
/// TEST ENGINE
///
///============================================================================
///============================================================================

/**
  MorBitTestApp
  
  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
MorBitTestApp (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE* SystemTable
  )
{

  UNIT_TEST_FRAMEWORK* Fw;
  UNIT_TEST_SUITE* MorBitTests;
  EFI_STATUS Status;

  Fw = NULL;

  DEBUG((DEBUG_INFO, "%s v%s\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework(&Fw, 
                                 UNIT_TEST_APP_NAME, 
                                 UNIT_TEST_APP_SHORT_NAME, 
                                 UNIT_TEST_APP_VERSION);

  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework (%r)\n", Status));
    goto EXIT;
  }

  //
  // Populate the Unit Test Persistence Test Suite.
  //
  Status = CreateUnitTestSuite(&MorBitTests, 
                               Fw, 
                               L"MOR Bit Unit Test", 
                               L"MORBit.Permutations",
                               NULL, 
                               NULL);

  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "CreateUnitTestSuite failed for MorBitTests\n"));
    goto EXIT;
  }

  AddTestCase(MorBitTests,
              L"Set MOR Bit None",
              L"MORBit.Permutations.NoBits",
              TestSetMorBitNone,
              NULL,
              NULL,
              NULL);

  AddTestCase(MorBitTests,
              L"Set MOR Bit Zero",
              L"MORBit.Permutations.BitZero",
              TestSetMorBitZero,
              NULL,
              NULL,
              NULL);
#if 0
  AddTestCase(MorBitTests, 
              L"Set MOR Bit 4", 
              TestSetMorBitFour, 
              NULL,
              NULL,
              NULL);
#endif

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites(Fw);

EXIT:
  if (Fw != FALSE) {
    FreeUnitTestFramework(Fw);
  }

  return Status;
}
