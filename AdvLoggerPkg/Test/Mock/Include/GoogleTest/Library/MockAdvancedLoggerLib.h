/** @file
  Google Test mocks for AdvancedLoggerLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_ADVANCED_LOGGER_LIB_H_
#define MOCK_ADVANCED_LOGGER_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <PiPei.h>
  #include <PiDxe.h>
  #include <PiSmm.h>
  #include <PiMm.h>
  #include <Uefi.h>
  #include <Library/AdvancedLoggerLib.h>
}

struct MockAdvancedLoggerLib {
  MOCK_INTERFACE_DECLARATION (MockAdvancedLoggerLib);

  MOCK_FUNCTION_DECLARATION (
    VOID,
    AdvancedLoggerWrite,
    (IN       UINTN  ErrorLevel,
     IN CONST CHAR8  *Buffer,
     IN       UINTN  NumberOfBytes)
    );
};

#endif
