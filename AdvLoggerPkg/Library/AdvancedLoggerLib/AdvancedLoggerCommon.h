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
  Helper function to return the string prefix for each message.
  This function is intended to be used to distinguish between
  various types of modules.

  @param[out]   MessagePrefixSize  The size of the prefix string in bytes,
                excluding NULL terminator.

  @return       Pointer to the prefix string. NULL if no prefix is available.
**/
CONST CHAR8 *
EFIAPI
AdvancedLoggerGetStringPrefix (
  IN UINTN  *MessagePrefixSize
  );

#endif // __ADVANCED_LOGGER_COMMON_H__
