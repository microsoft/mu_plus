/** @file
  UEFI shell unit test application for MP management driver.

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
#include <Library/TimerLib.h>
#include <Protocol/MpService.h>
#include <Protocol/MpManagement.h>

#define UNIT_TEST_APP_NAME        "MP Management Unit Test"
#define UNIT_TEST_APP_SHORT_NAME  "Mp_Mgmt_Test"
#define UNIT_TEST_APP_VERSION     "1.0"

#define PROTOCOL_DOUBLE_CHECK  1
#define BSP_SUSPEND_TIMER_US   1000000
#define US_TO_NS(a)  (a * 1000)

MP_MANAGEMENT_PROTOCOL  *mMpManagement = NULL;
UINTN                   mBspIndex      = 0;
UINTN                   mApDutIndex    = 0;

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
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = mMpManagement->ApOn (mMpManagement, OPERATION_FOR_ALL_APS);

  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
} // PowerOnAps ()

/**
  Power on a single AP before we test anything on it.

  @param[in] Context                  Test context applied for this test case

  @retval UNIT_TEST_PASSED            The entry point executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED Null pointer detected.

**/
UNIT_TEST_STATUS
EFIAPI
PowerOnSingleAp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = mMpManagement->ApOn (mMpManagement, mApDutIndex);

  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
} // PowerOnSingleAp ()

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
  EFI_STATUS  Status;

  ASSERT (mMpManagement != NULL);

  Status = mMpManagement->ApOff (mMpManagement, OPERATION_FOR_ALL_APS);

  ASSERT_EFI_ERROR (Status);
} // PowerOffAps ()

/**
  Power off a single AP to clean up the slate.

  @param[in] Context                  Test context applied for this test case

  @retval UNIT_TEST_PASSED            The entry point executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED Null pointer detected.

**/
VOID
EFIAPI
PowerOffSingleAp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  ASSERT (mMpManagement != NULL);

  Status = mMpManagement->ApOff (mMpManagement, mApDutIndex);

  ASSERT_EFI_ERROR (Status);
} // PowerOffSingleAp ()

/// ================================================================================================
/// ================================================================================================
///
/// TEST CASES
///
/// ================================================================================================
/// ================================================================================================

/**
  Unit test for turning on a all APs.

  @param[in] Context                    An optional parameter that supports:
                                        1) NULL input will expect the APs to turn on properly
                                        2) Point to PROTOCOL_DOUBLE_CHECK will expect the APs
                                           already being turned on and return with expected
                                           error code.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
TurnOnAllAps (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApOn (mMpManagement, OPERATION_FOR_ALL_APS);

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we power them all on, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN *)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // TurnOnAllAps()

/**
  Unit test for turning off a all APs.

  @param[in] Context                    An optional parameter that supports:
                                        1) NULL input will expect the APs to turn off properly
                                        2) Point to PROTOCOL_DOUBLE_CHECK will expect the APs
                                           already being turned off and return with expected
                                           error code.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
TurnOffAllAps (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApOff (mMpManagement, OPERATION_FOR_ALL_APS);

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we power them all off, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN *)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // TurnOffAllAps()

/**
  Unit test for turning on a single AP.

  @param[in] Context                    An optional parameter that supports:
                                        1) NULL input will expect the single AP to turn on properly
                                        2) Point to PROTOCOL_DOUBLE_CHECK will expect the single AP
                                           already being turned on and return with expected error
                                           code.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
TurnOnSingleAp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApOn (mMpManagement, mApDutIndex);

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we power a single one on, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN *)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // TurnOnSingleAp()

/**
  Unit test for turning off a single AP.

  @param[in] Context                    An optional parameter that supports:
                                        1) NULL input will expect the single AP to turn off properly
                                        2) Point to PROTOCOL_DOUBLE_CHECK will expect the single AP
                                           already being turned off and return with expected error
                                           code.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
TurnOffSingleAp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApOff (mMpManagement, mApDutIndex);

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we power a single one off, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN *)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // TurnOffSingleAp()

/**
  Unit test for turning on the BSP with AP interfaces.

  @param[in] Context                    An optional parameter unused here

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
TurnOnBsp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApOn (mMpManagement, mBspIndex);

  if (Status != EFI_INVALID_PARAMETER) {
    // BSP is not supported under this interface
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // TurnOnBsp()

/**
  Unit test for turning off the BSP with AP interfaces.

  @param[in] Context                    An optional parameter unused here

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
TurnOffBsp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApOff (mMpManagement, mBspIndex);

  if (Status != EFI_INVALID_PARAMETER) {
    // BSP is not supported under this interface
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // TurnOffBsp()

/**
  Unit test for suspending all APs to C1 state.

  @param[in] Context                    An optional parameter that supports:
                                        1) NULL input will expect all APs to suspend properly
                                        2) Point to PROTOCOL_DOUBLE_CHECK will expect all APs
                                           already being suspended to C1 and return with expected
                                           error code.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
SuspendAllApsToC1 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApSuspend (mMpManagement, OPERATION_FOR_ALL_APS, AP_POWER_C1, 0);

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we suspend them to C1, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN *)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // SuspendAllApsToC1()

/**
  Unit test for suspending a single AP to C1 state.

  @param[in] Context                    An optional parameter that supports:
                                        1) NULL input will expect the single AP to suspend properly
                                        2) Point to PROTOCOL_DOUBLE_CHECK will expect the single AP
                                           already being suspended to C1 and return with expected
                                           error code.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
SuspendSingleApToC1 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApSuspend (mMpManagement, mApDutIndex, AP_POWER_C1, 0);

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we suspend it to C1, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN *)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // SuspendSingleApToC1()

/**
  Unit test for suspending all APs to C2 state.

  @param[in] Context                    An optional parameter that supports:
                                        1) NULL input will expect all APs to suspend properly
                                        2) Point to PROTOCOL_DOUBLE_CHECK will expect all APs
                                           already being suspended to C2 and return with expected
                                           error code.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
SuspendAllApsToC2 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApSuspend (mMpManagement, OPERATION_FOR_ALL_APS, AP_POWER_C2, (UINTN)PcdGet64 (PcdPlatformC2PowerState));

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we suspend them to C2, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN *)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // SuspendAllApsToC2()

/**
  Unit test for suspending a single AP to C2 state.

  @param[in] Context                    An optional parameter that supports:
                                        1) NULL input will expect the single AP to suspend properly
                                        2) Point to PROTOCOL_DOUBLE_CHECK will expect the single AP
                                           already being suspended to C2 and return with expected
                                           error code.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
SuspendSingleApToC2 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApSuspend (mMpManagement, mApDutIndex, AP_POWER_C2, (UINTN)PcdGet64 (PcdPlatformC2PowerState));

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we suspend it to C2, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN *)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // SuspendSingleApToC2()

/**
  Unit test for suspending all APs to C3 state.

  @param[in] Context                    An optional parameter that supports:
                                        1) NULL input will expect all APs to suspend properly
                                        2) Point to PROTOCOL_DOUBLE_CHECK will expect all APs
                                           already being suspended to C3 and return with expected
                                           error code.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
SuspendAllApsToC3 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApSuspend (mMpManagement, OPERATION_FOR_ALL_APS, AP_POWER_C3, (UINTN)PcdGet64 (PcdPlatformC3PowerState));

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we suspend them to C3, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN *)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // SuspendAllApsToC3()

/**
  Unit test for suspending a single AP to C3 state.

  @param[in] Context                    An optional parameter that supports:
                                        1) NULL input will expect the single AP to suspend properly
                                        2) Point to PROTOCOL_DOUBLE_CHECK will expect the single AP
                                           already being suspended to C3 and return with expected
                                           error code.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
SuspendSingleApToC3 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApSuspend (mMpManagement, mApDutIndex, AP_POWER_C3, (UINTN)PcdGet64 (PcdPlatformC3PowerState));

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we suspend it to C3, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN *)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // SuspendSingleApToC3()

/**
  Unit test for resuming all APs to on state.

  @param[in] Context                    An optional parameter that supports:
                                        1) NULL input will expect all APs to suspend properly
                                        2) Point to PROTOCOL_DOUBLE_CHECK will expect all APs
                                           already in on state and return with expected error
                                           code.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
ResumeAllAps (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApResume (mMpManagement, OPERATION_FOR_ALL_APS);

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we resume all the APs, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN *)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // ResumeAllAps()

/**
  Unit test for resuming a single AP to on state.

  @param[in] Context                    An optional parameter that supports:
                                        1) NULL input will expect a single AP to suspend properly
                                        2) Point to PROTOCOL_DOUBLE_CHECK will expect a single AP
                                           already in on state and return with expected error
                                           code.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
ResumeSingleAp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  Status = mMpManagement->ApResume (mMpManagement, mApDutIndex);

  if ((Context == NULL) && EFI_ERROR (Status)) {
    // If this is the first time we resume this AP, it should succeed.
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else if ((Context != NULL) && ((*(UINTN *)Context) == PROTOCOL_DOUBLE_CHECK) && (Status != EFI_ALREADY_STARTED)) {
    // Otherwise, the protocol should take care of the state check.
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
} // ResumeSingleAp()

/**
  Unit test for suspending the BSP to C1 state.

  @param[in] Context                    An optional parameter that supports:
                                        1) NULL input will expect the BSP to suspend properly
                                        2) Point to PROTOCOL_DOUBLE_CHECK will expect the BSP
                                           already suspended to on state and return with expected
                                           error code.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
SuspendBspToC1 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT64      StartTick;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  StartTick = GetPerformanceCounter ();

  Status = mMpManagement->BspSuspend (mMpManagement, AP_POWER_C1, 0, BSP_SUSPEND_TIMER_US);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_TRUE (GetTimeInNanoSecond (GetPerformanceCounter () - StartTick) > US_TO_NS (BSP_SUSPEND_TIMER_US));

  return UNIT_TEST_PASSED;
} // SuspendBspToC1()

/**
  Unit test for suspending the BSP to C2 state.

  @param[in] Context                    An optional parameter that supports:
                                        1) NULL input will expect the BSP to suspend properly
                                        2) Point to PROTOCOL_DOUBLE_CHECK will expect the BSP
                                           already suspended to on state and return with expected
                                           error code.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
SuspendBspToC2 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT64      StartTick;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  StartTick = GetPerformanceCounter ();

  Status = mMpManagement->BspSuspend (mMpManagement, AP_POWER_C2, (UINTN)PcdGet64 (PcdPlatformC2PowerState), BSP_SUSPEND_TIMER_US);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_TRUE (GetTimeInNanoSecond (GetPerformanceCounter () - StartTick) > US_TO_NS (BSP_SUSPEND_TIMER_US));

  return UNIT_TEST_PASSED;
} // SuspendBspToC2()

/**
  Unit test for suspending the BSP to C3 state.

  @param[in] Context                    An optional parameter that supports:
                                        1) NULL input will expect the BSP to suspend properly
                                        2) Point to PROTOCOL_DOUBLE_CHECK will expect the BSP
                                           already suspended to on state and return with expected
                                           error code.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.

**/
UNIT_TEST_STATUS
EFIAPI
SuspendBspToC3 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT64      StartTick;

  if (mMpManagement == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG ((DEBUG_INFO, "%a Entry.. \n", __FUNCTION__));

  StartTick = GetPerformanceCounter ();

  Status = mMpManagement->BspSuspend (mMpManagement, AP_POWER_C3, (UINTN)PcdGet64 (PcdPlatformC3PowerState), BSP_SUSPEND_TIMER_US);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_TRUE (GetTimeInNanoSecond (GetPerformanceCounter () - StartTick) > US_TO_NS (BSP_SUSPEND_TIMER_US));

  return UNIT_TEST_PASSED;
} // SuspendBspToC3()

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
    DEBUG ((DEBUG_ERROR, "%a Failed to get the index of BSP!!! - %r\n", __FUNCTION__, Status));
    goto Done;
  }

  mApDutIndex = 0;
  while (mApDutIndex < NumCpus) {
    if (mApDutIndex != mBspIndex) {
      break;
    }

    mApDutIndex++;
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
  MpManagementTestApp entrypoint.

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

  AddTestCase (BasicOperationTests, "Turn on all APs should succeed", "MpManagement.CpuOn.AllInit", TurnOnAllAps, NULL, NULL, NULL);
  AddTestCase (BasicOperationTests, "Double turn on all APs should fail", "MpManagement.CpuOn.AllDouble", TurnOnAllAps, NULL, NULL, &Context);
  AddTestCase (BasicOperationTests, "Turn off all APs should succeed", "MpManagement.CpuOff.AllInit", TurnOffAllAps, NULL, NULL, NULL);
  AddTestCase (BasicOperationTests, "Double turn off all APs should fail", "MpManagement.CpuOff.AllDouble", TurnOffAllAps, NULL, NULL, &Context);
  AddTestCase (BasicOperationTests, "Turn on a single AP should succeed", "MpManagement.CpuOn.ApInit", TurnOnSingleAp, NULL, NULL, NULL);
  AddTestCase (BasicOperationTests, "Double turn on a single AP should fail", "MpManagement.CpuOn.ApDouble", TurnOnSingleAp, NULL, NULL, &Context);
  AddTestCase (BasicOperationTests, "Turn off a single AP should succeed", "MpManagement.CpuOff.ApInit", TurnOffSingleAp, NULL, NULL, NULL);
  AddTestCase (BasicOperationTests, "Double turn off a single AP should fail", "MpManagement.CpuOff.ApDouble", TurnOffSingleAp, NULL, NULL, &Context);
  AddTestCase (BasicOperationTests, "Turn on BSP should fail", "MpManagement.CpuOn.Bsp", TurnOnBsp, NULL, NULL, NULL);
  AddTestCase (BasicOperationTests, "Turn off BSP should fail", "MpManagement.CpuOff.Bsp", TurnOffBsp, NULL, NULL, NULL);

  AddTestCase (SuspendOperationTests, "Suspend to C1 on all APs should succeed", "MpManagement.SuspendC1.AllInit", SuspendAllApsToC1, PowerOnAps, NULL, NULL);
  AddTestCase (SuspendOperationTests, "Double suspend to C1 on all APs should fail", "MpManagement.SuspendC1.AllDouble", SuspendAllApsToC1, NULL, NULL, &Context);
  AddTestCase (SuspendOperationTests, "Resume all APs from C1 should succeed", "MpManagement.ResumeC1.AllInit", ResumeAllAps, NULL, NULL, NULL);
  AddTestCase (SuspendOperationTests, "Double resume all APs from C1 should fail", "MpManagement.ResumeC1.AllDouble", ResumeAllAps, NULL, PowerOffAps, &Context);

  AddTestCase (SuspendOperationTests, "Suspend to C1 on a single AP should succeed", "MpManagement.SuspendC1.SingleInit", SuspendSingleApToC1, PowerOnSingleAp, NULL, NULL);
  AddTestCase (SuspendOperationTests, "Double suspend to C1 on a single AP should fail", "MpManagement.SuspendC1.SingleDouble", SuspendSingleApToC1, NULL, NULL, &Context);
  AddTestCase (SuspendOperationTests, "Resume a single AP from C1 should succeed", "MpManagement.ResumeC1.SingleInit", ResumeSingleAp, NULL, NULL, NULL);
  AddTestCase (SuspendOperationTests, "Double resume a single AP from C1 should fail", "MpManagement.ResumeC1.SingleDouble", ResumeSingleAp, NULL, PowerOffSingleAp, &Context);

  AddTestCase (SuspendOperationTests, "Suspend to C2 on all APs should succeed", "MpManagement.SuspendC2.AllInit", SuspendAllApsToC2, PowerOnAps, NULL, NULL);
  AddTestCase (SuspendOperationTests, "Double suspend to C2 on all APs should fail", "MpManagement.SuspendC2.AllDouble", SuspendAllApsToC2, NULL, NULL, &Context);
  AddTestCase (SuspendOperationTests, "Resume all APs from C2 should succeed", "MpManagement.ResumeC2.AllInit", ResumeAllAps, NULL, NULL, NULL);
  AddTestCase (SuspendOperationTests, "Double resume all APs from C2 should fail", "MpManagement.ResumeC2.AllDouble", ResumeAllAps, NULL, PowerOffAps, &Context);

  AddTestCase (SuspendOperationTests, "Suspend to C2 on single AP should succeed", "MpManagement.SuspendC2.SingleInit", SuspendSingleApToC2, PowerOnSingleAp, NULL, NULL);
  AddTestCase (SuspendOperationTests, "Double suspend to C2 on single AP should fail", "MpManagement.SuspendC2.SingleDouble", SuspendSingleApToC2, NULL, NULL, &Context);
  AddTestCase (SuspendOperationTests, "Resume single AP from C2 should succeed", "MpManagement.ResumeC2.SingleInit", ResumeSingleAp, NULL, NULL, NULL);
  AddTestCase (SuspendOperationTests, "Double resume single AP from C2 should fail", "MpManagement.ResumeC2.SingleDouble", ResumeSingleAp, NULL, PowerOffSingleAp, &Context);

  AddTestCase (SuspendOperationTests, "Suspend to C3 on all APs should succeed", "MpManagement.SuspendC3.AllInit", SuspendAllApsToC3, PowerOnAps, NULL, NULL);
  AddTestCase (SuspendOperationTests, "Double suspend to C3 on all APs should fail", "MpManagement.SuspendC3.AllDouble", SuspendAllApsToC3, NULL, NULL, &Context);
  AddTestCase (SuspendOperationTests, "Resume all APs from C3 should succeed", "MpManagement.ResumeC3.AllInit", ResumeAllAps, NULL, NULL, NULL);
  AddTestCase (SuspendOperationTests, "Double resume all APs from C3 should fail", "MpManagement.ResumeC3.AllDouble", ResumeAllAps, NULL, PowerOffAps, &Context);

  AddTestCase (SuspendOperationTests, "Suspend to C3 on single AP should succeed", "MpManagement.SuspendC3.SingleInit", SuspendSingleApToC3, PowerOnSingleAp, NULL, NULL);
  AddTestCase (SuspendOperationTests, "Double suspend to C3 on single AP should fail", "MpManagement.SuspendC3.SingleDouble", SuspendSingleApToC3, NULL, NULL, &Context);
  AddTestCase (SuspendOperationTests, "Resume single AP from C3 should succeed", "MpManagement.ResumeC3.SingleInit", ResumeSingleAp, NULL, NULL, NULL);
  AddTestCase (SuspendOperationTests, "Double resume single AP from C3 should fail", "MpManagement.ResumeC3.SingleDouble", ResumeSingleAp, NULL, PowerOffSingleAp, &Context);

  AddTestCase (SuspendOperationTests, "Suspend to C1 on BSP should succeed after a timeout", "MpManagement.SuspendC1.BSP", SuspendBspToC1, NULL, NULL, NULL);
  AddTestCase (SuspendOperationTests, "Suspend to C2 on BSP should succeed after a timeout", "MpManagement.SuspendC2.BSP", SuspendBspToC2, NULL, NULL, NULL);
  AddTestCase (SuspendOperationTests, "Suspend to C3 on BSP should succeed after a timeout", "MpManagement.SuspendC3.BSP", SuspendBspToC3, NULL, NULL, NULL);

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
