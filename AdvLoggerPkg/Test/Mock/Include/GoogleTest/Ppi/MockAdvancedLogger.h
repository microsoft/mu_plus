/** @file MockAdvancedLogger.h
    This file declares a mock of the Advanced Logger Ppi.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_ADVANCED_LOGGER_PPI_H_
#define MOCK_ADVANCED_LOGGER_PPI_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Ppi/AdvancedLogger.h>
}

struct MockAdvancedLoggerPpi {
  MOCK_INTERFACE_DECLARATION (MockAdvancedLoggerPpi);

  MOCK_FUNCTION_DECLARATION (
    VOID,
    gAL_AdvancedLoggerWritePpi,
    (IN        UINTN    ErrorLevel,
     IN  CONST CHAR8    *Buffer,
     IN        UINTN    NumberOfBytes)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    gAL_AdvancedLoggerPrintPpi,
    (IN        UINTN    ErrorLevel,
     IN  CONST CHAR8    *Format,
     IN        VA_LIST  VaListMarker)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    gAL_AdvancedLoggerAssertPpi,
    (IN CONST CHAR8    *FileName,
     IN       UINTN    LineNumber,
     IN CONST CHAR8    *Description)
    );
};

//
// Mock function definitions in header file to prevent need of cpp file and
// make it easier to consume in tests.
// This is only suitable for protocol mocks.
//
MOCK_INTERFACE_DEFINITION (MockAdvancedLoggerPpi);
MOCK_FUNCTION_DEFINITION (MockAdvancedLoggerPpi, gAL_AdvancedLoggerWritePpi, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAdvancedLoggerPpi, gAL_AdvancedLoggerPrintPpi, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockAdvancedLoggerPpi, gAL_AdvancedLoggerAssertPpi, 3, EFIAPI);

ADVANCED_LOGGER_PPI  advancedLoggerPpiInstance = {
  ADVANCED_LOGGER_PPI_SIGNATURE, // UINT32
  ADVANCED_LOGGER_PPI_VERSION,   // UINT32
  gAL_AdvancedLoggerWritePpi,    // ADVANCED_LOGGER_WRITE_PPI
  gAL_AdvancedLoggerPrintPpi,    // ADVANCED_LOGGER_PRINT_PPI
  gAL_AdvancedLoggerAssertPpi    // ADVANCED_LOGGER_ASSERT_PPI
};

extern "C" {
  ADVANCED_LOGGER_PPI  *gALPpi = &advancedLoggerPpiInstance;
}

#endif // MOCK_ADVANCED_LOGGER_PPI_H_
