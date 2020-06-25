/** @file
    Advanced Logger Common function declaration


    Copyright (C) Microsoft Corporation. All rights reserved.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ADVANCED_LOGGER_COMMON_H__
#define __ADVANCED_LOGGER_COMMON_H__

/**
    Write data from buffer to possible debugging devices.

    This is the interface from PeiCore
    This is also called by the Ppi

    Writes NumberOfBytes data bytes from Buffer to the debugging devices.

    @param  ErrorLevel       Error level of items top be printed
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

/**
    Get the Logger Information block

    Each instance of the AdvancedLogger Library must provide the following interface
    for use by AdVancedLoggerWrite ();
 **/
ADVANCED_LOGGER_INFO *
EFIAPI
AdvancedLoggerGetLoggerInfo (
    VOID
);

#endif  // __ADVANCED_LOGGER_COMMON_H__
