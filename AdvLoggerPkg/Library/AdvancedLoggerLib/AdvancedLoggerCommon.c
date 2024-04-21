/** @file
  Advanced Logger Common functions


  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>

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

  @retval LoggerInfo       Returns the logger info block. Returns NULL if it cannot
                           be located. This occurs prior to SEC completion.
**/
ADVANCED_LOGGER_INFO *
EFIAPI
AdvancedLoggerMemoryLoggerWrite (
  IN       UINTN  DebugLevel,
  IN CONST CHAR8  *Buffer,
  IN       UINTN  NumberOfBytes
  )
{
  ADVANCED_LOGGER_INFO              *LoggerInfo;
  UINT32                            CurrentBuffer;
  UINT32                            NewBuffer;
  UINT32                            OldValue;
  UINT32                            OldSize;
  UINT32                            NewSize;
  UINT32                            CurrentSize;
  UINTN                             EntrySize;
  UINTN                             UsedSize;
  ADVANCED_LOGGER_MESSAGE_ENTRY_V2  *Entry;

  if ((NumberOfBytes == 0) || (Buffer == NULL)) {
    return NULL;
  }

  if (NumberOfBytes > MAX_UINT16) {
    return NULL;
  }

  LoggerInfo = AdvancedLoggerGetLoggerInfo ();

  if (LoggerInfo != NULL) {
    EntrySize = MESSAGE_ENTRY_SIZE_V2 (OFFSET_OF (ADVANCED_LOGGER_MESSAGE_ENTRY_V2, MessageText), NumberOfBytes);
    do {
      CurrentBuffer = LoggerInfo->LogCurrentOffset;
      UsedSize      = USED_LOG_SIZE (LoggerInfo);
      if ((UsedSize >= LoggerInfo->LogBufferSize) ||
          ((LoggerInfo->LogBufferSize - UsedSize) < EntrySize))
      {
        if (FeaturePcdGet (PcdAdvancedLoggerAutoWrapEnable) && (LoggerInfo->AtRuntime)) {
          //
          // Wrap around the current cursor when auto wrap is enabled on buffer full during runtime.
          //
          NewBuffer = LoggerInfo->LogBufferOffset;
          OldValue  = InterlockedCompareExchange32 (
                        &LoggerInfo->LogCurrentOffset,
                        CurrentBuffer,
                        NewBuffer
                        );
          if (OldValue != CurrentBuffer) {
            //
            // Another thread has updated the buffer, we should retry the logging.
            //
            continue;
          }

          // Now that we have a buffer that starts from the beginning, proceed to log the current message, from the beginning.
          // Note that in this case, if there are other threads in the middle of logging a message, they will continue to write
          // to the end of the buffer as it fits.
          // If there is another clearing attempt on the other thread, i.e. another thread also try to fill up the buffer, the
          // first clear will take effect and the other log entries will fail to update and proceed with a normal retry.
        } else {
          //
          // Update the number of bytes of log that have not been captured
          //
          do {
            CurrentSize = LoggerInfo->DiscardedSize;
            NewSize     = CurrentSize + (UINT32)NumberOfBytes;
            OldSize     = InterlockedCompareExchange32 (
                            (UINT32 *)&LoggerInfo->DiscardedSize,
                            (UINT32)CurrentSize,
                            (UINT32)NewSize
                            );
          } while (OldSize != CurrentSize);

          return LoggerInfo;
        }
      }

      // EntrySize is contained within a UINT32, this is safe to do
      NewBuffer = (UINT32)(CurrentBuffer + EntrySize);
      OldValue  = InterlockedCompareExchange32 (
                    &LoggerInfo->LogCurrentOffset,
                    CurrentBuffer,
                    NewBuffer
                    );
    } while (OldValue != CurrentBuffer);

    Entry               = (ADVANCED_LOGGER_MESSAGE_ENTRY_V2 *)((UINT8 *)LoggerInfo + CurrentBuffer);
    Entry->MajorVersion = ADVANCED_LOGGER_MSG_MAJ_VER;
    Entry->MinorVersion = ADVANCED_LOGGER_MSG_MIN_VER;
    Entry->TimeStamp    = GetPerformanceCounter ();    // AdvancedLoggerGetTimeStamp();
    Entry->Phase        = AdvancedLoggerGetPhase ();

    // DebugLevel is defined as a UINTN, so it is 32 bits in PEI and 64 bits in DXE.
    // However, the DEBUG_* values and the PcdFixedDebugPrintErrorLevel are only 32 bits.
    Entry->DebugLevel    = (UINT32)DebugLevel;
    Entry->MessageOffset = OFFSET_OF (ADVANCED_LOGGER_MESSAGE_ENTRY_V2, MessageText);
    Entry->MessageLen    = (UINT16)NumberOfBytes;
    CopyMem (Entry->MessageText, Buffer, NumberOfBytes);
    Entry->Signature = MESSAGE_ENTRY_SIGNATURE_V2;
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

  @retval LoggerInfo       Returns the logger info block. Returns NULL if it cannot
                           be located. This occurs prior to SEC completion.

**/
VOID
EFIAPI
AdvancedLoggerWrite (
  IN       UINTN  DebugLevel,
  IN CONST CHAR8  *Buffer,
  IN       UINTN  NumberOfBytes
  )
{
  ADVANCED_LOGGER_INFO  *LoggerInfo;

  // All messages go to the in memory log.
  LoggerInfo = AdvancedLoggerMemoryLoggerWrite (DebugLevel, Buffer, NumberOfBytes);

  // Only selected messages go to the hdw port.

 #ifdef ADVANCED_LOGGER_SEC
  // If LoggerInfo == NULL, assume there is a HdwPort and it has not been disabled. This
  // does occur in SEC
  if ((LoggerInfo == NULL) || (!LoggerInfo->HdwPortDisabled)) {
    if (DebugLevel & PcdGet32 (PcdAdvancedLoggerHdwPortDebugPrintErrorLevel)) {
      AdvancedLoggerHdwPortWrite (DebugLevel, (UINT8 *)Buffer, NumberOfBytes);
    }
  }

 #else
  if ((LoggerInfo != NULL) && (!LoggerInfo->HdwPortDisabled)) {
    // if we are at a high enough version to support HW_LVL logging, only call the HdwPortWrite if this DebugLevel
    // is asked to be logged
    // if we are at an older version, check the PCD to see if we should log this message
    if (LoggerInfo->Version >= ADVANCED_LOGGER_HW_LVL_VER) {
      if (DebugLevel & LoggerInfo->HwPrintLevel) {
        AdvancedLoggerHdwPortWrite (DebugLevel, (UINT8 *)Buffer, NumberOfBytes);
      }
    } else if (DebugLevel & PcdGet32 (PcdAdvancedLoggerHdwPortDebugPrintErrorLevel)) {
      AdvancedLoggerHdwPortWrite (DebugLevel, (UINT8 *)Buffer, NumberOfBytes);
    }
  }

 #endif
}
