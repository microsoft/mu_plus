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
  Returns whether the given message should be written to the hardware port for this
  Advanced Logger instance.

  Each library instance provides this function so that instances with special hardware port
  considerations can override the default behavior. The implementation owns the full hardware
  port decision, including debug level filtering, so callers write to the hardware port
  whenever this returns TRUE.

  @param  LoggerInfo  The logger info block, or NULL if it is not available.
  @param  DebugLevel  The debug level of the message being logged.

  @retval TRUE   The message should be written to the hardware port.
  @retval FALSE  The message should not be written to the hardware port.
**/
BOOLEAN
EFIAPI
AdvancedLoggerPrintToHwPort (
  IN ADVANCED_LOGGER_INFO  *LoggerInfo,
  IN UINTN                 DebugLevel
  );

/**
  Returns whether the given message's debug level is enabled for the hardware port.

  This is the generic hardware port debug level policy shared by the Advanced Logger instances
  that honor the logger info block's dynamic level. Instances that must not consult the dynamic
  level (such as SEC instances) provide their own AdvancedLoggerPrintToHwPort instead of using
  this helper.

  @param  LoggerInfo  The logger info block, or NULL if it is not available.
  @param  DebugLevel  The debug level of the message being logged.

  @retval TRUE   The message's debug level is enabled for the hardware port.
  @retval FALSE  The message's debug level is not enabled for the hardware port.
**/
BOOLEAN
EFIAPI
AdvancedLoggerHwPortLevelEnabled (
  IN ADVANCED_LOGGER_INFO  *LoggerInfo,
  IN UINTN                 DebugLevel
  );

#endif // __ADVANCED_LOGGER_COMMON_H__
