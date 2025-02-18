/** @file
LineParserTestApp.c

This is a Unit Test for the LineParser code.

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

#define UNIT_TEST_APP_NAME     "LineParser Library test cases"
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
  "supported by the graphics controller and all of the video output ",
  "devices represented by the handle.\n"
};

// The following text represents the output lines from the line parser given the above input

/* spell-checker: disable */
CHAR8  Line00[] = "09:06:45.012 : [INFO] First normal test line\n";
CHAR8  Line01[] = "09:06:45.012 : [ERR ] The QueryMode() function returns information for an available graphics mod\n";
CHAR8  Line02[] = "09:06:45.012 : [ERR ] e that the graphics device and the set of active video output devices supp\n";
CHAR8  Line03[] = "09:06:45.012 : [ERR ] orts.\n";
CHAR8  Line04[] = "09:06:45.012 : [ERR ] If ModeNumber is not between 0 and MaxMode - 1, then EFI_INVALID_PARAMETER\n";
CHAR8  Line05[] = "09:06:45.012 : [ERR ]  is returned.\n";
CHAR8  Line06[] = "09:06:45.012 : [INFO] MaxMode is available from the Mode structure of the EFI_GRAPHICS_OUTPUT_PR\n";
CHAR8  Line07[] = "09:06:45.012 : [INFO] OTOCOL.\n";
CHAR8  Line08[] = "09:06:45.012 : [ERR ] The size of the Info structure should never be assumed and the value of Si\n";
CHAR8  Line09[] = "09:06:45.012 : [ERR ] zeOfInfo is the only valid way to know the size of Info.\n";
CHAR8  Line10[] = "09:06:45.012 : [ERR ] \n";
CHAR8  Line11[] = "09:06:45.012 : [ERR ] If the EFI_GRAPHICS_OUTPUT_PROTOCOL is installed on the handle that repres\n";
CHAR8  Line12[] = "09:06:45.012 : [INFO] ents a single video output device, then the set of modes returned by this \n";
CHAR8  Line13[] = "09:06:45.012 : [ERR ] service is the subset of modes supported by both the graphics controller a\n";
CHAR8  Line14[] = "09:06:45.012 : [ERR ] nd the video output device.\n";
CHAR8  Line15[] = "09:06:45.012 : [ERR ] \n";
CHAR8  Line16[] = "09:06:45.012 : [ERR ] If the EFI_GRAPHICS_OUTPUT_PROTOCOL is installed on the handle that repres\n";
CHAR8  Line17[] = "09:06:45.012 : [ERR ] ents a combination of video output devices, then the set of modes returned\n";
CHAR8  Line18[] = "09:06:45.012 : [INFO]  by this service is the subset of modes supported by the graphics controll\n";
CHAR8  Line19[] = "09:06:45.012 : [ERR ] er and all of the video output devices represented by the handle.\n";

CHAR8  Line00V2[] = "09:06:45.012 : [DXE  ] [INFO] First normal test line\n";
CHAR8  Line01V2[] = "09:06:45.012 : [DXE  ] [ERR ] The QueryMode() function returns information for an available graphics mod\n";
CHAR8  Line02V2[] = "09:06:45.012 : [DXE  ] [ERR ] e that the graphics device and the set of active video output devices supp\n";
CHAR8  Line03V2[] = "09:06:45.012 : [DXE  ] [ERR ] orts.\n";
CHAR8  Line04V2[] = "09:06:45.012 : [DXE  ] [ERR ] If ModeNumber is not between 0 and MaxMode - 1, then EFI_INVALID_PARAMETER\n";
CHAR8  Line05V2[] = "09:06:45.012 : [DXE  ] [ERR ]  is returned.\n";
CHAR8  Line06V2[] = "09:06:45.012 : [DXE  ] [INFO] MaxMode is available from the Mode structure of the EFI_GRAPHICS_OUTPUT_PR\n";
CHAR8  Line07V2[] = "09:06:45.012 : [DXE  ] [INFO] OTOCOL.\n";
CHAR8  Line08V2[] = "09:06:45.012 : [DXE  ] [ERR ] The size of the Info structure should never be assumed and the value of Si\n";
CHAR8  Line09V2[] = "09:06:45.012 : [DXE  ] [ERR ] zeOfInfo is the only valid way to know the size of Info.\n";
CHAR8  Line10V2[] = "09:06:45.012 : [DXE  ] [ERR ] \n";
CHAR8  Line11V2[] = "09:06:45.012 : [DXE  ] [ERR ] If the EFI_GRAPHICS_OUTPUT_PROTOCOL is installed on the handle that repres\n";
CHAR8  Line12V2[] = "09:06:45.012 : [DXE  ] [INFO] ents a single video output device, then the set of modes returned by this \n";
CHAR8  Line13V2[] = "09:06:45.012 : [DXE  ] [ERR ] service is the subset of modes supported by both the graphics controller a\n";
CHAR8  Line14V2[] = "09:06:45.012 : [DXE  ] [ERR ] nd the video output device.\n";
CHAR8  Line15V2[] = "09:06:45.012 : [DXE  ] [ERR ] \n";
CHAR8  Line16V2[] = "09:06:45.012 : [DXE  ] [ERR ] If the EFI_GRAPHICS_OUTPUT_PROTOCOL is installed on the handle that repres\n";
CHAR8  Line17V2[] = "09:06:45.012 : [DXE  ] [ERR ] ents a combination of video output devices, then the set of modes returned\n";
CHAR8  Line18V2[] = "09:06:45.012 : [DXE  ] [INFO]  by this service is the subset of modes supported by the graphics controll\n";
CHAR8  Line19V2[] = "09:06:45.012 : [DXE  ] [ERR ] er and all of the video output devices represented by the handle.\n";

/* spell-checker: enable */

STATIC ADVANCED_LOGGER_INFO  *mLoggerInfo = NULL;

VOID
EFIAPI
TestLoggerWrite (
  IN        ADVANCED_LOGGER_PROTOCOL  *AdvancedLoggerProtocol OPTIONAL,
  IN  UINTN                           ErrorLevel,
  IN  CONST CHAR8                     *Buffer,
  IN  UINTN                           NumberOfBytes
  )
{
  DEBUG ((DEBUG_ERROR, "Function not implemented\n"));
  ASSERT (TRUE);
}

STATIC ADVANCED_LOGGER_PROTOCOL_CONTAINER  mLoggerProtocol = {
  .AdvLoggerProtocol             = {
    .Signature                   = ADVANCED_LOGGER_PROTOCOL_SIGNATURE,
    .Version                     = ADVANCED_LOGGER_PROTOCOL_VERSION,
    .AdvancedLoggerWriteProtocol = TestLoggerWrite
  },
  .LoggerInfo                    = NULL
};

ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY  mMessageEntry;

/**
  Return a known value of 9:06:45.012 for the TimeStamp
  */
UINT64
InternalGetPerformanceCounter (
  VOID
  )
{
  UINT64           TimeInMs;
  UINT64           TimeInNs;
  UINT64           Frequency;
  UINT32           Remainder = 0;
  STATIC   UINT64  Ticks     = 0;

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
  Ticks    = DivU64x32Remainder (MultU64x64 (TimeInMs, Frequency), 1000u, &Remainder) + Remainder;
  TimeInNs = GetTimeInNanoSecond (Ticks);

  UT_ASSERT_TRUE (TimeInMs == (TimeInNs / 1000000u));

  return Ticks;
}

UNIT_TEST_STATUS
EFIAPI
InternalTestLoggerWrite (
  IN  UINTN        DebugLevel,
  IN  CONST CHAR8  *Buffer,
  IN  UINTN        NumberOfBytes
  )
{
  EFI_PHYSICAL_ADDRESS           CurrentBuffer;
  ADVANCED_LOGGER_MESSAGE_ENTRY  *Entry;
  UINTN                          EntrySize;
  ADVANCED_LOGGER_INFO           *LoggerInfo;

  UT_ASSERT_FALSE ((NumberOfBytes == 0) || (Buffer == NULL));

  UT_ASSERT_FALSE (NumberOfBytes > MAX_UINT16);

  LoggerInfo    = mLoggerInfo;
  EntrySize     = MESSAGE_ENTRY_SIZE (NumberOfBytes);
  CurrentBuffer = PA_FROM_PTR (LOG_CURRENT_FROM_ALI (LoggerInfo));

  UT_ASSERT_TRUE (
    (LoggerInfo->LogBufferSize -
     ((UINTN)(LoggerInfo->LogCurrentOffset - LoggerInfo->LogBufferOffset))) > NumberOfBytes
    );

  LoggerInfo->LogCurrentOffset = LoggerInfo->LogCurrentOffset + (UINT32)EntrySize;
  Entry                        = (ADVANCED_LOGGER_MESSAGE_ENTRY *)PTR_FROM_PA (CurrentBuffer);
  if ((Entry != (ADVANCED_LOGGER_MESSAGE_ENTRY *)ALIGN_POINTER (Entry, 8)) ||                       // Insure pointer is on boundary
      (Entry <  (ADVANCED_LOGGER_MESSAGE_ENTRY *)PTR_FROM_PA (LOG_BUFFER_FROM_ALI (LoggerInfo))) || // and within the log region
      (Entry >  (ADVANCED_LOGGER_MESSAGE_ENTRY *)PTR_FROM_PA ((UINT8 *)LoggerInfo + TOTAL_LOG_SIZE_WITH_ALI (LoggerInfo))))
  {
    UT_ASSERT_TRUE (FALSE);
  }

  Entry->TimeStamp = InternalGetPerformanceCounter ();

  // DebugLevel is defined as a UINTN, so it is 32 bits in PEI and 64 bits in DXE.
  // However, the DEBUG_* values and the PcdFixedDebugPrintErrorLevel are only 32 bits.
  Entry->DebugLevel = (UINT32)DebugLevel;
  Entry->MessageLen = (UINT16)NumberOfBytes;
  CopyMem (Entry->MessageText, Buffer, NumberOfBytes);
  Entry->Signature = MESSAGE_ENTRY_SIGNATURE;

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
InternalTestLoggerWriteV2 (
  IN  UINTN        DebugLevel,
  IN  CONST CHAR8  *Buffer,
  IN  UINTN        NumberOfBytes
  )
{
  EFI_PHYSICAL_ADDRESS              CurrentBuffer;
  ADVANCED_LOGGER_MESSAGE_ENTRY_V2  *Entry;
  UINTN                             EntrySize;
  ADVANCED_LOGGER_INFO              *LoggerInfo;

  UT_ASSERT_FALSE ((NumberOfBytes == 0) || (Buffer == NULL));

  UT_ASSERT_FALSE (NumberOfBytes > MAX_UINT16);

  LoggerInfo    = mLoggerInfo;
  EntrySize     = MESSAGE_ENTRY_SIZE_V2 (OFFSET_OF (ADVANCED_LOGGER_MESSAGE_ENTRY_V2, MessageText), NumberOfBytes);
  CurrentBuffer = PA_FROM_PTR (LOG_CURRENT_FROM_ALI (LoggerInfo));

  UT_ASSERT_TRUE (
    (LoggerInfo->LogBufferSize -
     ((UINTN)(LoggerInfo->LogCurrentOffset - LoggerInfo->LogBufferOffset))) > NumberOfBytes
    );

  LoggerInfo->LogCurrentOffset = LoggerInfo->LogCurrentOffset + (UINT32)EntrySize;
  Entry                        = (ADVANCED_LOGGER_MESSAGE_ENTRY_V2 *)PTR_FROM_PA (CurrentBuffer);
  if ((Entry != (ADVANCED_LOGGER_MESSAGE_ENTRY_V2 *)ALIGN_POINTER (Entry, 8)) ||                       // Insure pointer is on boundary
      (Entry <  (ADVANCED_LOGGER_MESSAGE_ENTRY_V2 *)PTR_FROM_PA (LOG_BUFFER_FROM_ALI (LoggerInfo))) || // and within the log region
      (Entry >  (ADVANCED_LOGGER_MESSAGE_ENTRY_V2 *)PTR_FROM_PA ((UINT8 *)LoggerInfo + TOTAL_LOG_SIZE_WITH_ALI (LoggerInfo))))
  {
    UT_ASSERT_TRUE (FALSE);
  }

  Entry->MajorVersion = ADVANCED_LOGGER_MSG_MAJ_VER;
  Entry->MinorVersion = ADVANCED_LOGGER_MSG_MIN_VER;
  Entry->TimeStamp    = InternalGetPerformanceCounter ();
  Entry->Phase        = ADVANCED_LOGGER_PHASE_DXE;

  // DebugLevel is defined as a UINTN, so it is 32 bits in PEI and 64 bits in DXE.
  // However, the DEBUG_* values and the PcdFixedDebugPrintErrorLevel are only 32 bits.
  Entry->DebugLevel    = (UINT32)DebugLevel;
  Entry->MessageLen    = (UINT16)NumberOfBytes;
  Entry->MessageOffset = OFFSET_OF (ADVANCED_LOGGER_MESSAGE_ENTRY_V2, MessageText);
  CopyMem (Entry->MessageText, Buffer, NumberOfBytes);
  Entry->Signature = MESSAGE_ENTRY_SIGNATURE_V2;

  return UNIT_TEST_PASSED;
}

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
    Initialize the test in memory log

*/
STATIC
UNIT_TEST_STATUS
EFIAPI
InitializeInMemoryLog (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN             i;
  EFI_STATUS        Status;
  UNIT_TEST_STATUS  UnitTestStatus;

  if (mLoggerInfo != NULL) {
    return UNIT_TEST_PASSED;
  }

  #define IN_MEMORY_PAGES  32// 1 MB test memory log

  mLoggerInfo = AllocatePages (IN_MEMORY_PAGES);
  if (mLoggerInfo == NULL) {
    UT_ASSERT_TRUE (FALSE);
  }

  mLoggerInfo->Signature        = ADVANCED_LOGGER_SIGNATURE;
  mLoggerInfo->GoneVirtual      = FALSE;
  mLoggerInfo->AtRuntime        = FALSE;
  mLoggerInfo->LogBufferSize    = EFI_PAGE_SIZE * IN_MEMORY_PAGES - sizeof (*mLoggerInfo);
  mLoggerInfo->LogBufferOffset  = sizeof (*mLoggerInfo);
  mLoggerInfo->LogCurrentOffset = mLoggerInfo->LogBufferOffset;

  for (i = 0; i < ARRAY_SIZE (InternalMemoryLog); i++) {
    UnitTestStatus = InternalTestLoggerWrite (
                       (i % 5) == 0 ? DEBUG_INFO : DEBUG_ERROR,
                       InternalMemoryLog[i],
                       AsciiStrLen (InternalMemoryLog[i])
                       );
    UT_ASSERT_TRUE (UnitTestStatus == UNIT_TEST_PASSED);
  }

  mLoggerProtocol.LoggerInfo = mLoggerInfo;
  Status                     = AdvancedLoggerAccessLibUnitTestInitialize (&mLoggerProtocol.AdvLoggerProtocol, ADV_LOG_MAX_SIZE);
  UT_ASSERT_NOT_EFI_ERROR (Status)

  return UNIT_TEST_PASSED;
}

/*
    Initialize the v2 test in memory log

*/
STATIC
UNIT_TEST_STATUS
EFIAPI
InitializeInMemoryLogV2 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN             i;
  EFI_STATUS        Status;
  UNIT_TEST_STATUS  UnitTestStatus;

  if (mLoggerInfo != NULL) {
    FreePages ((VOID *)mLoggerInfo, IN_MEMORY_PAGES);
    mLoggerInfo = NULL;
  }

  // Repopulate the content with v2 messages
  mLoggerInfo = AllocatePages (IN_MEMORY_PAGES);
  if (mLoggerInfo == NULL) {
    UT_ASSERT_TRUE (FALSE);
  }

  mLoggerInfo->Signature        = ADVANCED_LOGGER_SIGNATURE;
  mLoggerInfo->GoneVirtual      = FALSE;
  mLoggerInfo->AtRuntime        = FALSE;
  mLoggerInfo->LogBufferSize    = EFI_PAGE_SIZE * IN_MEMORY_PAGES - sizeof (*mLoggerInfo);
  mLoggerInfo->LogBufferOffset  = sizeof (*mLoggerInfo);
  mLoggerInfo->LogCurrentOffset = mLoggerInfo->LogBufferOffset;

  ZeroMem (&mMessageEntry, sizeof (mMessageEntry));

  for (i = 0; i < ARRAY_SIZE (InternalMemoryLog); i++) {
    UnitTestStatus = InternalTestLoggerWriteV2 (
                       (i % 5) == 0 ? DEBUG_INFO : DEBUG_ERROR,
                       InternalMemoryLog[i],
                       AsciiStrLen (InternalMemoryLog[i])
                       );
    UT_ASSERT_TRUE (UnitTestStatus == UNIT_TEST_PASSED);
  }

  mLoggerProtocol.LoggerInfo = mLoggerInfo;
  Status                     = AdvancedLoggerAccessLibUnitTestInitialize (&mLoggerProtocol.AdvLoggerProtocol, ADV_LOG_MAX_SIZE);
  UT_ASSERT_NOT_EFI_ERROR (Status)

  return UNIT_TEST_PASSED;
}

/*
    Initialize the v1 and v2 mixed test in memory log

*/
STATIC
UNIT_TEST_STATUS
EFIAPI
InitializeInMemoryLogV2Hybrid (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN             i;
  EFI_STATUS        Status;
  UNIT_TEST_STATUS  UnitTestStatus;

  if (mLoggerInfo != NULL) {
    FreePages ((VOID *)mLoggerInfo, IN_MEMORY_PAGES);
    mLoggerInfo = NULL;
  }

  mLoggerInfo = AllocatePages (IN_MEMORY_PAGES);
  if (mLoggerInfo == NULL) {
    UT_ASSERT_TRUE (FALSE);
  }

  mLoggerInfo->Signature        = ADVANCED_LOGGER_SIGNATURE;
  mLoggerInfo->GoneVirtual      = FALSE;
  mLoggerInfo->AtRuntime        = FALSE;
  mLoggerInfo->LogBufferSize    = EFI_PAGE_SIZE * IN_MEMORY_PAGES - sizeof (*mLoggerInfo);
  mLoggerInfo->LogBufferOffset  = sizeof (*mLoggerInfo);
  mLoggerInfo->LogCurrentOffset = mLoggerInfo->LogBufferOffset;

  ZeroMem (&mMessageEntry, sizeof (mMessageEntry));

  for (i = 0; i < 8; i++) {
    UnitTestStatus = InternalTestLoggerWrite (
                       (i % 5) == 0 ? DEBUG_INFO : DEBUG_ERROR,
                       InternalMemoryLog[i],
                       AsciiStrLen (InternalMemoryLog[i])
                       );
    UT_ASSERT_TRUE (UnitTestStatus == UNIT_TEST_PASSED);
  }

  for ( ; i < ARRAY_SIZE (InternalMemoryLog); i++) {
    UnitTestStatus = InternalTestLoggerWriteV2 (
                       ((i) % 5) == 0 ? DEBUG_INFO : DEBUG_ERROR,
                       InternalMemoryLog[i],
                       AsciiStrLen (InternalMemoryLog[i])
                       );
    UT_ASSERT_TRUE (UnitTestStatus == UNIT_TEST_PASSED);
  }

  mLoggerProtocol.LoggerInfo = mLoggerInfo;
  Status                     = AdvancedLoggerAccessLibUnitTestInitialize (&mLoggerProtocol.AdvLoggerProtocol, ADV_LOG_MAX_SIZE);
  UT_ASSERT_NOT_EFI_ERROR (Status)

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
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BASIC_TEST_CONTEXT  *Btc;
  EFI_STATUS          Status;

  Btc = (BASIC_TEST_CONTEXT *)Context;

  Status = AdvancedLoggerAccessLibGetNextFormattedLine (&mMessageEntry);
  UT_ASSERT_STATUS_EQUAL (Status, Btc->ExpectedStatus);

  Status = AdvancedLoggerAccessLibUnitTestInitialize (NULL, 0);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = AdvancedLoggerAccessLibReset (&mMessageEntry);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

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
LineParserTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UNIT_TEST_FRAMEWORK_HANDLE  Fw;
  UNIT_TEST_SUITE_HANDLE      LineParserTests;
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
  Status = CreateUnitTestSuite (&LineParserTests, Fw, "Validate Line parser returns valid data", "LineParser.Test", NULL, NULL);
  if (EFI_ERROR (Status)) {
    AsciiPrint ("Failed in CreateUnitTestSuite for Line Parser Tests\n");
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  // Start with legacy message entry tests
  // -----------Suite------------Description-------Class---------Test Function-Pre---Clean-Context
  AddTestCase (LineParserTests, "Init", "SelfInit", InitializeInMemoryLog, NULL, NULL, NULL);
  AddTestCase (LineParserTests, "Basic check", "BasicCheck", BasicTests, NULL, CleanUpTestContext, &mTest00);
  AddTestCase (LineParserTests, "Line check  1", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest01);
  AddTestCase (LineParserTests, "Line check  2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest02);
  AddTestCase (LineParserTests, "Line check  3", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest03);
  AddTestCase (LineParserTests, "Line check  4", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest04);
  AddTestCase (LineParserTests, "Line check  5", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest05);
  AddTestCase (LineParserTests, "Line check  6", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest06);
  AddTestCase (LineParserTests, "Line check  7", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest07);
  AddTestCase (LineParserTests, "Line check  8", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest08);
  AddTestCase (LineParserTests, "Line check  9", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest09);
  AddTestCase (LineParserTests, "Line check 10", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest10);
  AddTestCase (LineParserTests, "Line check 11", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest11);
  AddTestCase (LineParserTests, "Line check 12", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest12);
  AddTestCase (LineParserTests, "Line check 13", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest13);
  AddTestCase (LineParserTests, "Line check 14", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest14);
  AddTestCase (LineParserTests, "Line check 15", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest15);
  AddTestCase (LineParserTests, "Line check 16", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest16);
  AddTestCase (LineParserTests, "Line check 17", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest17);
  AddTestCase (LineParserTests, "Line check 18", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest18);
  AddTestCase (LineParserTests, "Line check 19", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest19);
  AddTestCase (LineParserTests, "Check EOF", "SelfCheck", EOFTest, NULL, CleanUpTestContext, &mTest20);

  // Followed by V2 message entry tests
  AddTestCase (LineParserTests, "Init V2", "SelfInit", InitializeInMemoryLogV2, NULL, NULL, NULL);
  AddTestCase (LineParserTests, "Basic check V2", "BasicCheck", BasicTests, NULL, CleanUpTestContext, &mTest00V2);
  AddTestCase (LineParserTests, "Line check  1 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest01V2);
  AddTestCase (LineParserTests, "Line check  2 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest02V2);
  AddTestCase (LineParserTests, "Line check  3 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest03V2);
  AddTestCase (LineParserTests, "Line check  4 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest04V2);
  AddTestCase (LineParserTests, "Line check  5 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest05V2);
  AddTestCase (LineParserTests, "Line check  6 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest06V2);
  AddTestCase (LineParserTests, "Line check  7 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest07V2);
  AddTestCase (LineParserTests, "Line check  8 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest08V2);
  AddTestCase (LineParserTests, "Line check  9 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest09V2);
  AddTestCase (LineParserTests, "Line check 10 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest10V2);
  AddTestCase (LineParserTests, "Line check 11 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest11V2);
  AddTestCase (LineParserTests, "Line check 12 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest12V2);
  AddTestCase (LineParserTests, "Line check 13 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest13V2);
  AddTestCase (LineParserTests, "Line check 14 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest14V2);
  AddTestCase (LineParserTests, "Line check 15 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest15V2);
  AddTestCase (LineParserTests, "Line check 16 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest16V2);
  AddTestCase (LineParserTests, "Line check 17 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest17V2);
  AddTestCase (LineParserTests, "Line check 18 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest18V2);
  AddTestCase (LineParserTests, "Line check 19 V2", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest19V2);
  AddTestCase (LineParserTests, "Check EOF V2", "SelfCheck", EOFTest, NULL, CleanUpTestContext, &mTest20);

  // End with hybrid message entry tests
  AddTestCase (LineParserTests, "Init V2 Hybrid", "SelfInit", InitializeInMemoryLogV2Hybrid, NULL, NULL, NULL);
  AddTestCase (LineParserTests, "Basic check V2 Hybrid", "BasicCheck", BasicTests, NULL, CleanUpTestContext, &mTest00);
  AddTestCase (LineParserTests, "Line check  1 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest01);
  AddTestCase (LineParserTests, "Line check  2 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest02);
  AddTestCase (LineParserTests, "Line check  3 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest03);
  AddTestCase (LineParserTests, "Line check  4 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest04);
  AddTestCase (LineParserTests, "Line check  5 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest05);
  AddTestCase (LineParserTests, "Line check  6 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest06);
  AddTestCase (LineParserTests, "Line check  7 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest07);
  AddTestCase (LineParserTests, "Line check  8 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest08);
  AddTestCase (LineParserTests, "Line check  9 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest09);
  AddTestCase (LineParserTests, "Line check 10 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest10);
  AddTestCase (LineParserTests, "Line check 11 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest11V2);
  AddTestCase (LineParserTests, "Line check 12 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest12V2);
  AddTestCase (LineParserTests, "Line check 13 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest13V2);
  AddTestCase (LineParserTests, "Line check 14 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest14V2);
  AddTestCase (LineParserTests, "Line check 15 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest15V2);
  AddTestCase (LineParserTests, "Line check 16 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest16V2);
  AddTestCase (LineParserTests, "Line check 17 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest17V2);
  AddTestCase (LineParserTests, "Line check 18 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest18V2);
  AddTestCase (LineParserTests, "Line check 19 V2 Hybrid", "SelfCheck", BasicTests, NULL, CleanUpTestContext, &mTest19V2);
  AddTestCase (LineParserTests, "Check EOF V2 Hybrid", "SelfCheck", EOFTest, NULL, CleanUpTestContext, &mTest20);

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
