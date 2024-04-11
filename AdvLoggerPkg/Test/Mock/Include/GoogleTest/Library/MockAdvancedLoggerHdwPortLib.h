/** @file
  Google Test mocks for AdvancedLoggerHdwPortLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_ADVANCED_LOGGER_HDW_PORT_LIB_H_
#define MOCK_ADVANCED_LOGGER_HDW_PORT_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <PiPei.h>
  #include <PiDxe.h>
  #include <PiSmm.h>
  #include <PiMm.h>
  #include <Uefi.h>
  #include <Library/AdvancedLoggerHdwPortLib.h>
}

struct MockAdvancedLoggerHdwPortLib {
  MOCK_INTERFACE_DECLARATION (MockAdvancedLoggerHdwPortLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    AdvancedLoggerHdwPortInitialize,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    AdvancedLoggerHdwPortWrite,
    (IN UINTN  DebugLevel,
     IN UINT8  *Buffer,
     IN UINTN  NumberOfBytes)
    );
};

#endif
