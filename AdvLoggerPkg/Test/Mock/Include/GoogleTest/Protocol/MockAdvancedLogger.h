/** @file MockAdvancedLogger.h
    This file declares a mock of the Advanced Logger Protocol.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_ADVANCED_LOGGER_PROTOCOL_H_
#define MOCK_ADVANCED_LOGGER_PROTOCOL_H_

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

//
// Mock function definitions in header file to prevent need of cpp file and
// make it easier to consume in tests.
// This is only suitable for protocol mocks.
//

MOCK_INTERFACE_DEFINITION (MockAdvancedLogger);
MOCK_FUNCTION_DEFINITION (MockAdvancedLogger, gAL_AdvancedLoggerWriteProtocol, 4, EFIAPI);

static ADVANCED_LOGGER_PROTOCOL  advancedLoggerInstance = {
  ADVANCED_LOGGER_PROTOCOL_SIGNATURE,   // UINT32
  ADVANCED_LOGGER_PROTOCOL_VERSION,     // UINT32
  gAL_AdvancedLoggerWriteProtocol       // ADVANCED_LOGGER_WRITE_PROTOCOL
};

extern "C" {
  ADVANCED_LOGGER_PROTOCOL  *gALProtocol = &advancedLoggerInstance;
}

#endif // MOCK_ADVANCED_LOGGER_PROTOCOL_H_
