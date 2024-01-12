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

#define UNIT_TEST_APP_NAME     "AdvancedLoggerWrapper Library test cases"
#define UNIT_TEST_APP_VERSION  "1.0"

// The following text represents the file logger stream of text as individual DEBUG
// statements.

#define ADV_LOG_MAX_SIZE  76       // ------ 75 characters + NULL ------------------------------+
//                                                                                             +
//                            1         2         3         4         5         6         7    v
//                   123456789012345678901234567890123456789012345678901234567890123456789012345

CHAR8  *InternalMemoryLog[] = {
  "First normal test line\n",
  "The QueryMode() function returns information for an available",
  " graphics mode that the graphics device and the set of active video ",
  "output devices supports.\nIf ModeNumber is not between 0 and MaxMode - 1,",
  " then EFI_INVALID_PARAMETER is returned.\nMaxMode is available from the ",
  "Mode structure of the EFI_GRAPHICS_OUTPUT_PROTOCOL.\n",
  "The size of the Info structure should never be assumed and the ",
  "value of SizeOfInfo is the only valid way to know the size of Info.\n\n",
  "If the EFI_GRAPHICS_OUTPUT_PROTOCOL is installed on the handle that represents a single ",
  "video output device, then the set of modes ",
  "returned by this service is the subset of modes supported ",
  "by both the graphics controller and the video output device.\n",
  "\nIf the EFI_GRAPHICS_OUTPUT_PROTOCOL is installed on the handle ",
  "that represents a combination of video output devices, then the set ",
  "of modes returned by this service is the subset of modes ",
  "supported by the graphics controller and the all of the video output ",
  "devices represented by the handle.\n"
};

// The following text represents the output lines from the line parser given the above input

/* spell-checker: disable */
CHAR8  Line00[] = "09:06:45.012 : First normal test line\n";
CHAR8  Line01[] = "09:06:45.012 : The QueryMode() function returns information for an available graphics mod\n";
CHAR8  Line02[] = "09:06:45.012 : e that the graphics device and the set of active video output devices supp\n";
CHAR8  Line03[] = "09:06:45.012 : orts.\n";
CHAR8  Line04[] = "09:06:45.012 : If ModeNumber is not between 0 and MaxMode - 1, then EFI_INVALID_PARAMETER\n";
CHAR8  Line05[] = "09:06:45.012 :  is returned.\n";
CHAR8  Line06[] = "09:06:45.012 : MaxMode is available from the Mode structure of the EFI_GRAPHICS_OUTPUT_PR\n";
CHAR8  Line07[] = "09:06:45.012 : OTOCOL.\n";
CHAR8  Line08[] = "09:06:45.012 : The size of the Info structure should never be assumed and the value of Si\n";
CHAR8  Line09[] = "09:06:45.012 : zeOfInfo is the only valid way to know the size of Info.\n";
CHAR8  Line10[] = "09:06:45.012 : \n";
CHAR8  Line11[] = "09:06:45.012 : If the EFI_GRAPHICS_OUTPUT_PROTOCOL is installed on the handle that repres\n";
CHAR8  Line12[] = "09:06:45.012 : ents a single video output device, then the set of modes returned by this \n";
CHAR8  Line13[] = "09:06:45.012 : service is the subset of modes supported by both the graphics controller a\n";
CHAR8  Line14[] = "09:06:45.012 : nd the video output device.\n";
CHAR8  Line15[] = "09:06:45.012 : \n";
CHAR8  Line16[] = "09:06:45.012 : If the EFI_GRAPHICS_OUTPUT_PROTOCOL is installed on the handle that repres\n";
CHAR8  Line17[] = "09:06:45.012 : ents a combination of video output devices, then the set of modes returned\n";
CHAR8  Line18[] = "09:06:45.012 :  by this service is the subset of modes supported by the graphics controll\n";
CHAR8  Line19[] = "09:06:45.012 : er and the all of the video output devices represented by the handle.\n";

CHAR8  Line00V2[] = "09:06:45.012 : [DXE] First normal test line\n";
CHAR8  Line01V2[] = "09:06:45.012 : [DXE] The QueryMode() function returns information for an available graphics mod\n";
CHAR8  Line02V2[] = "09:06:45.012 : [DXE] e that the graphics device and the set of active video output devices supp\n";
CHAR8  Line03V2[] = "09:06:45.012 : [DXE] orts.\n";
CHAR8  Line04V2[] = "09:06:45.012 : [DXE] If ModeNumber is not between 0 and MaxMode - 1, then EFI_INVALID_PARAMETER\n";
CHAR8  Line05V2[] = "09:06:45.012 : [DXE]  is returned.\n";
CHAR8  Line06V2[] = "09:06:45.012 : [DXE] MaxMode is available from the Mode structure of the EFI_GRAPHICS_OUTPUT_PR\n";
CHAR8  Line07V2[] = "09:06:45.012 : [DXE] OTOCOL.\n";
CHAR8  Line08V2[] = "09:06:45.012 : [DXE] The size of the Info structure should never be assumed and the value of Si\n";
CHAR8  Line09V2[] = "09:06:45.012 : [DXE] zeOfInfo is the only valid way to know the size of Info.\n";
CHAR8  Line10V2[] = "09:06:45.012 : [DXE] \n";
CHAR8  Line11V2[] = "09:06:45.012 : [DXE] If the EFI_GRAPHICS_OUTPUT_PROTOCOL is installed on the handle that repres\n";
CHAR8  Line12V2[] = "09:06:45.012 : [DXE] ents a single video output device, then the set of modes returned by this \n";
CHAR8  Line13V2[] = "09:06:45.012 : [DXE] service is the subset of modes supported by both the graphics controller a\n";
CHAR8  Line14V2[] = "09:06:45.012 : [DXE] nd the video output device.\n";
CHAR8  Line15V2[] = "09:06:45.012 : [DXE] \n";
CHAR8  Line16V2[] = "09:06:45.012 : [DXE] If the EFI_GRAPHICS_OUTPUT_PROTOCOL is installed on the handle that repres\n";
CHAR8  Line17V2[] = "09:06:45.012 : [DXE] ents a combination of video output devices, then the set of modes returned\n";
CHAR8  Line18V2[] = "09:06:45.012 : [DXE]  by this service is the subset of modes supported by the graphics controll\n";
CHAR8  Line19V2[] = "09:06:45.012 : [DXE] er and the all of the video output devices represented by the handle.\n";

ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY  mMessageEntry;
ADVANCED_LOGGER_INFO                       *mLoggerInfo;

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
STATIC BASIC_TEST_CONTEXT  mTest01 = { "Basic tests", Line01, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest02 = { "Basic tests", Line02, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest03 = { "Basic tests", Line03, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest04 = { "Basic tests", Line04, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest05 = { "Basic tests", Line05, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest06 = { "Basic tests", Line06, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest07 = { "Basic tests", Line07, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest08 = { "Basic tests", Line08, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest09 = { "Basic tests", Line09, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest10 = { "Basic tests", Line10, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest11 = { "Basic tests", Line11, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest12 = { "Basic tests", Line12, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest13 = { "Basic tests", Line13, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest14 = { "Basic tests", Line14, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest15 = { "Basic tests", Line15, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest16 = { "Basic tests", Line16, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest17 = { "Basic tests", Line17, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest18 = { "Basic tests", Line18, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest19 = { "Basic tests", Line19, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest20 = { "End Of File", NULL, NULL, EFI_END_OF_FILE };

STATIC BASIC_TEST_CONTEXT  mTest00V2 = { "Basic tests", Line00V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest01V2 = { "Basic tests", Line01V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest02V2 = { "Basic tests", Line02V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest03V2 = { "Basic tests", Line03V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest04V2 = { "Basic tests", Line04V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest05V2 = { "Basic tests", Line05V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest06V2 = { "Basic tests", Line06V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest07V2 = { "Basic tests", Line07V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest08V2 = { "Basic tests", Line08V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest09V2 = { "Basic tests", Line09V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest10V2 = { "Basic tests", Line10V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest11V2 = { "Basic tests", Line11V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest12V2 = { "Basic tests", Line12V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest13V2 = { "Basic tests", Line13V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest14V2 = { "Basic tests", Line14V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest15V2 = { "Basic tests", Line15V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest16V2 = { "Basic tests", Line16V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest17V2 = { "Basic tests", Line17V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest18V2 = { "Basic tests", Line18V2, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT  mTest19V2 = { "Basic tests", Line19V2, NULL, EFI_SUCCESS };

/// ================================================================================================
/// ================================================================================================
///
/// HELPER FUNCTIONS
///
/// ================================================================================================
/// ================================================================================================

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
  UINTN                     TempSize;
  EFI_STATUS                Status;

  if (gBS == NULL) {
    return;
  }

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
    }

    if (!ValidateInfoBlock ()) {
      mLoggerInfo = NULL;
    }
  }

  return UNIT_TEST_PASSED;
}

/*
    Basic Tests

    Validates that the DEBUG print blocks are returned as null terminated lines.
*/
STATIC
UNIT_TEST_STATUS
EFIAPI
BasicTests (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BASIC_TEST_CONTEXT  *Btc;
  EFI_STATUS          Status;

  Btc = (BASIC_TEST_CONTEXT *)Context;

  Status = AdvancedLoggerAccessLibGetNextFormattedLine (&mMessageEntry);

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

  // The following also verifies that the string is NULL terminated.
  UT_ASSERT_MEM_EQUAL (mMessageEntry.Message, Btc->ExpectedLine, mMessageEntry.MessageLen + sizeof (CHAR8));

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
  AddTestCase (AdvLoggerWrapperTests, "Basic check", "BasicCheck", BasicTests, NULL, CleanUpTestContext, &mTest00);

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
