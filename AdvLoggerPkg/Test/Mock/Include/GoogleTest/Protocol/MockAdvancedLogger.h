/** @file    MockAdvancedLogger.h
    This file declares a mock of the Advanced Logger Protocol.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_ADVANCED_LOGGER_LIB_H_
#define MOCK_ADVANCED_LOGGER_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/AdvancedLogger.h>
}

struct MockAdvancedLogger {
  MOCK_INTERFACE_DECLARATION (MockAdvancedLogger);

  MOCK_FUNCTION_DECLARATION (
    VOID,
    gAL_AdvancedLoggerWriteProtocol,
    (IN        ADVANCED_LOGGER_PROTOCOL *ThisAdvancedLoggerProtocol OPTIONAL,
     IN        UINTN                     ErrorLevel,
     IN  CONST CHAR8                    *Buffer,
     IN        UINTN                     NumberOfBytes)
    );
};

extern "C" {
  extern ADVANCED_LOGGER_PROTOCOL  *gALProtocol;
}

#endif // MOCK_ADVANCED_LOGGER_LIB_H_
