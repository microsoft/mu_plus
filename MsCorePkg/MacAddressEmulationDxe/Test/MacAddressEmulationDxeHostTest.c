/** @file
  This module tests MacAddressEmulation behavior for
  MacAddressEmulationDxe driver.

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

#include <Library/UnitTestLib.h>
#include <Library/DebugLib.h>

#include <Protocol/SimpleNetwork.h>

#include "../MacAddressEmulationDxe.h"

#define UNIT_TEST_NAME     "Mac Address Emulation Dxe Host Test"
#define UNIT_TEST_VERSION  "0.1"

EFI_RUNTIME_SERVICES  mMockRuntime;

EFI_STATUS
IsMacEmulationEnabled (
  OUT EFI_MAC_ADDRESS *Address
  )
{
  return EFI_UNSUPPORTED;
}

BOOLEAN
SnpSupportsMacEmulation (
  IN  EFI_HANDLE SnpHandle
  )
{
  return FALSE;
}

EFI_STATUS
PlatformMacEmulationEnable (
  IN  EFI_MAC_ADDRESS *Address
  )
{
  return EFI_ABORTED;
}

/**
  Unit test for DummyTest ()

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpHandleNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  BOOLEAN SupportsEmu;
  EFI_SIMPLE_NETWORK_PROTOCOL Snp;
  UINTN MacContext;

  // Act
  SupportsEmu = SnpSupportsMacEmuCheck(NULL, &Snp, &MacContext);

  // Assert
  assert_true(SupportsEmu == FALSE);

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
  UNIT_TEST_SUITE_HANDLE      TestSuite;

  Framework = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));

  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
  }

  if (!EFI_ERROR (Status)) {
    Status = CreateUnitTestSuite (&TestSuite, Framework, "TargetVerifyPhase", "ReportRouter.Phase", NULL, NULL);
  }
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for TestSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
  }

  if (!EFI_ERROR (Status)) {
    AddTestCase (TestSuite, "Dummy Test Description", "Dummy Test", SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpHandleNull, NULL, NULL, NULL);
  
    Status = RunAllTestSuites (Framework);
  }

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
