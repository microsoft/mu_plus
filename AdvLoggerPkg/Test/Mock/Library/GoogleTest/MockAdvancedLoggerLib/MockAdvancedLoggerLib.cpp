/** @file
  Google Test mocks for AdvancedLoggerLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockAdvancedLoggerLib.h>

//
// Global Variables that are not const
//

MOCK_INTERFACE_DEFINITION (MockAdvancedLoggerLib);

MOCK_FUNCTION_DEFINITION (MockAdvancedLoggerLib, AdvancedLoggerWrite, 3, EFIAPI);
