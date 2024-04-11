/** @file
  Google Test mocks for AdvancedLoggerAccessLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_ADVANCED_LOGGER_ACCESS_LIB_H_
#define MOCK_ADVANCED_LOGGER_ACCESS_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <PiPei.h>
  #include <PiDxe.h>
  #include <PiSmm.h>
  #include <PiMm.h>
  #include <Uefi.h>
  #include <Library/AdvancedLoggerAccessLib.h>
}

struct MockAdvancedLoggerAccessLib {
  MOCK_INTERFACE_DECLARATION (MockAdvancedLoggerAccessLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    AdvancedLoggerAccessLibGetNextMessageBlock,
    (IN ADVANCED_LOGGER_ACCESS_MESSAGE_BLOCK_ENTRY  *BlockEntry)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    AdvancedLoggerAccessLibGetNextFormattedLine,
    (IN ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY  *LineEntry)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    AdvancedLoggerAccessLibReset,
    (IN ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY  *AccessEntry)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    AdvancedLoggerAccessLibUnitTestInitialize,
    (IN ADVANCED_LOGGER_PROTOCOL  *TestProtocol OPTIONAL,
     IN UINT16                    MaxMessageSize)
    );
};

#endif
