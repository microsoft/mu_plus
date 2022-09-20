/** @file ExceptionPersistenceTestApp.c

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/ExceptionPersistenceLib.h>

#define UNIT_TEST_APP_NAME        "ExceptionPersistenceTestApp"
#define UNIT_TEST_APP_SHORT_NAME  "ExPersistTest"
#define UNIT_TEST_APP_VERSION     "1.0"

/// ================================================================================================
/// ================================================================================================
///
/// TEST ENGINE
///
/// ================================================================================================
/// ================================================================================================

UNIT_TEST_STATUS
EFIAPI
ReadWriteReadTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN           index = 0;
  EXCEPTION_TYPE  ex    = 0;

  UT_ASSERT_NOT_EFI_ERROR (ExPersistClearAll ());

  for ( ; index < ExceptionPersistMax; index++) {
    UT_ASSERT_NOT_EFI_ERROR (ExPersistSetException ((EXCEPTION_TYPE)index));
    UT_ASSERT_NOT_EFI_ERROR (ExPersistGetException (&ex));

    UT_ASSERT_EQUAL (ex, (EXCEPTION_TYPE)index);

    UT_ASSERT_NOT_EFI_ERROR (ExPersistClearExceptions ());
    UT_ASSERT_NOT_EFI_ERROR (ExPersistGetException (&ex));

    UT_ASSERT_EQUAL (ex, ExceptionPersistNone);
  }

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
IgnoreNextPageFaultTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  IgnoreNextPageFault = FALSE;

  UT_ASSERT_NOT_EFI_ERROR (ExPersistClearAll ());

  UT_ASSERT_NOT_EFI_ERROR (ExPersistSetIgnoreNextPageFault ());
  UT_ASSERT_NOT_EFI_ERROR (ExPersistGetIgnoreNextPageFault (&IgnoreNextPageFault));
  UT_ASSERT_EQUAL (IgnoreNextPageFault, TRUE);
  UT_ASSERT_NOT_EFI_ERROR (ExPersistClearIgnoreNextPageFault ());
  UT_ASSERT_NOT_EFI_ERROR (ExPersistGetIgnoreNextPageFault (&IgnoreNextPageFault));
  UT_ASSERT_EQUAL (IgnoreNextPageFault, FALSE);

  return UNIT_TEST_PASSED;
}

/**
  Test the functionality of ExceptionPersistenceLib

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
ExceptionPersistenceTestApp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Fw = NULL;
  UNIT_TEST_SUITE_HANDLE      TestSuite;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the TestSuite Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&TestSuite, Fw, "Exception Persistence Library Tests", "Security.ExPersist", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for TestSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (TestSuite, "Test Reading and Writing", "Security.ReadWriteRead", ReadWriteReadTest, NULL, NULL, NULL);
  AddTestCase (TestSuite, "Test Ignore Next Page Fault", "Security.IgnoreNextPageFaultTest", IgnoreNextPageFaultTest, NULL, NULL, NULL);

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
