/** @file
    Advanced Logger Common function declaration


    Copyright (C) Microsoft Corporation. All rights reserved.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ADVANCED_LOGGER_COMMON_H__
#define __ADVANCED_LOGGER_COMMON_H__

/**
    Write data from buffer into the in memory logging buffer.


    Writes NumberOfBytes data bytes from Buffer to the debugging devices.

    @param  ErrorLevel       Error level of items top be printed
    @param  Buffer           Pointer to the data buffer to be written.
    @param  NumberOfBytes    Number of bytes to written to the log.

**/
VOID
EFIAPI
AdvancedLoggerWrite (
  IN       UINTN  ErrorLevel,
  IN CONST CHAR8  *Buffer,
  IN       UINTN  NumberOfBytes
  );

/**
    Get the Logger Information block

    Each instance of the AdvancedLogger Library must provide the following interface
    for use by AdVancedLoggerWrite ();

    @retval         Returns a pointer to the ADVANCED_LOGGER_INFO block.  Returns NULL
                    if it cannot be located.  This occurs prior to SEC completion.
 **/
ADVANCED_LOGGER_INFO *
EFIAPI
AdvancedLoggerGetLoggerInfo (
  VOID
  );

/**
  Helper function to return the log phase for each message.

  This function is intended to be used to distinguish between
  various types of modules.

  @return       Phase of current advanced logger instance.
**/
UINT16
EFIAPI
AdvancedLoggerGetPhase (
  VOID
  );

/**
  Write data from buffer into the in memory logging buffer.

  Writes NumberOfBytes data bytes from Buffer to the logging buffer.

  @param  DebugLevel       Debug level of the message
  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to be written to the Advanced Logger log.

  @retval LoggerInfo       Returns the logger info block. Returns NULL if it cannot
                           be located. This occurs prior to SEC completion.
**/
ADVANCED_LOGGER_INFO *
EFIAPI
AdvancedLoggerMemoryLoggerWrite (
  IN       UINTN  DebugLevel,
  IN CONST CHAR8  *Buffer,
  IN       UINTN  NumberOfBytes
  );

/**
  Returns whether the current Advanced Logger instance permits writing debug output to the
  hardware port.

  Each library instance that links AdvancedLoggerCommon.c provides this function. The default
  implementation (AdvancedLoggerHwPort.c) permits hardware port writes at boot time, and at OS
  runtime unless the platform sets PcdAdvancedLoggerHdwPortOsRuntimeDisable. The DXE runtime
  instance overrides the default implementation because it clears its logger info block at
  ExitBootServices.

  @param  LoggerInfo  The logger info block, or NULL if it is not available.

  @retval TRUE   Hardware port writes are permitted.
  @retval FALSE  Hardware port writes are currently suppressed.
**/
BOOLEAN
EFIAPI
AdvancedLoggerPrintToHwPort (
  IN ADVANCED_LOGGER_INFO  *LoggerInfo
  );

#endif // __ADVANCED_LOGGER_COMMON_H__
