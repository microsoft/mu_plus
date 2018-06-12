/**@file
CapsuleTestApp.c
This application will test Capsule processing feature.

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

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <UnitTestTypes.h>
#include <Private/Library/TestCapsuleHelperLib.h>
#include <Library/UnitTestLib.h>
#include <Library/UnitTestLogLib.h>
#include <Library/UnitTestAssertLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>

#define UNIT_TEST_APP_NAME        L"Capsule Test"
#define UNIT_TEST_APP_SHORT_NAME  L"Capsule_Test"
#define UNIT_TEST_APP_VERSION     L"0.1"


///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================

UNIT_TEST_STATUS
EFIAPI
TestSGListGoodRtn(
  IN UNIT_TEST_FRAMEWORK_HANDLE Framework,
  IN UNIT_TEST_CONTEXT Context
)
{

  EFI_CAPSULE_HEADER* CapsuleHeaderArray[2];
  UINT64 MaxCapsuleSize;
  UINT32 Phase;
  EFI_RESET_TYPE ResetType;
  EFI_CAPSULE_BLOCK_DESCRIPTOR* SgList;
  EFI_STATUS Status;
  UINTN Layout[] = { 0x1000, 0x0, 0x400, 0x2000, 0x0, 0xC00 };


  //
  // The test phase is supplied in the context. It's NULL for the first phase.
  //
  if (Context != NULL) {
    Phase = *(UINT32*)Context;
  }
  else {
    Phase = 0;
  }

  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test SG List that is good... Phase=%d\n", Phase));
  UT_LOG_INFO("Test SG List that is good... Phase=%d\n", Phase);

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
    CapsuleHeaderArray[0] = (EFI_CAPSULE_HEADER*)(UINTN)(SgList[0].Union.DataBlock);
    CapsuleHeaderArray[1] = NULL;

    //
    // Inquire about the platform capability of UpdateCapsule.
    //
    Status = gRT->QueryCapsuleCapabilities(CapsuleHeaderArray,
      1,
      &MaxCapsuleSize,
      &ResetType);
    UT_ASSERT_NOT_EFI_ERROR(Status);

    //
    // Check that the capsule we've created is not too large.
    //
    UT_ASSERT_FALSE(GetLayoutTotalSize(ARRAY_SIZE(Layout), Layout) > MaxCapsuleSize);

    //
    // Call update capsule.
    //
    Status = gRT->UpdateCapsule(CapsuleHeaderArray, 1, (UINTN)SgList);
    UT_ASSERT_NOT_EFI_ERROR(Status);

    Status = SaveFrameworkStateAndReboot(Framework,
      &Phase,
      sizeof(Phase),
      ResetType);

    //
    // We should not get here. The system should have reset.
    //
    UT_LOG_ERROR("   should not have gotten here (%r)!\n", Status);
    UT_ASSERT_TRUE(FALSE);
    return UNIT_TEST_ERROR_TEST_FAILED;

  }
  else if (Phase == 1) {


    UT_ASSERT_EQUAL(GetTestCapsuleCountFromSystemTable(), 1);
    return UNIT_TEST_PASSED;

  }
  else {
    UT_LOG_ERROR("   unexpected Phase (%d)\n", Phase);
    UT_ASSERT_TRUE(FALSE);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }
}

UNIT_TEST_STATUS
EFIAPI
TestSGListWithLargeContinuationPointerRtn(
  IN UNIT_TEST_FRAMEWORK_HANDLE Framework,
  IN UNIT_TEST_CONTEXT Context
)
{

  EFI_CAPSULE_HEADER* CapsuleHeaderArray[2];
  UINT64 MaxCapsuleSize;
  UINT32 Phase;
  EFI_RESET_TYPE ResetType;
  EFI_CAPSULE_BLOCK_DESCRIPTOR* SgList;
  EFI_CAPSULE_BLOCK_DESCRIPTOR* Temp;
  EFI_STATUS Status;
  UINTN Layout[] = { 0x1000, 0x0, 0x400, 0x2000, 0x0, 0xC00 };

  Phase = 0;

  //
  // The test phase is supplied in the context. It's NULL for the first phase.
  //
  if (Context != NULL) {
    Phase = *(UINT32*)Context;
  }

  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test SG List that has large continuation pointer... Phase=%d\n", Phase));
  UT_LOG_INFO("Test SG List that has large continuation pointer... Phase=%d\n", Phase);

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
    // Give it a bad SGL.  We know the test capsule is 2 -> 3 -> 2
    // Change the ContinuationPointer on the 5th block descriptor
    Temp = SgList + 1;
    Temp = (EFI_CAPSULE_BLOCK_DESCRIPTOR*)((UINTN)Temp->Union.ContinuationPointer);
    Temp = Temp + 2;
    Temp->Union.ContinuationPointer |= 0x1000000000000000;


    //
    // Initialize the capsule header array. We're passing in one capsule.
    // the array must be NULL-terminated.
    //
    CapsuleHeaderArray[0] = (EFI_CAPSULE_HEADER*)(UINTN)(SgList[0].Union.DataBlock);
    CapsuleHeaderArray[1] = NULL;

    //
    // Inquire platform capability of UpdateCapsule.
    //
    Status = gRT->QueryCapsuleCapabilities(CapsuleHeaderArray,
      1,
      &MaxCapsuleSize,
      &ResetType);

    UT_ASSERT_NOT_EFI_ERROR(Status);

    //
    // Check that the capsule we've created is not too large.
    //
    UT_ASSERT_FALSE(GetLayoutTotalSize(ARRAY_SIZE(Layout), Layout) > MaxCapsuleSize);

    //
    // Call update capsule.
    //
    Status = gRT->UpdateCapsule(CapsuleHeaderArray, 1, (UINTN)SgList);
    UT_ASSERT_NOT_EFI_ERROR(Status);

    Status = SaveFrameworkStateAndReboot(Framework,
      &Phase,
      sizeof(Phase),
      ResetType);

    //
    // We should not get here. The system should have reset.
    //
    UT_LOG_ERROR("   should not have gotten here (%r)!\n", Status);
    UT_ASSERT_TRUE(FALSE);
    return UNIT_TEST_ERROR_TEST_FAILED;

  }
  else if (Phase == 1) 
  {

    //
    // Try to find the capsule in the EFI system table.
    //
    UT_ASSERT_EQUAL(GetTestCapsuleCountFromSystemTable(), 0);
    return UNIT_TEST_PASSED;

  }
  else {
    UT_LOG_ERROR("   unexpected Phase (%d)\n", Phase);
    UT_ASSERT_TRUE(FALSE);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }
}

///================================================================================================
///================================================================================================
///
/// TEST ENGINE
///
///================================================================================================
///================================================================================================

/**
  CapsuleTestApp

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
CapsuleTestApp(
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE* SystemTable
)
{

  UNIT_TEST_FRAMEWORK* Fw;
  UNIT_TEST_SUITE* PersistenceTests;
  EFI_STATUS Status;
  Fw = NULL;

  DEBUG((DEBUG_INFO, "%s v%s\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework(&Fw, UNIT_TEST_APP_NAME, UNIT_TEST_APP_SHORT_NAME, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the Unit Test Persistence Test Suite.
  //
  Status = CreateUnitTestSuite(&PersistenceTests, Fw, L"Capsule Processing Unit Test", L"Capsule.Persistence", NULL, NULL);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for PersistenceTests\n"));
    goto EXIT;
  }

  AddTestCase(PersistenceTests,
    L"Call UpdateCapsule with valid Scatter Gather List",
    L"Capsule.Persistence.GoodSGList",
    TestSGListGoodRtn,
    NULL,
    NULL,
    NULL);

  AddTestCase(PersistenceTests,
    L"Call UpdateCapsule with SG List that contains very large continuation pointer",
    L"Capsule.Persistence.SGListWithLargeContinuationPtr",
    TestSGListWithLargeContinuationPointerRtn,
    NULL,
    NULL,
    NULL);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites(Fw);

EXIT:
  if (Fw != NULL) {
    FreeUnitTestFramework(Fw);
  }

  return Status;
}

