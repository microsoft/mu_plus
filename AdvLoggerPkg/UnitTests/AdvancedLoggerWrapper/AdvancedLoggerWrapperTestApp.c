/** @file
AdvancedLoggerWrapperTestApp.c

This is a Unit Test for the AdvancedLoggerWrapper code.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <AdvancedLoggerInternal.h>

#include <Protocol/AdvancedLogger.h>
#include <Protocol/MpService.h>
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
#define ADV_TIME_TEST_STR         "Test"
#define ADV_WRAP_TEST_STR         "DEADBEEF\n"

// The following text represents the output lines from the line parser given the above input

/* spell-checker: disable */
CHAR8  Line00[] = "09:06:45.012 : [DXE] DEADBEEF\n";
CHAR8  Line01[] = "09:06:45.012 : [DXE] 00000000DEADBEEF\n";

extern CONST   CHAR8  *AdvMsgEntryPrefix[ADVANCED_LOGGER_PHASE_CNT];

ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY  mMessageEntry;
ADVANCED_LOGGER_INFO                       *mLoggerInfo;
EFI_PHYSICAL_ADDRESS                       mMaxAddress  = 0;
UINT32                                     mBufferSize  = 0;
EFI_MP_SERVICES_PROTOCOL                   *mMpServicesProtocol = NULL;

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

/**
  CleanUpTestContext

  Cleans up after a test case.  Free any allocated buffers if a test
  takes the error exit.
**/
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

  ZeroMem (&mMessageEntry, sizeof (mMessageEntry));
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

  // Get ahold of the MP services protocol
  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&mMpServicesProtocol);

  return UNIT_TEST_PASSED;
}

/**
  Basic Tests

  Validates that the DEBUG print blocks are returned as null terminated lines.

  @param[in] Context    The test context

  @retval UNIT_TEST_PASSED    The test passed.
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
  while (mLoggerInfo->LogCurrent + MESSAGE_ENTRY_SIZE_V2 (sizeof (ADVANCED_LOGGER_MESSAGE_ENTRY_V2), sizeof (ADV_TIME_TEST_STR)) < mMaxAddress) {
    AdvancedLoggerWrite (DEBUG_ERROR, ADV_TIME_TEST_STR, sizeof (ADV_TIME_TEST_STR));
  }

  // This is the last logging that should push the buffer over the edge
  AdvancedLoggerWrite (DEBUG_ERROR, ADV_WRAP_TEST_STR, sizeof (ADV_WRAP_TEST_STR));

  Status = AdvancedLoggerAccessLibGetNextFormattedLine (&mMessageEntry);

  UT_ASSERT_STATUS_EQUAL (mLoggerInfo->LogCurrent - mLoggerInfo->LogBuffer, MESSAGE_ENTRY_SIZE_V2 (sizeof (ADVANCED_LOGGER_MESSAGE_ENTRY_V2), sizeof (ADV_WRAP_TEST_STR)));
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


  Btc->MemoryToFree = mMessageEntry.Message;

  return UNIT_TEST_PASSED;
}

/**
  @brief This is the AP procedure that will be run on each AP. It will log a message
         and then return.

  @param Buffer - A pointer to the buffer that was passed in to the AP startup code.
**/
VOID
EFIAPI
ApProcedure (
  IN OUT VOID  *Buffer
  )
{
  UINTN Index;
  UINTN Size;
  CHAR8 AsciiBuffer[8 + sizeof (ADV_WRAP_TEST_STR)];

  // First figure out the current MP index
  mMpServicesProtocol->WhoAmI (mMpServicesProtocol, &Index);

  // Then fill out the string buffer with the index
  Size = AsciiSPrint (AsciiBuffer, sizeof (AsciiBuffer), "%08x%a", Index, ADV_WRAP_TEST_STR);

  // This is the last logging that should push the buffer over the edge
  AdvancedLoggerWrite (DEBUG_ERROR, AsciiBuffer, Size);
}

/**
  Basic Tests in MP context.

  Validates that the DEBUG print blocks are returned as null terminated lines.

  This test is the same as TestCursorWrapping, but it uses the MP version of the
  AdvancedLoggerAccessLibGetNextFormattedLine function.

  @param[in] Context    The test context

  @retval UNIT_TEST_PASSED    The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestCursorWrappingMP (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BASIC_TEST_CONTEXT  *Btc;
  EFI_STATUS          Status;
  UINTN               Index;
  UINTN               StrIndex;
  UINTN               NumberOfProcessors;
  UINTN               EnabledProcessors;
  UINT8               *TempCache = NULL;
  UNIT_TEST_STATUS    UtStatus;
  UINTN               PrefixSize;
  CHAR8               *EndPointer;

  Btc = (BASIC_TEST_CONTEXT *)Context;

  // First fill in the buffer
  while (mLoggerInfo->LogCurrent + MESSAGE_ENTRY_SIZE_V2 (sizeof (ADVANCED_LOGGER_MESSAGE_ENTRY_V2), sizeof (ADV_TIME_TEST_STR)) < mMaxAddress) {
    AdvancedLoggerWrite (DEBUG_ERROR, ADV_TIME_TEST_STR, sizeof (ADV_TIME_TEST_STR));
  }

  if (mMpServicesProtocol == NULL) {
    UtStatus = UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
    goto Done;
  }

  Status = mMpServicesProtocol->StartupAllAPs (
                         mMpServicesProtocol,
                         (EFI_AP_PROCEDURE)ApProcedure,
                         FALSE,
                         NULL,
                         0,
                         NULL,
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    UtStatus = UNIT_TEST_ERROR_TEST_FAILED;
    goto Done;
  }

  // BSP needs to run the procedure as well...
  ApProcedure (NULL);

  Status = mMpServicesProtocol->GetNumberOfProcessors (mMpServicesProtocol, &NumberOfProcessors, &EnabledProcessors);
  if (EFI_ERROR (Status)) {
    UtStatus = UNIT_TEST_ERROR_TEST_FAILED;
    goto Done;
  }

  TempCache = AllocatePool (NumberOfProcessors * sizeof (UINT8));
  if (TempCache == NULL) {
    UtStatus = UNIT_TEST_ERROR_TEST_FAILED;
    goto Done;
  }

  // Initialize the cache to 0xFF
  SetMem (TempCache, NumberOfProcessors * sizeof (UINT8), 0xFF);

  for (Index = 0; Index < NumberOfProcessors; Index++) {
    Status = AdvancedLoggerAccessLibGetNextFormattedLine (&mMessageEntry);
    if (EFI_ERROR (Status)) {
      UtStatus = UNIT_TEST_ERROR_TEST_FAILED;
      goto Done;
    }

    // HACKHACK: Bypass the potential print out from MpLib
    if ((Index == 0) && AsciiStrStr (mMessageEntry.Message, "5-Level Paging") != NULL) {
      Index--;
      continue;
    }

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
    PrefixSize = AsciiStrLen (AdvMsgEntryPrefix[ADVANCED_LOGGER_PHASE_DXE]);
    UT_ASSERT_MEM_EQUAL (&mMessageEntry.Message[ADV_TIME_STAMP_PREFIX_LEN],
    AdvMsgEntryPrefix[ADVANCED_LOGGER_PHASE_DXE],
    PrefixSize);

    // The following verifies that the string content is NULL terminated and matches expectation.
    UT_ASSERT_MEM_EQUAL (&mMessageEntry.Message[ADV_TIME_STAMP_PREFIX_LEN + PrefixSize + 8],
    ADV_WRAP_TEST_STR,
    sizeof (ADV_WRAP_TEST_STR)
    );

    // Now check the index
    EndPointer = &mMessageEntry.Message[ADV_TIME_STAMP_PREFIX_LEN + sizeof (ADV_WRAP_TEST_STR) + 8 - 1];
    Status = AsciiStrHexToUintnS (&mMessageEntry.Message[ADV_TIME_STAMP_PREFIX_LEN + sizeof (ADV_WRAP_TEST_STR) - 1],
    &EndPointer,
    &StrIndex);
    if (EFI_ERROR (Status)) {
      UtStatus = UNIT_TEST_ERROR_TEST_FAILED;
      goto Done;
    }

    if (StrIndex >= NumberOfProcessors) {
      // Index is out of range
      UtStatus = UNIT_TEST_ERROR_TEST_FAILED;
      goto Done;
    }

    if (TempCache[StrIndex] == 0) {
      // Printed this index already
      UtStatus = UNIT_TEST_ERROR_TEST_FAILED;
      goto Done;
    } else {
      TempCache[StrIndex] = 0;
    }
  }

  if (!IsZeroBuffer (TempCache, NumberOfProcessors * sizeof (UINT8))) {
    UtStatus = UNIT_TEST_ERROR_TEST_FAILED;
    goto Done;
  }


Done:
  if (TempCache != NULL) {
    FreePool (TempCache);
  }

  Btc->MemoryToFree = mMessageEntry.Message;

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
  AddTestCase (AdvLoggerWrapperTests, "Basic check in MP Context", "BasicCheckInMP", TestCursorWrappingMP, NULL, CleanUpTestContext, &mTest01);

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
