/** @file AdvancedLogger.h

  Advanced Logger protocol definition for the DXE interface


  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ADVANCED_LOGGER_PROTOCOL_H__
#define __ADVANCED_LOGGER_PROTOCOL_H__

#define ADVANCED_LOGGER_PROTOCOL_SIGNATURE     SIGNATURE_32('L','O','G','P')

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

typedef struct {
    UINT32                 Signature;
    UINT32                 Reserved;
    ADVANCED_LOGGER_WRITE  AdvancedLoggerWrite;
    VOID                  *Context;                  // Context is used to pass the LoggerInfo
                                                     // block to the file logger.
} ADVANCED_LOGGER_PROTOCOL;


#endif  // __ADVANCED_LOGGER_PROTOCOL_H__
