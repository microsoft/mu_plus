/** @file MockAdvancedLogger.cpp
    Google Test mock for Advanced Logger Protocol.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockAdvancedLogger.h>

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
