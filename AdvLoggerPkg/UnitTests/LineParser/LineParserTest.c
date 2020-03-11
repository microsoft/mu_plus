/** @file
LineParserTestApp.c

This is a Unit Test for the LineParser code.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <AdvancedLoggerInternal.h>

#include <Protocol/AdvancedLogger.h>

#include <Library/AdvancedLoggerAccessLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiLib.h>
#include <Library/UnitTestLib.h>


#define UNIT_TEST_APP_NAME        "LineParser Library test cases"
#define UNIT_TEST_APP_VERSION     "1.0"

// The following text represents the file logger stream of text as individual DEBUG
// statements.

#define ADV_LOG_MAX_SIZE    76     //------ 75 characters + NULL ------------------------------+
//                                                                                             +
//                            1         2         3         4         5         6         7    v
//                   123456789012345678901234567890123456789012345678901234567890123456789012345

CHAR8       *InternalMemoryLog[] = {
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
CHAR8    Line00[] = " 9:06:45.012 First normal test line\n";
CHAR8    Line01[] = " 9:06:45.012 The QueryMode() function returns information for an available graphics mod\n";
CHAR8    Line02[] = " 9:06:45.012 e that the graphics device and the set of active video output devices supp\n";
CHAR8    Line03[] = " 9:06:45.012 orts.\n";
CHAR8    Line04[] = " 9:06:45.012 If ModeNumber is not between 0 and MaxMode - 1, then EFI_INVALID_PARAMETER\n";
CHAR8    Line05[] = " 9:06:45.012  is returned.\n";
CHAR8    Line06[] = " 9:06:45.012 MaxMode is available from the Mode structure of the EFI_GRAPHICS_OUTPUT_PR\n";
CHAR8    Line07[] = " 9:06:45.012 OTOCOL.\n";
CHAR8    Line08[] = " 9:06:45.012 The size of the Info structure should never be assumed and the value of Si\n";
CHAR8    Line09[] = " 9:06:45.012 zeOfInfo is the only valid way to know the size of Info.\n";
CHAR8    Line10[] = " 9:06:45.012 \n";
CHAR8    Line11[] = " 9:06:45.012 If the EFI_GRAPHICS_OUTPUT_PROTOCOL is installed on the handle that repres\n";
CHAR8    Line12[] = " 9:06:45.012 ents a single video output device, then the set of modes returned by this \n";
CHAR8    Line13[] = " 9:06:45.012 service is the subset of modes supported by both the graphics controller a\n";
CHAR8    Line14[] = " 9:06:45.012 nd the video output device.\n";
CHAR8    Line15[] = " 9:06:45.012 \n";
CHAR8    Line16[] = " 9:06:45.012 If the EFI_GRAPHICS_OUTPUT_PROTOCOL is installed on the handle that repres\n";
CHAR8    Line17[] = " 9:06:45.012 ents a combination of video output devices, then the set of modes returned\n";
CHAR8    Line18[] = " 9:06:45.012  by this service is the subset of modes supported by the graphics controll\n";
CHAR8    Line19[] = " 9:06:45.012 er and the all of the video output devices represented by the handle.\n";

/* spell-checker: enable */

ADVANCED_LOGGER_INFO mLoggerInfo = {
    ADVANCED_LOGGER_SIGNATURE,
    0,
    0,
    0,
    0,
    TRUE,
    TRUE,
    FALSE,
    FALSE
};

VOID
EFIAPI
TestLoggerWrite (
    IN  UINTN           ErrorLevel,
    IN  CONST CHAR8    *Buffer,
    IN  UINTN           NumberOfBytes
    ) {

    DEBUG((DEBUG_ERROR, "Function not implemented\n"));
    ASSERT(TRUE);
}

ADVANCED_LOGGER_PROTOCOL mLoggerProtocol = {
    ADVANCED_LOGGER_PROTOCOL_SIGNATURE,
    0,
    TestLoggerWrite,
    NULL
};

ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY mMessageEntry;

/**
  Return a known value of 9:06:45.012 for the TimeStamp
  */
UINT64
InternalGetPerformanceCounter (
    VOID
  ) {

            UINT64  TimeInMs;
            UINT64  TimeInNs;
            UINT64  Frequency;
   STATIC   UINT64  Ticks = 0;


    if (Ticks != 0) {
        return Ticks;
    }

    // Get a time in milliseconds of 9:06:45.012 in ms
    TimeInMs = 9 * 60 * 60 * 1000 +
                6 * 60 * 1000 +
               45 * 1000 +
               12;

    // Time is normally in ns.  But don't multiply by 1E6 and leave time in ms.

    // Since TimerLib is used to convert Ticks to time, use TimerLib to get the
    // frequency of the timer to calculate the Ticks for our fixed time.

    Frequency = GetPerformanceCounterProperties (NULL, NULL);

    //
    //              Ticks
    // TimeInNs = --------- x 1,000,000,000
    //            Frequency
    //
    //     TimeInNs           Ticks
    // ---------------- = -----------
    //   1,000,000,000     Frequency
    //
    //      TimeInNs
    // -------------------- * Frequency = Ticks
    //    1,000,000,000
    //
    //       TimeInMs
    // -------------------- * Frequency = Ticks
    //         1,000
    //
    //       TimeInMs *  Frequency
    // ----------------------------- = Ticks
    //             1,000
    //
    //
    // Do multiply first, then divide, to keep as many bits as possible
    //
    Ticks = DivU64x32 (MultU64x64 (TimeInMs, Frequency), 1000u);
    TimeInNs = GetTimeInNanoSecond (Ticks);

    UT_ASSERT_TRUE (TimeInMs == (TimeInNs / 1000000u));

    return Ticks;
}

UNIT_TEST_STATUS
EFIAPI
InternalTestLoggerWrite (
    IN  UINTN                       DebugLevel,
    IN  CONST CHAR8                *Buffer,
    IN  UINTN                       NumberOfBytes
  ) {
    EFI_PHYSICAL_ADDRESS            CurrentBuffer;
    ADVANCED_LOGGER_MESSAGE_ENTRY  *Entry;
    UINTN                           EntrySize;
    ADVANCED_LOGGER_INFO           *LoggerInfo;

    UT_ASSERT_FALSE((NumberOfBytes == 0) || (Buffer == NULL));

    UT_ASSERT_FALSE(NumberOfBytes > MAX_UINT16);

    LoggerInfo = &mLoggerInfo;
    EntrySize = MESSAGE_ENTRY_SIZE(NumberOfBytes);
    CurrentBuffer = LoggerInfo->LogCurrent;

    UT_ASSERT_TRUE ( (LoggerInfo->LogBufferSize -
                    ((UINTN)(CurrentBuffer - LoggerInfo->LogBuffer))) > NumberOfBytes);

    LoggerInfo->LogCurrent = PA_FROM_PTR((CHAR8_FROM_PA(LoggerInfo->LogCurrent) + EntrySize));
    Entry = (ADVANCED_LOGGER_MESSAGE_ENTRY *) PTR_FROM_PA(CurrentBuffer);
    if  ((Entry != (ADVANCED_LOGGER_MESSAGE_ENTRY *) ALIGN_POINTER(Entry, 8)) ||               // Insure pointer is on boundary
         (Entry <  (ADVANCED_LOGGER_MESSAGE_ENTRY *) PTR_FROM_PA(LoggerInfo->LogBuffer)) ||    // and within the log region
         (Entry >  (ADVANCED_LOGGER_MESSAGE_ENTRY *) PTR_FROM_PA(LoggerInfo->LogBuffer + LoggerInfo->LogBufferSize))) {
        UT_ASSERT_TRUE(FALSE);
    }

    Entry->TimeStamp = InternalGetPerformanceCounter();

    // DebugLevel is defined as a UINTN, so it is 32 bits in PEI and 64 bits in DXE.
    // However, the DEBUG_* values and the PcdFixedDebugPrintErrorLevel are only 32 bits.
    Entry->DebugLevel = (UINT32) DebugLevel;
    Entry->MessageLen = (UINT16) NumberOfBytes;
    CopyMem(Entry->MessageText, Buffer, NumberOfBytes);
    Entry->Signature = MESSAGE_ENTRY_SIGNATURE;

    return UNIT_TEST_PASSED;
}

typedef struct {
    CHAR8                *IdString;
    CHAR8                *ExpectedLine;
    VOID                 *MemoryToFree;
    EFI_STATUS            ExpectedStatus;
} BASIC_TEST_CONTEXT;

//*----------------------------------------------------------------------------------*
//* Test Contexts                                                                    *
//*----------------------------------------------------------------------------------*
STATIC BASIC_TEST_CONTEXT mTest00  = { "Basic tests", Line00, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest01  = { "Basic tests", Line01, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest02  = { "Basic tests", Line02, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest03  = { "Basic tests", Line03, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest04  = { "Basic tests", Line04, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest05  = { "Basic tests", Line05, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest06  = { "Basic tests", Line06, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest07  = { "Basic tests", Line07, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest08  = { "Basic tests", Line08, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest09  = { "Basic tests", Line09, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest10  = { "Basic tests", Line10, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest11  = { "Basic tests", Line11, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest12  = { "Basic tests", Line12, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest13  = { "Basic tests", Line13, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest14  = { "Basic tests", Line14, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest15  = { "Basic tests", Line15, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest16  = { "Basic tests", Line16, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest17  = { "Basic tests", Line17, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest18  = { "Basic tests", Line18, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest19  = { "Basic tests", Line19, NULL, EFI_SUCCESS };
STATIC BASIC_TEST_CONTEXT mTest20  = { "End Of File", NULL,   NULL, EFI_END_OF_FILE };


///================================================================================================
///================================================================================================
///
/// HELPER FUNCTIONS
///
///================================================================================================
///================================================================================================

/*
    CleanUpTestContext

    Cleans up after a test case.  Free any allocated buffers if a test
    takes the error exit.

*/
STATIC
VOID
EFIAPI
CleanUpTestContext (
    IN UNIT_TEST_CONTEXT           Context
  ) {
    BASIC_TEST_CONTEXT *Btc;

    Btc = (BASIC_TEST_CONTEXT *) Context;

    if (NULL != Btc->MemoryToFree) {
        FreePool (Btc->MemoryToFree);
        Btc->MemoryToFree = NULL;
    }
}

///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================

/*
    Initialize the test in memory log

*/
STATIC
UNIT_TEST_STATUS
EFIAPI
InitializeInMemoryLog (
    IN UNIT_TEST_CONTEXT           Context
  ) {
    UINTN               i;
    EFI_STATUS          Status;
    UNIT_TEST_STATUS    UnitTestStatus;

    if (mLoggerInfo.LogBuffer != 0LL) {
        return UNIT_TEST_PASSED;
    }

#define IN_MEMORY_PAGES 32 // 1 MB test memory log

    mLoggerInfo.LogBuffer = (EFI_PHYSICAL_ADDRESS) AllocatePages (IN_MEMORY_PAGES);
    mLoggerInfo.LogBufferSize = EFI_PAGE_SIZE * IN_MEMORY_PAGES;
    mLoggerInfo.LogCurrent = mLoggerInfo.LogBuffer;

    for (i = 0; i < ARRAY_SIZE(InternalMemoryLog); i++) {
        UnitTestStatus = InternalTestLoggerWrite ( (i % 5) == 0 ? DEBUG_INFO : DEBUG_ERROR,
                                                   InternalMemoryLog[i],
                                                   AsciiStrLen (InternalMemoryLog[i]));
        UT_ASSERT_TRUE(UnitTestStatus == UNIT_TEST_PASSED);
    }

    mLoggerProtocol.Context = (VOID *) &mLoggerInfo;
    Status = AdvancedLoggerAccessLibUnitTestInitialize (&mLoggerProtocol, ADV_LOG_MAX_SIZE);
    UT_ASSERT_NOT_EFI_ERROR(Status)

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
    IN UNIT_TEST_CONTEXT           Context
  ) {
    BASIC_TEST_CONTEXT *Btc;
    EFI_STATUS          Status;


    Btc = (BASIC_TEST_CONTEXT *) Context;

    Status = AdvancedLoggerAccessLibGetNextFormattedLine (&mMessageEntry);

    UT_ASSERT_STATUS_EQUAL(Status, Btc->ExpectedStatus);
    UT_ASSERT_NOT_NULL (mMessageEntry.Message);
    UT_LOG_INFO ("Return Length=%d\n", mMessageEntry.MessageLen);
    UT_LOG_INFO ("\n = %a =\n", mMessageEntry.Message);
    UT_ASSERT_EQUAL (mMessageEntry.MessageLen, AsciiStrLen(Btc->ExpectedLine));

    // The following also verifies that the string is NULL terminated.
    UT_ASSERT_MEM_EQUAL (mMessageEntry.Message, Btc->ExpectedLine, mMessageEntry.MessageLen + sizeof(CHAR8));

    return UNIT_TEST_PASSED;
}


/*
    EOFTest

    Validates that the end of DEBUG print blocks properly returns END_OF_FILE.

    This test turns off the private logger info protocol.  Any test after this
    will use the real logger info protocol unless AdvancedLoggerAccessLibUnitTestInitialize
    is called again.
*/
STATIC
UNIT_TEST_STATUS
EFIAPI
EOFTest (
    IN UNIT_TEST_CONTEXT           Context
  ) {
    BASIC_TEST_CONTEXT *Btc;
    EFI_STATUS          Status;

    Btc = (BASIC_TEST_CONTEXT *) Context;

    Status = AdvancedLoggerAccessLibGetNextFormattedLine (&mMessageEntry);
    UT_ASSERT_STATUS_EQUAL(Status, Btc->ExpectedStatus);

    Status = AdvancedLoggerAccessLibUnitTestInitialize (NULL, 0);
    UT_ASSERT_STATUS_EQUAL(Status, EFI_SUCCESS);

    Status = AdvancedLoggerAccessLibReset(&mMessageEntry);
    UT_ASSERT_STATUS_EQUAL(Status, EFI_SUCCESS);


    return UNIT_TEST_PASSED;
}


///================================================================================================
///================================================================================================
///
/// TEST ENGINE
///
///================================================================================================
///================================================================================================


/**
  DeviceIdTestAppEntry

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
LineParserTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    UNIT_TEST_FRAMEWORK_HANDLE  Fw;
    UNIT_TEST_SUITE_HANDLE      LineParserTests;
    EFI_STATUS                  Status;

    Fw = (UNIT_TEST_FRAMEWORK_HANDLE)NULL;
    AsciiPrint ("%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION );
    DEBUG((DEBUG_ERROR, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

    ZeroMem (&mMessageEntry, sizeof(mMessageEntry));

    //
    // Start setting up the test framework for running the tests.
    //
    Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
    if (EFI_ERROR( Status )) {
        AsciiPrint ("Failed in InitUnitTestFramework. Status = %r\n", Status);
        goto EXIT;
    }

    //
    // Populate the DeviceId Library Test Suite.
    //
    Status = CreateUnitTestSuite( &LineParserTests, Fw, "Validate Line parser returns valid data", "LineParser.Test", NULL, NULL);
    if (EFI_ERROR( Status )) {
        AsciiPrint ("Failed in CreateUnitTestSuite for Line Parser Tests\n");
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;
    }

    //-----------Suite------------Description-------Class---------Test Function-Pre---Clean-Context
    AddTestCase( LineParserTests, "Init",          "SelfInit",  InitializeInMemoryLog, NULL, NULL,      NULL);
    AddTestCase( LineParserTests, "Basic check",   "BasicCheck", BasicTests,  NULL, CleanUpTestContext, &mTest00);
    AddTestCase( LineParserTests, "Line check  1", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest01);
    AddTestCase( LineParserTests, "Line check  2", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest02);
    AddTestCase( LineParserTests, "Line check  3", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest03);
    AddTestCase( LineParserTests, "Line check  4", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest04);
    AddTestCase( LineParserTests, "Line check  5", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest05);
    AddTestCase( LineParserTests, "Line check  6", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest06);
    AddTestCase( LineParserTests, "Line check  7", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest07);
    AddTestCase( LineParserTests, "Line check  8", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest08);
    AddTestCase( LineParserTests, "Line check  9", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest09);
    AddTestCase( LineParserTests, "Line check 10", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest10);
    AddTestCase( LineParserTests, "Line check 11", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest11);
    AddTestCase( LineParserTests, "Line check 12", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest12);
    AddTestCase( LineParserTests, "Line check 13", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest13);
    AddTestCase( LineParserTests, "Line check 14", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest14);
    AddTestCase( LineParserTests, "Line check 15", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest15);
    AddTestCase( LineParserTests, "Line check 16", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest16);
    AddTestCase( LineParserTests, "Line check 17", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest17);
    AddTestCase( LineParserTests, "Line check 18", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest18);
    AddTestCase( LineParserTests, "Line check 19", "SelfCheck", BasicTests,   NULL, CleanUpTestContext, &mTest19);
    AddTestCase( LineParserTests, "Check EOF",     "SelfCheck", EOFTest,      NULL, CleanUpTestContext, &mTest20);

    //
    // Execute the tests.
    //
    Status = RunAllTestSuites (Fw);

EXIT:
    if (Fw) {
        FreeUnitTestFramework (Fw);
    }

    if (mLoggerInfo.LogBuffer != 0LL) {
        FreePages ((VOID *) mLoggerInfo.LogBuffer, IN_MEMORY_PAGES);
        mLoggerInfo.LogBuffer = 0LL;
    }

    return Status;
}