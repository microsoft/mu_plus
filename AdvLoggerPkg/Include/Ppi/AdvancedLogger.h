/** @file AdvancedLogger.h

  Advanced Logger Ppi


  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ADVANCED_LOGGER_PPI_H__
#define __ADVANCED_LOGGER_PPI_H__

typedef struct _ADVANCED_LOGGER_PPI ADVANCED_LOGGER_PPI;

#define ADVANCED_LOGGER_PPI_SIGNATURE  SIGNATURE_32('L','O','P','I')

#define ADVANCED_LOGGER_PPI_VERSION  (2)

/**
  Function pointer for PPI routing to AdvancedLoggerWrite

  @param  ErrorLevel      The error level of the debug message.
  @param  Buffer          The debug message to log.
  @param  NumberOfBytes   Number of bytes in the debug message.

**/
typedef
VOID
(EFIAPI *ADVANCED_LOGGER_WRITE_PPI)(
  IN        UINTN                ErrorLevel,
  IN  CONST CHAR8               *Buffer,
  IN        UINTN                NumberOfBytes
  );

/**
  Function pointer for PPI routing to AdvancedLoggerWrite

  @param  ErrorLevel      The error level of the debug message.
  @param  Buffer          The debug message to log.
  @param  NumberOfBytes   Number of bytes in the debug message.

**/
typedef
VOID
(EFIAPI *ADVANCED_LOGGER_PRINT_PPI)(
  IN        UINTN                ErrorLevel,
  IN  CONST CHAR8               *Format,
  IN        VA_LIST              VaListMarker
  );

/**
  Function pointer for PPI routing to correct DebugAssert function

  @param  FileName        The pointer to the name of the source file that generated the assert condition.
  @param  LineNumber      The line number in the source file that generated the assert condition
  @param  Description     The pointer to the description of the assert condition.

**/
typedef
VOID
(EFIAPI *ADVANCED_LOGGER_ASSERT_PPI)(
  IN CONST CHAR8               *FileName,
  IN       UINTN                LineNumber,
  IN CONST CHAR8               *Description
  );

struct _ADVANCED_LOGGER_PPI {
  UINT32                        Signature;
  UINT32                        Version;
  ADVANCED_LOGGER_WRITE_PPI     AdvancedLoggerWritePpi;
  ADVANCED_LOGGER_PRINT_PPI     AdvancedLoggerPrintPpi;
  ADVANCED_LOGGER_ASSERT_PPI    AdvancedLoggerAssertPpi;
};

extern  EFI_GUID  gAdvancedLoggerPpiGuid;

#endif // __ADVANCED_LOGGER_PPI_H__
