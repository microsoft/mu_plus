/** @file
AdvancedLoggerWrapperTestApp.c

This is a Unit Test for the AdvancedLoggerWrapper code.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <AdvancedLoggerInternal.h>

#include <Protocol/AdvancedLogger.h>
#include <AdvancedLoggerInternalProtocol.h>

#include <Library/AdvancedLoggerAccessLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiLib.h>
#include <Library/UnitTestLib.h>
#include <Library/AdvancedLoggerLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define UNIT_TEST_APP_NAME        "AdvancedLoggerWrapper Library test cases"
#define UNIT_TEST_APP_VERSION     "1.0"
#define ADV_TIME_STAMP_PREFIX     "hh:mm:ss:ttt : "
#define ADV_TIME_STAMP_PREFIX_LEN (sizeof (ADV_TIME_STAMP_PREFIX) - sizeof (CHAR8))

CHAR8  *InternalMemoryLog[] = {
  "DEADBEEF\n"
};

// The following text represents the output lines from the line parser given the above input

/* spell-checker: disable */
CHAR8  Line00[] = "09:06:45.012 : [DXE] DEADBEEF\n";

ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY  mMessageEntry;
ADVANCED_LOGGER_INFO                       *mLoggerInfo;
EFI_PHYSICAL_ADDRESS                       mMaxAddress  = 0;
UINT32                                     mBufferSize  = 0;

typedef struct {
  CHAR8         *IdString;
  CHAR8         *ExpectedLine;
  VOID          *MemoryToFree;
  EFI_STATUS    ExpectedStatus;
} BASIC_TEST_CONTEXT;

// *----------------------------------------------------------------------------------*
// * Test Contexts                                                                    *
// *----------------------------------------------------------------------------------*
STATIC BASIC_TEST_CONTEXT  mTest00 = { "Basic tests", Line00, NULL, EFI_SUCCESS };

/// ================================================================================================
/// ================================================================================================
///
/// HELPER FUNCTIONS
///
/// ================================================================================================
/// ================================================================================================

/**
    ValidateInfoBlock

    The address of the ADVANCE_LOGGER_INFO block pointer is captured before END_OF_DXE.  The
    pointers LogBuffer and LogCurrent, and LogBufferSize, could be written to by untrusted code.  Here, we check that
    the pointers are within the allocated LoggerInfo space, and that LogBufferSize, which is used in multiple places
    to see if a new message will fit into the log buffer, is valid.

    @param          NONE

    @return         BOOLEAN     TRUE - mInforBlock passes security checks
    @return         BOOLEAN     FALSE- mInforBlock failed security checks

**/
STATIC
BOOLEAN
ValidateInfoBlock (
  VOID
  )
{
  if (mLoggerInfo == NULL) {
    return FALSE;
  }

  if (mLoggerInfo->Signature != ADVANCED_LOGGER_SIGNATURE) {
    return FALSE;
  }

  if (mLoggerInfo->LogBuffer != PA_FROM_PTR (mLoggerInfo + 1)) {
    return FALSE;
  }

  if ((mLoggerInfo->LogCurrent > mMaxAddress) ||
      (mLoggerInfo->LogCurrent < mLoggerInfo->LogBuffer))
  {
    return FALSE;
  }

  if (mLoggerInfo->LogBufferSize != mBufferSize) {
    return FALSE;
  }

  return TRUE;
}

/*
    CleanUpTestContext

    Cleans up after a test case.  Free any allocated buffers if a test
    takes the error exit.

*/
STATIC
VOID
EFIAPI
CleanUpTestContext (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BASIC_TEST_CONTEXT  *Btc;

  Btc = (BASIC_TEST_CONTEXT *)Context;

  if (NULL != Btc->MemoryToFree) {
    FreePool (Btc->MemoryToFree);
    Btc->MemoryToFree = NULL;
  }
}

/// ================================================================================================
/// ================================================================================================
///
/// TEST CASES
///
/// ================================================================================================
/// ================================================================================================

/*
  Initialize LoggerInfo for tracking the test progress
*/
STATIC
UNIT_TEST_STATUS
EFIAPI
InitializeInMemoryLog (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  ADVANCED_LOGGER_PROTOCOL  *LoggerProtocol;
  EFI_STATUS                Status;

  if (gBS == NULL) {
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  // Make sure the wrapper feature is enabled.
  if (!FeaturePcdGet (PcdAdvancedLoggerAutoClearEnable)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  ZeroMem (&mMessageEntry, sizeof (mMessageEntry));

  //
  // Locate the Logger Information block.
  //
  Status = gBS->LocateProtocol (
                  &gAdvancedLoggerProtocolGuid,
                  NULL,
                  (VOID **)&LoggerProtocol
                  );
  if (!EFI_ERROR (Status)) {
    mLoggerInfo = LOGGER_INFO_FROM_PROTOCOL (LoggerProtocol);
    if (mLoggerInfo != NULL) {
      mMaxAddress = mLoggerInfo->LogBuffer + mLoggerInfo->LogBufferSize;
      mBufferSize = mLoggerInfo->LogBufferSize;
    }

    if (!ValidateInfoBlock ()) {
      mLoggerInfo = NULL;
    }

    // This is to bypass the restriction on runtime check.
    mLoggerInfo->AtRuntime = TRUE;
  } else {
    return UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
  }

  return UNIT_TEST_PASSED;
}

/**
  Basic Tests

  Validates that the DEBUG print blocks are returned as null terminated lines.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestCursorWrapping (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BASIC_TEST_CONTEXT  *Btc;
  EFI_STATUS          Status;

  Btc = (BASIC_TEST_CONTEXT *)Context;

  // First fill in the buffer
  while (mLoggerInfo->LogCurrent + MESSAGE_ENTRY_SIZE_V2 (sizeof (ADVANCED_LOGGER_MESSAGE_ENTRY_V2), sizeof ("Test")) < mMaxAddress) {
    AdvancedLoggerWrite (DEBUG_ERROR, "Test", sizeof ("Test"));
  }

  // This is the last logging that should push the buffer over the edge
  AdvancedLoggerWrite (DEBUG_ERROR, "DEADBEEF\n", sizeof ("DEADBEED\n"));

  Status = AdvancedLoggerAccessLibGetNextFormattedLine (&mMessageEntry);

  UT_ASSERT_STATUS_EQUAL (mLoggerInfo->LogCurrent - mLoggerInfo->LogBuffer, MESSAGE_ENTRY_SIZE_V2 (sizeof (ADVANCED_LOGGER_MESSAGE_ENTRY_V2), sizeof ("DEADBEED\n")));
  UT_ASSERT_STATUS_EQUAL (Status, Btc->ExpectedStatus);
  UT_ASSERT_NOT_NULL (mMessageEntry.Message);
  UT_LOG_INFO ("\nReturn Length=%d\n", mMessageEntry.MessageLen);
  UT_LOG_INFO ("\n = %a =\n", mMessageEntry.Message);
  UT_LOG_INFO ("\nExpected Length=%d\n", AsciiStrLen (Btc->ExpectedLine));
  UT_LOG_INFO ("\n = %a =\n", Btc->ExpectedLine);

  if (mMessageEntry.MessageLen != AsciiStrLen (Btc->ExpectedLine)) {
    DUMP_HEX (DEBUG_ERROR, 0, mMessageEntry.Message, mMessageEntry.MessageLen, "Actual   - ");
    DUMP_HEX (DEBUG_ERROR, 0, Btc->ExpectedLine, AsciiStrLen (Btc->ExpectedLine), "Expected - ");
  }

  UT_ASSERT_EQUAL (mMessageEntry.MessageLen, AsciiStrLen (Btc->ExpectedLine));

  // The following verifies that the string content matches expectation and is NULL terminated, timestamp is not compared.
  UT_ASSERT_MEM_EQUAL (&mMessageEntry.Message[ADV_TIME_STAMP_PREFIX_LEN],
  &Btc->ExpectedLine[ADV_TIME_STAMP_PREFIX_LEN],
  (mMessageEntry.MessageLen + sizeof (CHAR8) - ADV_TIME_STAMP_PREFIX_LEN));

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
  DeviceIdTestAppEntry

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
AdvancedLoggerWrapperTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UNIT_TEST_FRAMEWORK_HANDLE  Fw;
  UNIT_TEST_SUITE_HANDLE      AdvLoggerWrapperTests;
  EFI_STATUS                  Status;

  Fw = (UNIT_TEST_FRAMEWORK_HANDLE)NULL;
  AsciiPrint ("%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION);
  DEBUG ((DEBUG_ERROR, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  ZeroMem (&mMessageEntry, sizeof (mMessageEntry));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
    AsciiPrint ("Failed in InitUnitTestFramework. Status = %r\n", Status);
    goto EXIT;
  }

  //
  // Populate the DeviceId Library Test Suite.
  //
  Status = CreateUnitTestSuite (&AdvLoggerWrapperTests, Fw, "Validate Line parser returns valid data", "AdvancedLoggerWrapper.Test", NULL, NULL);
  if (EFI_ERROR (Status)) {
    AsciiPrint ("Failed in CreateUnitTestSuite for Line Parser Tests\n");
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  // Start with normal message entry tests
  // -----------Suite------------Description-------Class---------Test Function-Pre---Clean-Context
  AddTestCase (AdvLoggerWrapperTests, "Init", "SelfInit", InitializeInMemoryLog, NULL, NULL, NULL);
  AddTestCase (AdvLoggerWrapperTests, "Basic check", "BasicCheck", TestCursorWrapping, NULL, CleanUpTestContext, &mTest00);
  AddTestCase (AdvLoggerWrapperTests, "Basic check", "BasicCheck", TestCursorWrapping, NULL, CleanUpTestContext, &mTest00);

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
