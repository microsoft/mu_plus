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

struct _ADVANCED_LOGGER_PPI {
    ADVANCED_LOGGER_WRITE    AdvancedLoggerWrite;
};

extern  EFI_GUID  gAdvancedLoggerPpiGuid;

#endif  // __ADVANCED_LOGGER_PPI_H__
