/** @file AdvancedLogger.h

  Advanced Logger protocol definition for the DXE interface


  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ADVANCED_LOGGER_PROTOCOL_H__
#define __ADVANCED_LOGGER_PROTOCOL_H__

#define ADVANCED_LOGGER_PROTOCOL_SIGNATURE  SIGNATURE_32('L','O','G','P')

#define ADVANCED_LOGGER_PROTOCOL_VERSION  (2)

typedef struct _ADVANCED_LOGGER_PROTOCOL ADVANCED_LOGGER_PROTOCOL;

/**
  Function pointer for PPI routing to AdvancedLoggerWrite

  @param  AdvancedLoggerProtocol This pointer (pointer to Protocol)
  @param  ErrorLevel             The error level of the debug message.
  @param  Buffer                 The debug message to log.
  @param  NumberOfBytes          Number of bytes in the debug message.

**/
typedef
VOID
(EFIAPI *ADVANCED_LOGGER_WRITE_PROTOCOL)(
  IN        ADVANCED_LOGGER_PROTOCOL *AdvancedLoggerProtocol OPTIONAL,
  IN        UINTN                     ErrorLevel,
  IN  CONST CHAR8                    *Buffer,
  IN        UINTN                     NumberOfBytes
  );

struct _ADVANCED_LOGGER_PROTOCOL {
  UINT32                            Signature;
  UINT32                            Version;
  ADVANCED_LOGGER_WRITE_PROTOCOL    AdvancedLoggerWriteProtocol;
};

#endif // __ADVANCED_LOGGER_PROTOCOL_H__
