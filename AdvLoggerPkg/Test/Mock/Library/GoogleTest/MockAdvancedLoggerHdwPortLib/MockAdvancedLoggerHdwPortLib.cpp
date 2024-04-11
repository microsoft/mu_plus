/** @file
  Google Test mocks for AdvancedLoggerHdwPortLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockAdvancedLoggerHdwPortLib.h>

//
// Global Variables that are not const
//

MOCK_INTERFACE_DEFINITION (MockAdvancedLoggerHdwPortLib);

MOCK_FUNCTION_DEFINITION (MockAdvancedLoggerHdwPortLib, AdvancedLoggerHdwPortInitialize, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAdvancedLoggerHdwPortLib, AdvancedLoggerHdwPortWrite, 3, EFIAPI);
