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

#ifdef ADVANCED_LOGGER_RUNTIME
//
// Defined by the DXE runtime AdvancedLoggerLib instance; TRUE after ExitBootServices.
//
extern BOOLEAN  gAdvancedLoggerAtRuntime;
#endif

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
  UINT32                HwPortDebugLevel;
  BOOLEAN               HwPortWriteAllowed;
  BOOLEAN               AtRuntime;

  // All messages go to the in memory log.
  LoggerInfo = AdvancedLoggerMemoryLoggerWrite (DebugLevel, Buffer, NumberOfBytes);

  HwPortDebugLevel = PcdGet32 (PcdAdvancedLoggerHdwPortDebugPrintErrorLevel);

  // Only selected messages go to the hdw port.

  if ((LoggerInfo == NULL) || (!LoggerInfo->HdwPortDisabled)) {
 #ifndef ADVANCED_LOGGER_SEC
    if ((LoggerInfo != NULL) && (LoggerInfo->Version >= ADVANCED_LOGGER_INFO_HW_LVL_SUPPORTED_VER)) {
      HwPortDebugLevel = LoggerInfo->HwPrintLevel;
    }

 #endif

    //
    // By default hardware port writes are always allowed, preserving the historical
    // behavior of writing debug output to the serial port at both boot time and OS runtime.
    //
    // The underlying hardware serial port implementation on some platforms cannot be safely
    // called at OS runtime (after ExitBootServices). Such a platform sets the FeatureFlag PCD
    // PcdAdvancedLoggerHdwPortRuntimeDisable to TRUE to restrict hardware port writes to boot
    // time only. The runtime detection below is therefore only needed for those opt-in
    // platforms; for everyone else the PCD is a compile-time FALSE and this whole block is
    // optimized away, leaving behavior and code size unchanged.
    //
    HwPortWriteAllowed = TRUE;
    if (FeaturePcdGet (PcdAdvancedLoggerHdwPortRuntimeDisable)) {
      //
      // Determine whether we are at OS runtime using whichever signal is available, so that
      // hardware port writes are suppressed only at runtime and early-boot serial output is
      // preserved.
      //
      // When the logger info block is available, its AtRuntime field is authoritative.
      // A NULL block usually indicates very early boot (before the block is locatable);
      // the exception is the DXE runtime instance, which also returns NULL at runtime
      // because it clears its logger info pointer at ExitBootServices.
      //
      if (LoggerInfo != NULL) {
        AtRuntime = LoggerInfo->AtRuntime;
      } else {
 #ifdef ADVANCED_LOGGER_RUNTIME
        //
        // The DXE runtime instance clears its logger info pointer at ExitBootServices, so a
        // NULL block is ambiguous between early boot and runtime; consult the runtime flag.
        //
        AtRuntime = gAdvancedLoggerAtRuntime;
 #else
        //
        // For all other instances a NULL block indicates very early boot.
        //
        AtRuntime = FALSE;
 #endif
      }

      HwPortWriteAllowed = (BOOLEAN)(!AtRuntime);
    }

    if ((DebugLevel & HwPortDebugLevel) && HwPortWriteAllowed) {
      AdvancedLoggerHdwPortWrite (DebugLevel, (UINT8 *)Buffer, NumberOfBytes);
    }
  }
}
