/** @file
  This module tests MFCI policy verification and apply
  logic for root of trust based MfciDxe driver.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>

#include <MfciPolicyType.h>
#include <MfciVariables.h>
#include <Guid/MuVarPolicyFoundationDxe.h>
#include <Protocol/MfciProtocol.h>
#include <Library/MfciRetrievePolicyLib.h>

#include <Library/BaseLib.h>                             // CpuDeadLoop()
#include <Library/DebugLib.h>                            // DEBUG tracing
#include <Library/BaseMemoryLib.h>                       // CopyGuid()
#include <Library/VariablePolicyHelperLib.h>             // NotifyMfciPolicyChange()
#include <Library/MemoryAllocationLib.h>

#include <Library/UnitTestLib.h>

#define UNIT_TEST_NAME     "RoT based Mfci Verify Policy And Change Host Test"
#define UNIT_TEST_VERSION  "0.1"

/**
  A mocked version of SetVariable.

  @returns mocked value supplied by test frameworks.

**/
EFI_STATUS
EFIAPI
UnitTestSetVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
  );

EFI_RUNTIME_SERVICES  mMockRuntime = {
  .SetVariable = UnitTestSetVariable,
};

extern MFCI_POLICY_TYPE  mCurrentPolicy;
extern BOOLEAN           mVarPolicyRegistered;

EFI_STATUS
EFIAPI
UnitTestSetVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
  )
{
  DEBUG ((DEBUG_INFO, "%a: %s\n", __FUNCTION__, VariableName));

  check_expected (VariableName);
  check_expected (Data);
  check_expected (DataSize);
  return mock ();
}

EFI_STATUS
EFIAPI
NotifyMfciPolicyChange (
  IN MFCI_POLICY_TYPE  NewPolicy
  )
{
  check_expected (NewPolicy);
  return (EFI_STATUS)mock ();
}

EFI_STATUS
EFIAPI
InitPublicInterface (
  VOID
  )
{
  return EFI_SUCCESS;
}

UNIT_TEST_STATUS
EFIAPI
VerifyPrerequisite (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  mCurrentPolicy       = CUSTOMER_STATE;
  mVarPolicyRegistered = TRUE;
  return UNIT_TEST_PASSED;
}

VOID
EFIAPI
VerifyCleanup (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  mCurrentPolicy       = CUSTOMER_STATE;
  mVarPolicyRegistered = FALSE;
}

VOID
EFIAPI
VerifyPolicyAndChange (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

// Verified on a normal path from one policy to the next
UNIT_TEST_STATUS
EFIAPI
UnitTestVerifyAndChangeNormal (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BASE_LIBRARY_JUMP_BUFFER  JumpBuf;
  MFCI_POLICY_TYPE          Policy = STD_ACTION_TPM_CLEAR;

  will_return (MfciRetrieveTargetPolicy, STD_ACTION_TPM_CLEAR);
  will_return (MfciRetrieveTargetPolicy, EFI_SUCCESS);

  expect_value (NotifyMfciPolicyChange, NewPolicy, STD_ACTION_TPM_CLEAR);
  will_return (NotifyMfciPolicyChange, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, sizeof (Policy));
  expect_memory (UnitTestSetVariable, Data, &Policy, sizeof (Policy));
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_value (ResetSystemWithSubtype, ResetType, EfiResetCold);
  expect_value (ResetSystemWithSubtype, ResetSubtype, &gMfciPolicyChangeResetGuid);
  will_return (ResetSystemWithSubtype, &JumpBuf);

  if (!SetJump (&JumpBuf)) {
    VerifyPolicyAndChange (NULL, NULL);
  }

  return UNIT_TEST_PASSED;
}

// Verified on a normal path without policy change
UNIT_TEST_STATUS
EFIAPI
UnitTestVerifyAndChangeNoChange (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  POLICY_LOCK_VAR  LockVar = MFCI_LOCK_VAR_VALUE;

  will_return (MfciRetrieveTargetPolicy, CUSTOMER_STATE);
  will_return (MfciRetrieveTargetPolicy, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, MFCI_LOCK_VAR_NAME, sizeof (MFCI_LOCK_VAR_NAME));
  expect_value (UnitTestSetVariable, DataSize, sizeof (POLICY_LOCK_VAR));
  expect_memory (UnitTestSetVariable, Data, &LockVar, sizeof (POLICY_LOCK_VAR));
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  VerifyPolicyAndChange (NULL, NULL);

  return UNIT_TEST_PASSED;
}

// Verify the failure of retrieving policy on customer state will boot on
UNIT_TEST_STATUS
EFIAPI
UnitTestVerifyAndChangeTargetPolicyFailedCustomer (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  POLICY_LOCK_VAR  LockVar = MFCI_LOCK_VAR_VALUE;

  will_return (MfciRetrieveTargetPolicy, CUSTOMER_STATE);
  will_return (MfciRetrieveTargetPolicy, EFI_DEVICE_ERROR);

  expect_memory (UnitTestSetVariable, VariableName, MFCI_LOCK_VAR_NAME, sizeof (MFCI_LOCK_VAR_NAME));
  expect_value (UnitTestSetVariable, DataSize, sizeof (POLICY_LOCK_VAR));
  expect_memory (UnitTestSetVariable, Data, &LockVar, sizeof (POLICY_LOCK_VAR));
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  VerifyPolicyAndChange (NULL, NULL);

  return UNIT_TEST_PASSED;
}

// Verify the failure of retrieving policy on non-customer state will clean up and reboot
UNIT_TEST_STATUS
EFIAPI
UnitTestVerifyAndChangeTargetPolicyFailedNonCustomer (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BASE_LIBRARY_JUMP_BUFFER  JumpBuf;
  MFCI_POLICY_TYPE          Policy = CUSTOMER_STATE;

  mCurrentPolicy = STD_ACTION_TPM_CLEAR;

  will_return (MfciRetrieveTargetPolicy, STD_ACTION_TPM_CLEAR);
  will_return (MfciRetrieveTargetPolicy, EFI_DEVICE_ERROR);

  expect_value (NotifyMfciPolicyChange, NewPolicy, CUSTOMER_STATE);
  will_return (NotifyMfciPolicyChange, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, sizeof (Policy));
  expect_memory (UnitTestSetVariable, Data, &Policy, sizeof (Policy));
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_value (ResetSystemWithSubtype, ResetType, EfiResetCold);
  expect_value (ResetSystemWithSubtype, ResetSubtype, &gMfciPolicyChangeResetGuid);
  will_return (ResetSystemWithSubtype, &JumpBuf);

  if (!SetJump (&JumpBuf)) {
    VerifyPolicyAndChange (NULL, NULL);
  }

  return UNIT_TEST_PASSED;
}

/**
  Initialize the unit test framework, suite, and unit tests for the
  sample unit tests and run the unit tests.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
EFI_STATUS
EFIAPI
UefiTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      TargetVerifyPhaseSuite;
  UNIT_TEST_SUITE_HANDLE      VerifyAndChangePhaseSuite;

  Framework = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  // The blob parsing part is tested in MfciPolicyParsingUnitTest, so will not go through those here.

  //
  // Populate the TargetVerifyPhaseSuite Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&TargetVerifyPhaseSuite, Framework, "TargetVerifyPhase", "ReportRouter.Phase", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for TargetVerifyPhaseSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // Populate the VerifyAndChangePhaseSuite Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&VerifyAndChangePhaseSuite, Framework, "VerifyAndChangePhase", "ReportRouter.Phase", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for VerifyAndChangePhaseSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (VerifyAndChangePhaseSuite, "VerifyAndChange should succeed with correct target information", "VerifyPerfect", UnitTestVerifyAndChangeNormal, VerifyPrerequisite, VerifyCleanup, NULL);
  AddTestCase (VerifyAndChangePhaseSuite, "VerifyAndChange should boot on without policy change", "VerifyNoChange", UnitTestVerifyAndChangeNoChange, VerifyPrerequisite, VerifyCleanup, NULL);
  AddTestCase (VerifyAndChangePhaseSuite, "VerifyAndChange should boot on with failed policy query on customer state", "VerifyFailedCustomer", UnitTestVerifyAndChangeTargetPolicyFailedCustomer, VerifyPrerequisite, VerifyCleanup, NULL);
  AddTestCase (VerifyAndChangePhaseSuite, "VerifyAndChange should boot on with failed policy query on non-customer state", "VerifyFailedNonCustomer", UnitTestVerifyAndChangeTargetPolicyFailedNonCustomer, VerifyPrerequisite, VerifyCleanup, NULL);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  return UefiTestMain ();
}
