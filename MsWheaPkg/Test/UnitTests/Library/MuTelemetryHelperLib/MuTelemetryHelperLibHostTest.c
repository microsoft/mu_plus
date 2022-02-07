/** @file -- MuTelemetryHelperLibHostTest.c
Host-based UnitTest for MuTelemetryHelperLib.

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
#include <Uefi/UefiBaseType.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>

#include <Guid/ZeroGuid.h>
#include <MsWheaHostTestCommon.h>

#include <MsWheaErrorStatus.h>
#include <Guid/MsWheaReportDataType.h>
#include <Library/MuTelemetryHelperLib.h>

#define UNIT_TEST_NAME     " MuTelemetryHelperLib Unit Test"
#define UNIT_TEST_VERSION  "0.1"

// Mocked version of ReportStatusCodeEx()
EFI_STATUS
EFIAPI
ReportStatusCodeEx (
  IN EFI_STATUS_CODE_TYPE   Type,
  IN EFI_STATUS_CODE_VALUE  Value,
  IN UINT32                 Instance,
  IN CONST EFI_GUID         *CallerId          OPTIONAL,
  IN CONST EFI_GUID         *ExtendedDataGuid  OPTIONAL,
  IN CONST VOID             *ExtendedData      OPTIONAL,
  IN UINTN                  ExtendedDataSize
  )
{
  MS_WHEA_RSC_INTERNAL_ERROR_DATA  *Header;
  UINT8                            *ExtraData;

  check_expected (Type);
  check_expected (Value);
  // Instance is Don't Care
  check_expected (CallerId);
  check_expected (ExtendedDataGuid);
  check_expected (ExtendedDataSize);

  Header = (MS_WHEA_RSC_INTERNAL_ERROR_DATA *)ExtendedData;
  check_expected_ptr (&Header->LibraryID);
  check_expected_ptr (&Header->IhvSharingGuid);
  check_expected (Header->AdditionalInfo1);
  check_expected (Header->AdditionalInfo2);

  if (ExtendedDataSize > sizeof (MS_WHEA_RSC_INTERNAL_ERROR_DATA)) {
    ExtraData = (UINT8 *)Header + sizeof (MS_WHEA_RSC_INTERNAL_ERROR_DATA);
    check_expected_ptr (ExtraData);
    check_expected_ptr (&ExtraData[sizeof (EFI_GUID)]);
  }

  return (EFI_STATUS)mock ();
}

UNIT_TEST_STATUS
EFIAPI
BasicLogTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  //
  // Pass only a StatusCode
  //
  expect_value (ReportStatusCodeEx, Type, MS_WHEA_ERROR_STATUS_TYPE_INFO);
  expect_value (ReportStatusCodeEx, Value, TEST_RSC_MISC_C);
  expect_value (ReportStatusCodeEx, CallerId, NULL);
  expect_memory (ReportStatusCodeEx, ExtendedDataGuid, &gMsWheaRSCDataTypeGuid, sizeof (EFI_GUID));
  expect_value (ReportStatusCodeEx, ExtendedDataSize, sizeof (MS_WHEA_RSC_INTERNAL_ERROR_DATA));

  expect_memory (ReportStatusCodeEx, &Header->LibraryID, &gZeroGuid, sizeof (EFI_GUID));
  expect_memory (ReportStatusCodeEx, &Header->IhvSharingGuid, &gZeroGuid, sizeof (EFI_GUID));
  expect_value (ReportStatusCodeEx, Header->AdditionalInfo1, 0x00);
  expect_value (ReportStatusCodeEx, Header->AdditionalInfo2, 0x00);

  will_return (ReportStatusCodeEx, EFI_SUCCESS);
  UT_ASSERT_NOT_EFI_ERROR (
    LogTelemetry (
      FALSE,
      NULL,
      TEST_RSC_MISC_C,
      NULL,
      NULL,
      0x00,
      0x00
      )
    );

  //
  // Pass Everything
  //
  expect_value (ReportStatusCodeEx, Type, MS_WHEA_ERROR_STATUS_TYPE_FATAL);
  expect_value (ReportStatusCodeEx, Value, TEST_RSC_MISC_C);
  expect_memory (ReportStatusCodeEx, CallerId, &gEfiCallerIdGuid, sizeof (EFI_GUID));
  expect_memory (ReportStatusCodeEx, ExtendedDataGuid, &gMsWheaRSCDataTypeGuid, sizeof (EFI_GUID));
  expect_value (ReportStatusCodeEx, ExtendedDataSize, sizeof (MS_WHEA_RSC_INTERNAL_ERROR_DATA));

  expect_memory (ReportStatusCodeEx, &Header->LibraryID, &mTestGuid2, sizeof (EFI_GUID));
  expect_memory (ReportStatusCodeEx, &Header->IhvSharingGuid, &mTestGuid3, sizeof (EFI_GUID));
  expect_value (ReportStatusCodeEx, Header->AdditionalInfo1, 0xDEADBEEFDEADBEEF);
  expect_value (ReportStatusCodeEx, Header->AdditionalInfo2, 0xFEEDF00DFEEDF00D);

  will_return (ReportStatusCodeEx, EFI_SUCCESS);
  UT_ASSERT_NOT_EFI_ERROR (
    LogTelemetry (
      TRUE,
      &gEfiCallerIdGuid,
      TEST_RSC_MISC_C,
      &mTestGuid2,
      &mTestGuid3,
      0xDEADBEEFDEADBEEF,
      0xFEEDF00DFEEDF00D
      )
    );

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
ExtraLogTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  CHAR8  TestData[] = "This is my test data.";

  //
  // Only pass NULLs
  //
  expect_value (ReportStatusCodeEx, Type, MS_WHEA_ERROR_STATUS_TYPE_INFO);
  expect_value (ReportStatusCodeEx, Value, TEST_RSC_MISC_C);
  expect_value (ReportStatusCodeEx, CallerId, NULL);
  expect_memory (ReportStatusCodeEx, ExtendedDataGuid, &gMsWheaRSCDataTypeGuid, sizeof (EFI_GUID));
  expect_value (ReportStatusCodeEx, ExtendedDataSize, sizeof (MS_WHEA_RSC_INTERNAL_ERROR_DATA));

  expect_memory (ReportStatusCodeEx, &Header->LibraryID, &gZeroGuid, sizeof (EFI_GUID));
  expect_memory (ReportStatusCodeEx, &Header->IhvSharingGuid, &gZeroGuid, sizeof (EFI_GUID));
  expect_value (ReportStatusCodeEx, Header->AdditionalInfo1, 0x00);
  expect_value (ReportStatusCodeEx, Header->AdditionalInfo2, 0x00);

  will_return (ReportStatusCodeEx, EFI_SUCCESS);
  UT_ASSERT_NOT_EFI_ERROR (
    LogTelemetryEx (
      FALSE,
      NULL,
      TEST_RSC_MISC_C,
      NULL,
      NULL,
      0x00,
      0x00,
      NULL,
      0x00,
      NULL
      )
    );

  //
  // Pass Everything
  //
  expect_value (ReportStatusCodeEx, Type, MS_WHEA_ERROR_STATUS_TYPE_FATAL);
  expect_value (ReportStatusCodeEx, Value, TEST_RSC_MISC_C);
  expect_memory (ReportStatusCodeEx, CallerId, &gEfiCallerIdGuid, sizeof (EFI_GUID));
  expect_memory (ReportStatusCodeEx, ExtendedDataGuid, &gMsWheaRSCDataTypeGuid, sizeof (EFI_GUID));
  expect_value (ReportStatusCodeEx, ExtendedDataSize, sizeof (MS_WHEA_RSC_INTERNAL_ERROR_DATA) + sizeof (EFI_GUID) + sizeof (TestData));

  expect_memory (ReportStatusCodeEx, &Header->LibraryID, &mTestGuid2, sizeof (EFI_GUID));
  expect_memory (ReportStatusCodeEx, &Header->IhvSharingGuid, &mTestGuid3, sizeof (EFI_GUID));
  expect_value (ReportStatusCodeEx, Header->AdditionalInfo1, 0xDEADBEEFDEADBEEF);
  expect_value (ReportStatusCodeEx, Header->AdditionalInfo2, 0xFEEDF00DFEEDF00D);

  expect_memory (ReportStatusCodeEx, ExtraData, &mTestGuid1, sizeof (EFI_GUID));
  expect_memory (ReportStatusCodeEx, &ExtraData[sizeof (EFI_GUID)], TestData, sizeof (TestData));

  will_return (ReportStatusCodeEx, EFI_SUCCESS);
  UT_ASSERT_NOT_EFI_ERROR (
    LogTelemetryEx (
      TRUE,
      &gEfiCallerIdGuid,
      TEST_RSC_MISC_C,
      &mTestGuid2,
      &mTestGuid3,
      0xDEADBEEFDEADBEEF,
      0xFEEDF00DFEEDF00D,
      &mTestGuid1,
      sizeof (TestData),
      TestData
      )
    );

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
ExtraLogParamTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  CHAR8  TestData[] = "This is my test data.";

  //
  // Don't pass a buffer.
  //
  UT_ASSERT_TRUE (
    EFI_ERROR (
      LogTelemetryEx (
        FALSE,
        NULL,
        TEST_RSC_MISC_C,
        NULL,
        NULL,
        0x00,
        0x00,
        &mTestGuid1,
        0x00,
        NULL
        )
      )
    );

  //
  // Don't pass a GUID
  //
  UT_ASSERT_TRUE (
    EFI_ERROR (
      LogTelemetryEx (
        FALSE,
        NULL,
        TEST_RSC_MISC_C,
        NULL,
        NULL,
        0x00,
        0x00,
        NULL,
        sizeof (TestData),
        TestData
        )
      )
    );

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
  UNIT_TEST_SUITE_HANDLE      LogSuite;

  // UNIT_TEST_SUITE_HANDLE      LogExSuite;

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

  //
  // Populate the LogSuite Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&LogSuite, Framework, "LogTelemetry", "Log.General", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for LogSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (LogSuite, "LogTelemetry should pass correctly formatted data to RSC", "BasicTest", BasicLogTest, NULL, NULL, NULL);
  AddTestCase (LogSuite, "LogTelemetryEx should pass correctly formatted data to RSC", "ExtraTest", ExtraLogTest, NULL, NULL, NULL);
  AddTestCase (LogSuite, "LogTelemetryEx should fail if extra params are partially provided", "ExtraParamFail", ExtraLogParamTest, NULL, NULL, NULL);

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
