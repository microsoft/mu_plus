/** @file
  PEI Library Instance of the Advanced Logger library.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>

#include <AdvancedLoggerInternal.h>

#include <Ppi/AdvancedLogger.h>

#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>

/**
  Advanced Logger Write

  @param  ErrorLevel      The error level of the debug message.
  @param  Buffer          The debug message to log.
  @param  NumberOfBytes   Number of bytes in the debug message.

**/
VOID
EFIAPI
AdvancedLoggerWrite (
  IN        UINTN  ErrorLevel,
  IN  CONST CHAR8  *Buffer,
  IN        UINTN  NumberOfBytes
  )
{
  ADVANCED_LOGGER_PPI  *AdvancedLoggerPpi;
  EFI_STATUS           Status;

  Status = PeiServicesLocatePpi (
             &gAdvancedLoggerPpiGuid,
             0,
             NULL,
             (VOID **)&AdvancedLoggerPpi
             );

  if (Status == EFI_SUCCESS) {
    if ((AdvancedLoggerPpi->Signature != ADVANCED_LOGGER_PPI_SIGNATURE) ||
        (AdvancedLoggerPpi->Version != ADVANCED_LOGGER_PPI_VERSION))
    {
      ASSERT (AdvancedLoggerPpi->Signature == ADVANCED_LOGGER_PPI_SIGNATURE);
      ASSERT (AdvancedLoggerPpi->Version == ADVANCED_LOGGER_PPI_VERSION);

      // Signature/Version mismatch, don't attempt to use the PPI.
      return;
    }

    AdvancedLoggerPpi->AdvancedLoggerWritePpi (ErrorLevel, Buffer, NumberOfBytes);
  }
}
