/** @file
TODO: Populate this.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UnitTestLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/MpService.h>
#include <Protocol/MpManagement.h>

#define UNIT_TEST_APP_NAME        "MP Management Unit Test"
#define UNIT_TEST_APP_SHORT_NAME  "Mp_Mgmt_Test"
#define UNIT_TEST_APP_VERSION     "1.0"

#define PROTOCOL_DOUBLE_CHECK     1

MP_MANAGEMENT_PROTOCOL  *mMpManagement  = NULL;
UINTN                   mBspIndex       = 0;
UINTN                   mApDutIndex     = 0;

/// ================================================================================================
/// ================================================================================================
///
/// HELPER FUNCTIONS
///
/// ================================================================================================
/// ================================================================================================

/// ================================================================================================
/// ================================================================================================
///
/// PRE REQ FUNCTIONS
///
/// ================================================================================================
/// ================================================================================================

/**
  Power on all APs before we test anything on them.

  @param[in] Context                  Test context applied for this test case

  @retval UNIT_TEST_PASSED            The entry point executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED Null pointer detected.

**/
UNIT_TEST_STATUS
EFIAPI
PowerOnAps (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = mMpManagement->ApOn (mMpManagement, OPERATION_FOR_ALL_APS);

  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
} // PowerOnAps ()

/// ================================================================================================
/// ================================================================================================
///
/// CLEANUP FUNCTIONS
///
/// ================================================================================================
/// ================================================================================================

/**
  Power off all APs to clean up the slate.

  @param[in] Context                  Test context applied for this test case

  @retval UNIT_TEST_PASSED            The entry point executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED Null pointer detected.

**/
VOID
EFIAPI
PowerOffAps (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS Status;

  ASSERT (mMpManagement != NULL);

  Status = mMpManagement->ApOff (mMpManagement, OPERATION_FOR_ALL_APS);

  ASSERT_EFI_ERROR (Status);

} // PowerOffAps ()

/// ================================================================================================
/// ================================================================================================
///
/// TEST CASES
///
/// ================================================================================================
/// ================================================================================================

UNIT_TEST_STATUS
EFIAPI
TurnOnAllAps (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApOn (mMpManagement, OPERATION_FOR_ALL_APS);

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we power them all on, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN*)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // TurnOnAllAps()

UNIT_TEST_STATUS
EFIAPI
TurnOffAllAps (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApOff (mMpManagement, OPERATION_FOR_ALL_APS);

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we power them all on, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN*)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // TurnOffAllAps()

UNIT_TEST_STATUS
EFIAPI
TurnOnSingleAp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApOn (mMpManagement, mApDutIndex);

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we power them all on, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN*)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // TurnOnSingleAp()

UNIT_TEST_STATUS
EFIAPI
TurnOffSingleAp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApOff (mMpManagement, mApDutIndex);

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we power them all on, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN*)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // TurnOffSingleAp()

UNIT_TEST_STATUS
EFIAPI
TurnOnBsp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApOn (mMpManagement, mBspIndex);

  if (Status != EFI_INVALID_PARAMETER) {
    // If this is the first time we power them all on, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // TurnOnBsp()

UNIT_TEST_STATUS
EFIAPI
TurnOffBsp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApOff (mMpManagement, mBspIndex);

  if (Status != EFI_INVALID_PARAMETER) {
    // If this is the first time we power them all on, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // TurnOffBsp()

UNIT_TEST_STATUS
EFIAPI
SuspendAllApsToC1 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApSuspend (mMpManagement, OPERATION_FOR_ALL_APS, AP_POWER_C1);

  if (Status != EFI_INVALID_PARAMETER) {
    // If this is the first time we power them all on, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // SuspendAllApsToC1()

UNIT_TEST_STATUS
EFIAPI
ResumeAllApsFromC1 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApResume (mMpManagement, OPERATION_FOR_ALL_APS);

  if (Status != EFI_INVALID_PARAMETER) {
    // If this is the first time we power them all on, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // ResumeAllApsFromC1()

/**
  This function will gather information and configure the
  environment for all tests to operate.

  @retval     EFI_SUCCESS
  @retval     Others

**/
STATIC
EFI_STATUS
InitializeTestEnvironment (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_MP_SERVICES_PROTOCOL  *MpServices;
  UINTN                     NumCpus;
  UINTN                     EnabledCpus;


  Status = gBS->LocateProtocol (
                &gEfiMpServiceProtocolGuid,
                NULL,
                (VOID **)&MpServices
                );
  if (EFI_ERROR (Status)) {
    // If we're here, we definitely had something weird happen...
    DEBUG ((DEBUG_ERROR, "%a Failed to locate MP service protocol!!! - %r\n", __FUNCTION__, Status));
    goto Done;
  }

  Status = MpServices->GetNumberOfProcessors (MpServices, &NumCpus, &EnabledCpus);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a Failed to get the number of processors!!! - %r\n", __FUNCTION__, Status));
    goto Done;
  }

  Status = MpServices->WhoAmI (MpServices, &mBspIndex);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a Failed to get the number of processors!!! - %r\n", __FUNCTION__, Status));
    goto Done;
  }

  mApDutIndex = 0;
  while (mApDutIndex < NumCpus) {
    if (mApDutIndex != mBspIndex) {
      break;
    }
    mApDutIndex ++;
  }

  if ((mApDutIndex >= NumCpus) || (mApDutIndex == mBspIndex)) {
    DEBUG ((DEBUG_ERROR, "%a Failed to find any AP to be tested!!! - %d\n", __FUNCTION__, mApDutIndex));
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  Status = gBS->LocateProtocol (
                  &gMpManagementProtocolGuid,
                  NULL,
                  (VOID **)&mMpManagement
                  );
  if (EFI_ERROR (Status)) {
    // If we're here, we had something weird happen.
    DEBUG ((DEBUG_ERROR, "%a Failed to locate MP management protocol!!! - %r\n", __FUNCTION__, Status));
    goto Done;
  }

Done:
  return Status;
} // InitializeTestEnvironment()

/**
  MpManagementTestApp

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
MpManagementTestApp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Fw = NULL;
  UNIT_TEST_SUITE_HANDLE      BasicOperationTests;
  UNIT_TEST_SUITE_HANDLE      SuspendOperationTests;
  UINTN                       Context = PROTOCOL_DOUBLE_CHECK;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  //
  // First, let's set up somethings that will be used by all test cases.
  //
  Status = InitializeTestEnvironment ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FAILED to initialize test environment!!\n"));
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
  // Populate the BasicOperationTests Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&BasicOperationTests, Fw, "Basic Operation Tests", "MpManagement.Operation", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for BasicOperationTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // Populate the SuspendOperationTests Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&SuspendOperationTests, Fw, "Suspend Operation Tests", "MpManagement.Suspend", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for SuspendOperationTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  // AddTestCase (BasicOperationTests, "Turn on all APs should succeed", "MpManagement.CpuOn.AllInit", TurnOnAllAps, NULL, NULL, NULL);
  // AddTestCase (BasicOperationTests, "Double turn on all APs should fail", "MpManagement.CpuOn.AllDouble", TurnOnAllAps, NULL, NULL, &Context);
  // AddTestCase (BasicOperationTests, "Turn off all APs should succeed", "MpManagement.CpuOff.AllInit", TurnOffAllAps, NULL, NULL, NULL);
  // AddTestCase (BasicOperationTests, "Double turn off all APs should fail", "MpManagement.CpuOff.AllDouble", TurnOffAllAps, NULL, NULL, &Context);
  // AddTestCase (BasicOperationTests, "Turn on a single AP should succeed", "MpManagement.CpuOn.ApInit", TurnOnSingleAp, NULL, NULL, NULL);
  // AddTestCase (BasicOperationTests, "Double turn on a single AP should fail", "MpManagement.CpuOn.ApDouble", TurnOnSingleAp, NULL, NULL, &Context);
  // AddTestCase (BasicOperationTests, "Turn off a single AP should succeed", "MpManagement.CpuOff.ApInit", TurnOffSingleAp, NULL, NULL, NULL);
  // AddTestCase (BasicOperationTests, "Double turn off a single AP should fail", "MpManagement.CpuOff.ApDouble", TurnOffSingleAp, NULL, NULL, &Context);
  // AddTestCase (BasicOperationTests, "Turn on BSP should fail", "MpManagement.CpuOn.Bsp", TurnOnBsp, NULL, NULL, NULL);
  // AddTestCase (BasicOperationTests, "Turn off BSP should fail", "MpManagement.CpuOff.Bsp", TurnOffBsp, NULL, NULL, NULL);

  AddTestCase (SuspendOperationTests, "Suspend to C1 on all APs should succeed", "MpManagement.SuspendC1.AllInit", SuspendAllApsToC1, PowerOnAps, NULL, NULL);
  // AddTestCase (SuspendOperationTests, "Double suspend to C1 on all APs should fail", "MpManagement.SuspendC1.AllDouble", SuspendAllApsToC1, NULL, NULL, &Context);
  // AddTestCase (SuspendOperationTests, "Resume all APs from C1 should succeed", "MpManagement.ResumeC1.AllInit", ResumeAllApsFromC1, NULL, NULL, NULL);
  AddTestCase (SuspendOperationTests, "Turn off all APs should succeed", "MpManagement.ResumeC1.AllDouble", ResumeAllApsFromC1, NULL, PowerOffAps, &Context);
  // AddTestCase (SuspendOperationTests, "Double turn off all APs should fail", "Security.MAT.MemMapZeroSizeEntries", NoLegacyMapEntriesShouldHaveZeroSize, NULL, NULL, NULL);
  // AddTestCase (SuspendOperationTests, "Turn on a single AP should succeed", "Security.MAT.MemMapSize", LegacyMapSizeShouldBeAMultipleOfDescriptorSize, NULL, NULL, NULL);
  // AddTestCase (SuspendOperationTests, "Double turn on a single AP should fail", "Security.MAT.MemMapZeroSizeEntries", NoLegacyMapEntriesShouldHaveZeroSize, NULL, NULL, NULL);
  // AddTestCase (SuspendOperationTests, "Turn off a single AP should succeed", "Security.MAT.Size", MatMapSizeShouldBeAMultipleOfDescriptorSize, NULL, NULL, NULL);
  // AddTestCase (SuspendOperationTests, "Double turn off a single AP should fail", "Security.MAT.Size", MatMapSizeShouldBeAMultipleOfDescriptorSize, NULL, NULL, NULL);
  // AddTestCase (SuspendOperationTests, "Turn on BSP should fail", "Security.MAT.MatZeroSizeEntries", NoMatMapEntriesShouldHaveZeroSize, NULL, NULL, NULL);
  // AddTestCase (SuspendOperationTests, "Turn off BSP should fail", "Security.MAT.MatZeroSizeEntries", NoMatMapEntriesShouldHaveZeroSize, NULL, NULL, NULL);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Fw);

EXIT:
  if (Fw) {
    FreeUnitTestFramework (Fw);
  }

  return Status;
}
