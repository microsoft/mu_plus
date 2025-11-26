/** @file
    Advanced Logger Common function declaration


    Copyright (C) Microsoft Corporation. All rights reserved.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ADVANCED_LOGGER_COMMON_H__
#define __ADVANCED_LOGGER_COMMON_H__

//
// The maximum depth to follow when traversing chains of advanced logger info structures
//
#define ADVANCED_LOGGER_MAX_LOGGER_CHAIN_DEPTH  3

/**
  Follow the logger info redirection chain to find the current logger info.

  This is primarily used to support migrating an advanced logger buffer to a new location.

  @param[in]  LoggerInfo    Pointer to a logger info structure to start the chain walk.

  @return               Pointer to the current logger info structure (with NewLoggerInfoAddress == 0).
                        Returns NULL if the input is NULL or if the chain is invalid.
**/
ADVANCED_LOGGER_INFO *
AdvancedLoggerGetCurrentLoggerInfo (
  IN ADVANCED_LOGGER_INFO  *LoggerInfo
  );

/**
  Follow the logger info redirection chain and update the provided logger info pointer.

  This function follows the logger address chain to find the current logger info
  and updates the input pointer if a new logger is found.

  The function does not attempt to make any validation statement about the logger info
  structures and it is expected NULL may be provided as input.

  @param[in,out]  LoggerInfo    Pointer to a logger info pointer to update.
  @param[out]     MaxAddress    Optional pointer to update with the max address of the current logger.
  @param[out]     BufferSize    Optional pointer to update with the buffer size of the current logger.

  @retval         TRUE          A new logger was found and LoggerInfo was updated to the new address.
  @retval         FALSE         A new logger was not found and no modification was made to LoggerInfo.
**/
BOOLEAN
AdvancedLoggerCheckForNewerLogger (
  IN OUT ADVANCED_LOGGER_INFO  **LoggerInfo,
  OUT    EFI_PHYSICAL_ADDRESS  *MaxAddress  OPTIONAL,
  OUT    UINT32                *BufferSize  OPTIONAL
  );

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

#endif // __ADVANCED_LOGGER_COMMON_H__
