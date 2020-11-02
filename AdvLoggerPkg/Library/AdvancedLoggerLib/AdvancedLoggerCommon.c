/** @file
  Advanced Logger Common functions


  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <AdvancedLoggerInternal.h>

#include <Library/AdvancedLoggerHdwPortLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/TimerLib.h>

#include "../AdvancedLoggerCommon.h"

/**
  Write data from buffer into the in memory logging buffer.

  Writes NumberOfBytes data bytes from Buffer to the logging buffer.

  @param  DebugLevel       Debug level of the message
  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to be written to the Advanced Logger log.

  @retval LoggerInfo       Returns the logger info block. Returns NULL
                           if it cannot be located. This occurs prior to SEC completion.
**/
STATIC
ADVANCED_LOGGER_INFO *
AdvancedLoggerMemoryLoggerWrite (
    IN       UINTN    DebugLevel,
    IN CONST CHAR8   *Buffer,
    IN       UINTN    NumberOfBytes
  ) {

    ADVANCED_LOGGER_INFO           *LoggerInfo;
    EFI_PHYSICAL_ADDRESS            CurrentBuffer;
    EFI_PHYSICAL_ADDRESS            NewBuffer;
    EFI_PHYSICAL_ADDRESS            OldValue;
    UINT32                          OldSize;
    UINT32                          NewSize;
    UINT32                          CurrentSize;
    UINTN                           EntrySize;
    ADVANCED_LOGGER_MESSAGE_ENTRY  *Entry;

    if ((NumberOfBytes == 0) || (Buffer == NULL)) {
        return NULL;
    }

    if (NumberOfBytes > MAX_UINT16) {
        return NULL;
    }

    LoggerInfo = AdvancedLoggerGetLoggerInfo ();

    EntrySize = MESSAGE_ENTRY_SIZE(NumberOfBytes);

    if (LoggerInfo != NULL) {
        do {
            CurrentBuffer = LoggerInfo->LogCurrent;
            if ((LoggerInfo->LogBufferSize -
                ((UINTN)(CurrentBuffer - LoggerInfo->LogBuffer))) < EntrySize) {

                //
                // Update the number of bytes of log that have not been captured
                //
                do {
                    CurrentSize = LoggerInfo->DiscardedSize;
                    NewSize = LoggerInfo->DiscardedSize + (UINT32) NumberOfBytes;
                    OldSize = InterlockedCompareExchange32 (
                                (UINT32 *) &LoggerInfo->DiscardedSize,
                                (UINT32) CurrentSize,
                                (UINT32) NewSize
                                );

                } while (OldSize != CurrentSize);

                return LoggerInfo;
            }

            NewBuffer = PA_FROM_PTR((CHAR8_FROM_PA(CurrentBuffer) + EntrySize));
            OldValue = InterlockedCompareExchange64 (
                         (UINT64 *) &LoggerInfo->LogCurrent,
                         (UINT64) CurrentBuffer,
                         (UINT64) NewBuffer
                         );

        } while (OldValue != CurrentBuffer);

        Entry = (ADVANCED_LOGGER_MESSAGE_ENTRY *) PTR_FROM_PA(CurrentBuffer);
        Entry->TimeStamp = GetPerformanceCounter(); //AdvancedLoggerGetTimeStamp();

        // DebugLevel is defined as a UINTN, so it is 32 bits in PEI and 64 bits in DXE.
        // However, the DEBUG_* values and the PcdFixedDebugPrintErrorLevel are only 32 bits.
        Entry->DebugLevel = (UINT32) DebugLevel;
        Entry->MessageLen = (UINT16) NumberOfBytes;
        CopyMem(Entry->MessageText, Buffer, NumberOfBytes);
        Entry->Signature = MESSAGE_ENTRY_SIGNATURE;
    }

    return LoggerInfo;
}


/**
  Write data from buffer to possible debugging devices.

  This is the interface from PeiCore
  This is also called by the Ppi

  Writes NumberOfBytes data bytes from Buffer to the debugging devices.

  @param  DebugLevel       Error level of items top be printed
  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to be written to the Advanced Logger log.


**/
VOID
EFIAPI
AdvancedLoggerWrite (
    IN       UINTN    DebugLevel,
    IN CONST CHAR8   *Buffer,
    IN       UINTN    NumberOfBytes
  ) {
    ADVANCED_LOGGER_INFO    *LoggerInfo;


    // All messages go to the in memory log.

    LoggerInfo = AdvancedLoggerMemoryLoggerWrite (DebugLevel, Buffer, NumberOfBytes);

    // Only selected messages go to the hdw port.

    // If LoggerInfo == NULL, assume there is a HdwPort and it has not been disabled. This
    // can occur in SEC and other points of darkness for logging debug messages

    if ((LoggerInfo == NULL) || (!LoggerInfo->HdwPortDisabled)) {
        AdvancedLoggerHdwPortWrite(DebugLevel, (UINT8 *) Buffer, NumberOfBytes);
    }
}
