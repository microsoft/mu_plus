/** @file AdvancedLogger.h

  Advanced Logger Ppi


  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ADVANCED_LOGGER_PPI_H__
#define __ADVANCED_LOGGER_PPI_H__

typedef struct _ADVANCED_LOGGER_PPI   ADVANCED_LOGGER_PPI;

/**
  Function pointer for PPI routing to AdvancedLoggerWrite

  @param  ErrorLevel      The error level of the debug message.
  @param  Buffer          The debug message to log.
  @param  NumberOfBytes   Number of bytes in the debug message.

**/
typedef
VOID
(EFIAPI *ADVANCED_LOGGER_WRITE)(
    IN  UINTN           ErrorLevel,
    IN  CONST CHAR8    *Buffer,
    IN  UINTN           NumberOfBytes
  );

/**
  Function pointer for PPI routing to correct DebugPrint function

  @param  ErrorLevel  The error level of the debug message.
  @param  Format      Format string for the debug message to print.
  @param  VaListMarker  VA_LIST marker for the variable argument list.

**/
typedef
VOID
(EFIAPI *ADVANCED_LOGGER_PRINT)(
    IN  UINTN        ErrorLevel,
    IN  CONST CHAR8 *Format,
    VA_LIST          VaListMarker
  );

/**
  Function pointer for PPI routing to correct DebugAssert function

  @param  FileName     The pointer to the name of the source file that generated the assert condition.
  @param  LineNumber   The line number in the source file that generated the assert condition
  @param  Description  The pointer to the description of the assert condition.

**/
typedef
VOID
(EFIAPI *ADVANCED_LOGGER_ASSERT)(
    IN CONST CHAR8 *FileName,
    IN UINTN        LineNumber,
    IN CONST CHAR8 *Description
  );

/**
  Function pointer for PPI routing to correct DebugDumpMemory function

  @param  Address      The address of the memory to dump.
  @param  Length       The length of the region to dump.
  @param  Flags        PrintAddress, PrintOffset etc

**/
typedef
VOID
(EFIAPI *ADVANCED_LOGGER_DUMP_MEMORY)(
    IN UINTN        ErrorLevel,
    IN CONST VOID  *Address,
    IN UINTN        Length,
    IN UINT32       Flags
  );


struct _ADVANCED_LOGGER_PPI {
    ADVANCED_LOGGER_WRITE         AdvancedLoggerWrite;
    ADVANCED_LOGGER_PRINT         AdvancedLoggerPrint;
    ADVANCED_LOGGER_ASSERT        AdvancedLoggerAssert;
    ADVANCED_LOGGER_DUMP_MEMORY   AdvancedLoggerDumpMemory;
};

extern  EFI_GUID  gAdvancedLoggerPpiGuid;

#endif  // __ADVANCED_LOGGER_PPI_H__
