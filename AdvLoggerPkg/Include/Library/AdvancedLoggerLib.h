/** @file AdvancedLoggerLib.h

  Advanced Logger Library interface for DebugLib


  Copyright (C) Microsoft Corporation. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ADVANCED_LOGGER_LIB_H__
#define __ADVANCED_LOGGER_LIB_H__


/**
  Write data from the DebugLib buffer to the in memory logging buffer,
  and route it to the Serial Port lib.

  Writes NumberOfBytes data bytes from Buffer to the logging buffer.

  @param  ErrorLevel       Error level passed into DebugLib
  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the serial device.


**/
VOID
EFIAPI
AdvancedLoggerWrite (
    IN       UINTN    ErrorLevel,
    IN CONST CHAR8   *Buffer,
    IN       UINTN    NumberOfBytes
  );


#endif  // __ADVANCED_LOGGER_LIB_H__
