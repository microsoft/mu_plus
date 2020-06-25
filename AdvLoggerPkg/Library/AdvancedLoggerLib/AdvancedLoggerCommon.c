/** @file
  Advanced Logger Common functions


  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <AdvancedLoggerInternal.h>

#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/TimerLib.h>

#include "../AdvancedLoggerCommon.h"

/**
  Write data from buffer into the in memory logging buffer.

  Writes NumberOfBytes data bytes from Buffer to the logging buffer.

  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the Advanced Logger log.

  @retval LoggerInfo       Returns the logger info block
**/
STATIC
VOID
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
        return;
    }

    if (NumberOfBytes > MAX_UINT16) {
        return;
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
                    OldSize = InterlockedCompareExchange32 ( (UINT32 *) &LoggerInfo->DiscardedSize,
                                                             (UINT32) CurrentSize,
                                                             (UINT32) NewSize);

                } while (OldSize != CurrentSize);

                return;
            }

            NewBuffer = PA_FROM_PTR((CHAR8_FROM_PA(CurrentBuffer) + EntrySize));
            OldValue = InterlockedCompareExchange64 ( (UINT64 *) &LoggerInfo->LogCurrent,
                                                      (UINT64) CurrentBuffer,
                                                      (UINT64) NewBuffer);
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
}


/**
  Write data from buffer to possible debugging devices.

  This is the interface from PeiCore
  This is also called by the Ppi

  Writes NumberOfBytes data bytes from Buffer to the debugging devices.

  @param  DebugLevel       Error level of items top be printed
  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the Advanced Logger log.


**/
VOID
EFIAPI
AdvancedLoggerWrite (
    IN       UINTN    DebugLevel,
    IN CONST CHAR8   *Buffer,
    IN       UINTN    NumberOfBytes
  ) {

    // All messages go to the in memory log.

    AdvancedLoggerMemoryLoggerWrite (DebugLevel, Buffer, NumberOfBytes);

}
