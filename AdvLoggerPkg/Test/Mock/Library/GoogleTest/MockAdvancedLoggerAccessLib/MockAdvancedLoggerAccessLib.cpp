/** @file
  Google Test mocks for AdvancedLoggerAccessLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockAdvancedLoggerAccessLib.h>

//
// Global Variables that are not const
//

MOCK_INTERFACE_DEFINITION (MockAdvancedLoggerAccessLib);

MOCK_FUNCTION_DEFINITION (MockAdvancedLoggerAccessLib, AdvancedLoggerAccessLibGetNextMessageBlock, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAdvancedLoggerAccessLib, AdvancedLoggerAccessLibGetNextFormattedLine, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAdvancedLoggerAccessLib, AdvancedLoggerAccessLibReset, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAdvancedLoggerAccessLib, AdvancedLoggerAccessLibUnitTestInitialize, 2, EFIAPI);
