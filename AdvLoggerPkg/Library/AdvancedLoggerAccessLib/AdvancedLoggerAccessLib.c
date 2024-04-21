/** @file
  Implementation of Advanced Logger Access Library.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <AdvancedLoggerInternal.h>

#include <Protocol/AdvancedLogger.h>
#include <AdvancedLoggerInternalProtocol.h>

#include <Library/AdvancedLoggerAccessLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC  ADVANCED_LOGGER_INFO  *mLoggerInfo                                  = NULL;
STATIC  EFI_PHYSICAL_ADDRESS  mLowAddress                                   = 0;
STATIC  EFI_PHYSICAL_ADDRESS  mHighAddress                                  = 0;
STATIC  UINT16                mMaxMessageSize                               = ADVANCED_LOGGER_MAX_MESSAGE_SIZE;
CONST   CHAR8                 *AdvMsgEntryPrefix[ADVANCED_LOGGER_PHASE_CNT] = {
  "[UNSPECIFIED] ",
  "[SEC] ",
  "[PEI] ",
  "[PEI64] ",
  "[DXE] ",
  "[RUNTIME] ",
  "[MM_CORE] ",
  "[MM] ",
  "[SMM_CORE] ",
  "[SMM] ",
  "[TFA] ",
};

#define ADV_TIME_STAMP_FORMAT  "%2.2d:%2.2d:%2.2d.%3.3d : "
#define ADV_TIME_STAMP_RESULT  "hh:mm:ss:ttt : "
#define ADV_PHASE_ERR_FORMAT   "[%04X] "
#define ADV_PHASE_MAX_SIZE     32

/**

FormatTimeStamp

Adds a times tamp to the message being returned.  Returns the time stamp in the form
of "hh:mm:ss.ttt ".

@param  MessageBuffer
@param  MessageBufferSize
@param  TimeStamp

@retval Number of characters printed

*/
STATIC
UINT16
FormatTimeStamp (
  IN CHAR8   *MessageBuffer,
  IN UINTN   MessageBufferSize,
  IN UINT64  TimeStamp
  )
{
  UINTN  Hours;
  UINTN  Minutes;
  UINTN  Seconds;
  UINTN  Milliseconds;
  UINTN  Temp;
  UINTN  TimeStampLen;

  Temp         = GetTimeInNanoSecond (TimeStamp);
  Temp         = Temp / (1000 * 1000);        // Get time in ms.
  Hours        = Temp / (1000 * 60 * 60);
  Temp         = Temp % (1000 * 60 * 60);
  Minutes      = Temp / (1000 * 60);
  Temp         = Temp % (1000 * 60);
  Seconds      = Temp / 1000;
  Milliseconds = Temp % 1000;

  //             prints        "hh:mm:ss:ttt "

  TimeStampLen = AsciiSPrint (
                   MessageBuffer,
                   MessageBufferSize,
                   ADV_TIME_STAMP_FORMAT,
                   Hours,
                   Minutes,
                   Seconds,
                   Milliseconds
                   );

  ASSERT (TimeStampLen == AsciiStrLen (ADV_TIME_STAMP_RESULT));

  return (UINT16)TimeStampLen;
}

/**
  FormatPhasePrefix

  Adds a phase indicator to the message being returned.  If phase is recognized and specified,
  returns the phase prefix in from the AdvMsgEntryPrefix, otherwise raw phase value is returned.

  @param  MessageBuffer
  @param  MessageBufferSize
  @param  Phase

  @retval Number of characters printed
*/
STATIC
UINT16
FormatPhasePrefix (
  IN CHAR8   *MessageBuffer,
  IN UINTN   MessageBufferSize,
  IN UINT16  Phase
  )
{
  UINTN  PhaseStringLen;

  if (Phase == ADVANCED_LOGGER_PHASE_UNSPECIFIED) {
    // This might be a legacy message
    PhaseStringLen = AsciiSPrint (MessageBuffer, MessageBufferSize, "");
  } else if (Phase < ADVANCED_LOGGER_PHASE_CNT) {
    // Normal message we recognize
    PhaseStringLen = AsciiSPrint (MessageBuffer, MessageBufferSize, AdvMsgEntryPrefix[Phase]);
  } else {
    // Unrecognized phase, just print the raw value
    PhaseStringLen = AsciiSPrint (MessageBuffer, MessageBufferSize, ADV_PHASE_ERR_FORMAT, Phase);
  }

  return (UINT16)PhaseStringLen;
}

/**
  Get Next Message Block.

  Get the next content of a message from the in memory buffer.

  When the CurrentMessage structure is initialized to NULL, the first message is returned. While
  not expected during normal use, to start reading from the beginning of the log again, set the
  Context field NULL.  That memory pointed to by Context may be freed with FreePool.

  NOTE:  The message pointed to by CurrentMessage->Message is NOT NULL terminated.

  @param  CurrentMessage         Information about the current message.

  @retval EFI_SUCCESS            CurrentMessage-Message points to a Message Length message that
                                 is NOT NULL terminated.
          EFI_NOT_STARTED        Error occurred during constructor
          EFI_INVALID_PARAMETER  A Bad CurrentMessage pointer provided
          EFI_END_OF_FILE        No more messages in the memory buffer.  ResumeContext is still
                                 valid to check for more messages.

**/
EFI_STATUS
EFIAPI
AdvancedLoggerAccessLibGetNextMessageBlock (
  IN  ADVANCED_LOGGER_ACCESS_MESSAGE_BLOCK_ENTRY  *BlockEntry
  )
{
  ADVANCED_LOGGER_MESSAGE_ENTRY     *LogEntry   = NULL;
  ADVANCED_LOGGER_MESSAGE_ENTRY_V2  *LogEntryV2 = NULL;

  if (mLoggerInfo == NULL) {
    return EFI_NOT_STARTED;
  }

  if (BlockEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mLoggerInfo->LogCurrentOffset == mLoggerInfo->LogBufferOffset) {
    return EFI_END_OF_FILE;
  }

  if (BlockEntry->Message == NULL) {
    LogEntry = (ADVANCED_LOGGER_MESSAGE_ENTRY *)LOG_BUFFER_FROM_ALI (mLoggerInfo);
    if (LogEntry->Signature == MESSAGE_ENTRY_SIGNATURE_V2) {
      // This is actually a v2 entry.
      LogEntryV2 = (ADVANCED_LOGGER_MESSAGE_ENTRY_V2 *)LogEntry;
    }
  } else {
    LogEntry = (ADVANCED_LOGGER_MESSAGE_ENTRY *)MESSAGE_ENTRY_FROM_MSG (BlockEntry->Message);
    if (LogEntry->Signature != MESSAGE_ENTRY_SIGNATURE) {
      // If this is not a v1 entry, this might be a v2 entry.
      LogEntryV2 = (ADVANCED_LOGGER_MESSAGE_ENTRY_V2 *)MESSAGE_ENTRY_FROM_MSG_V2 (BlockEntry->Message, BlockEntry->MessageOffset);
      if (LogEntryV2->Signature != MESSAGE_ENTRY_SIGNATURE_V2) {
        DEBUG ((DEBUG_ERROR, "Resume LogEntry invalid signature at %p or %p\n", LogEntry, LogEntryV2));
        DUMP_HEX (DEBUG_INFO, 0, (CHAR8 *)LogEntry - 128, 256, "");
        return EFI_INVALID_PARAMETER;
      }
    }

    if (LogEntryV2) {
      LogEntryV2 = NEXT_LOG_ENTRY_V2 (LogEntryV2);
    } else {
      LogEntry = NEXT_LOG_ENTRY (LogEntry);
    }
  }

  // At this point, if LogEntryV2 is not NULL, it points to the next entry to be read.
  // Otherwise LogEntry will contain the next entry. So we simplify the logic by only
  // using LogEntry and overwriting it to use the LogEntryV2 data as necessary. However,
  // note that regardless of how we inherit the pointer it has the possibility of
  // pointing to a different version of structure than the one we just looked at. So
  // we need to validate the structure before we can use it.
  if (LogEntryV2 != NULL) {
    LogEntry = (ADVANCED_LOGGER_MESSAGE_ENTRY *)LogEntryV2;
  }

  // Validate that LogEntry points within the proper Memory Log region
  // in memory log buffer
  if ((LogEntry != (ADVANCED_LOGGER_MESSAGE_ENTRY *)ALIGN_POINTER (LogEntry, 8)) || // Insure pointer is on boundary
      (PA_FROM_PTR (LogEntry) < mLowAddress) ||                                     // and within the log region
      (PA_FROM_PTR (LogEntry) > mHighAddress))
  {
    DEBUG ((DEBUG_ERROR, "Invalid Address for LogEntry %p. Low=%p, High=%p\n", LogEntry, mLowAddress, mHighAddress));
    return EFI_INVALID_PARAMETER;
  }

  if (LogEntry >= (ADVANCED_LOGGER_MESSAGE_ENTRY *)LOG_CURRENT_FROM_ALI (mLoggerInfo)) {
    return EFI_END_OF_FILE;
  }

  if (LogEntry->Signature == MESSAGE_ENTRY_SIGNATURE) {
    BlockEntry->TimeStamp  = LogEntry->TimeStamp;
    BlockEntry->DebugLevel = LogEntry->DebugLevel;
    BlockEntry->Message    = LogEntry->MessageText;
    BlockEntry->MessageLen = LogEntry->MessageLen;
    BlockEntry->Phase      = ADVANCED_LOGGER_PHASE_UNSPECIFIED;
  } else if (LogEntry->Signature == MESSAGE_ENTRY_SIGNATURE_V2) {
    LogEntryV2                = (ADVANCED_LOGGER_MESSAGE_ENTRY_V2 *)LogEntry;
    BlockEntry->TimeStamp     = LogEntryV2->TimeStamp;
    BlockEntry->DebugLevel    = LogEntryV2->DebugLevel;
    BlockEntry->Message       = LogEntryV2->MessageText;
    BlockEntry->MessageLen    = LogEntryV2->MessageLen;
    BlockEntry->MessageOffset = LogEntryV2->MessageOffset;
    BlockEntry->Phase         = LogEntryV2->Phase;
  } else {
    DEBUG ((DEBUG_ERROR, "Next LogEntry invalid signature at %p, Last=%p\n", LogEntry, BlockEntry->Message));
    DUMP_HEX (DEBUG_INFO, 0, (CHAR8 *)BlockEntry->Message - 128, 256, "");
    DUMP_HEX (DEBUG_INFO, 0, (CHAR8 *)LogEntry - 128, 256, "");
    return EFI_COMPROMISED_DATA;
  }

  return EFI_SUCCESS;
}

/**
  Get Next Formatted line.

  Get the next set of output characters up to and including the next \n.  The
  message is formatted with a time stamp.

  When the LineEntry structure is initialized to NULL, the first message is returned. Each
  subsequent call gets the portion of or next set of block messages that make up a single line.


  @param  CurrentMessage         Information about the current message.

  @retval EFI_SUCCESS            CurrentMessage->Message points to Message Length message that
                                 is properly NULL terminated. The NULL is not counted in the
                                 MessageLen field.

          EFI_NOT_STARTED        Error occurred during constructor
          EFI_INVALID_PARAMETER  A Bad CurrentMessage pointer provided
          EFI_END_OF_FILE        No more messages in the memory buffer. The private fields are
                                 still valid to check for more messages.

**/
EFI_STATUS
EFIAPI
AdvancedLoggerAccessLibGetNextFormattedLine (
  IN  ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY  *LineEntry
  )
{
  CHAR8       LastChar;
  CHAR8       *LineBuffer;
  EFI_STATUS  Status;
  CHAR8       *TargetPtr;
  UINT16      TargetLen;
  UINT16      PhaseStringLen;
  UINT16      CurrPhaseStringLen;
  CHAR8       TimeStampString[]               = { ADV_TIME_STAMP_RESULT };
  CHAR8       PhaseString[ADV_PHASE_MAX_SIZE] = { 0 };

  if (LineEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Only allocate one LineBuffer for an BlockEntry.  Once it is allocated,
  // reuse the previous LineBuffer
  //
  if (LineEntry->Message == NULL) {
    LineBuffer = AllocatePool (mMaxMessageSize + sizeof (TimeStampString) + ADV_PHASE_MAX_SIZE);
    if (LineBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    LineEntry->Message = LineBuffer;
  } else {
    LineBuffer = LineEntry->Message;
  }

  // Treat the incoming messages as a character pipe, and pull characters from the character
  // pipe up to and including '\n'.  Any characters in a MessageLog message after
  // the first '\n' are left in the ResidualMemoryBuffer for use on the next call to
  // GetNextLine.

  // In case this is a restart of the same Message, initialize the time stamp and prefix.
  PhaseStringLen = 0;
  if (LineEntry->BlockEntry.Message != NULL) {
    FormatTimeStamp (TimeStampString, sizeof (TimeStampString), LineEntry->BlockEntry.TimeStamp);
    CopyMem (LineBuffer, TimeStampString, sizeof (TimeStampString) - sizeof (CHAR8));
    PhaseStringLen = FormatPhasePrefix (PhaseString, sizeof (PhaseString), LineEntry->BlockEntry.Phase);
    CopyMem (LineBuffer + sizeof (TimeStampString) - sizeof (CHAR8), PhaseString, PhaseStringLen);
  }

  TargetPtr = &LineBuffer[sizeof (TimeStampString) - sizeof (CHAR8) + PhaseStringLen];
  TargetLen = 0;
  Status    = EFI_SUCCESS;

  do {
    // Check for existing data.

    if (LineEntry->ResidualLen > 0) {
      LastChar = '\0';
      while ((LineEntry->ResidualLen > 0) &&
             (LastChar != '\n') &&
             (TargetLen < (UINT16)(mMaxMessageSize - 2)))
      {
        LastChar     = *LineEntry->ResidualChar++;
        *TargetPtr++ = LastChar;
        TargetLen++;
        LineEntry->ResidualLen--;
      }

      if (LastChar == '\n') {
        *TargetPtr = '\0';
        break;
      }

      if (TargetLen >= (mMaxMessageSize - 2)) {
        *TargetPtr++ = '\n';
        *TargetPtr   = '\0';
        TargetLen++;
        break;
      }

      if (LineEntry->ResidualLen != 0) {
        Status = EFI_ABORTED;
        break;
      }
    }

    //
    // Get next message block using the formatted line master
    // access entry.
    //
    Status = AdvancedLoggerAccessLibGetNextMessageBlock (&LineEntry->BlockEntry);

    if (Status == EFI_END_OF_FILE) {
      if (TargetLen > 0) {
        Status = EFI_SUCCESS;
      }

      break;
    }

    if (!EFI_ERROR (Status)) {
      LineEntry->ResidualChar = LineEntry->BlockEntry.Message;
      LineEntry->ResidualLen  = LineEntry->BlockEntry.MessageLen;
      FormatTimeStamp (TimeStampString, sizeof (TimeStampString), LineEntry->BlockEntry.TimeStamp);
      CopyMem (LineBuffer, TimeStampString, sizeof (TimeStampString) - sizeof (CHAR8));
      CurrPhaseStringLen = FormatPhasePrefix (PhaseString, sizeof (PhaseString), LineEntry->BlockEntry.Phase);
      if (PhaseStringLen != CurrPhaseStringLen) {
        // Adjust the TargetPtr to point to the end of the PhaseString
        PhaseStringLen = CurrPhaseStringLen;
        TargetPtr      = &LineBuffer[sizeof (TimeStampString) - sizeof (CHAR8) + PhaseStringLen];
      }

      CopyMem (LineBuffer + sizeof (TimeStampString) - sizeof (CHAR8), PhaseString, PhaseStringLen);
    }
  } while (!EFI_ERROR (Status));

  if (!EFI_ERROR (Status)) {
    LineEntry->MessageLen = TargetLen + sizeof (TimeStampString) - sizeof (CHAR8) + PhaseStringLen;
    LineEntry->TimeStamp  = LineEntry->BlockEntry.TimeStamp;
    LineEntry->DebugLevel = LineEntry->BlockEntry.DebugLevel;
    LineEntry->Phase      = LineEntry->BlockEntry.Phase;
  }

  return Status;
}

/**
  Advanced Logger Unit Test Initialize

  Allows the Unit Test to reset internal operation and to provide its own internal
  memory log.

  @param    TestProtocol    Unit test instance of the AdvancedLoggerProtocol
  @param    MaxMessageSize  Allows unit test to specify a nominal max message

*/
EFI_STATUS
EFIAPI
AdvancedLoggerAccessLibUnitTestInitialize (
  IN ADVANCED_LOGGER_PROTOCOL  *TestProtocol  OPTIONAL,
  IN UINT16                    MaxMessageSize
  )
{
  ADVANCED_LOGGER_PROTOCOL  *LoggerProtocol = NULL;
  EFI_STATUS                Status;

  if (TestProtocol == NULL) {
    Status = gBS->LocateProtocol (
                    &gAdvancedLoggerProtocolGuid,
                    NULL,
                    (VOID **)&LoggerProtocol
                    );
  } else {
    LoggerProtocol = TestProtocol;
    Status         = EFI_SUCCESS;
  }

  if (MaxMessageSize == 0) {
    mMaxMessageSize = ADVANCED_LOGGER_MAX_MESSAGE_SIZE;
  } else {
    mMaxMessageSize = MaxMessageSize;
  }

  if (!EFI_ERROR (Status)) {
    mLoggerInfo  = LOGGER_INFO_FROM_PROTOCOL (LoggerProtocol);
    mLowAddress  = PA_FROM_PTR (LOG_BUFFER_FROM_ALI (mLoggerInfo));
    mHighAddress = PA_FROM_PTR (TOTAL_LOG_SIZE_WITH_ALI (mLoggerInfo));
  }

  return Status;
}

/**
  Advanced Logger Library Constructor.
 **/
EFI_STATUS
EFIAPI
AdvancedLoggerAccessLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  ADVANCED_LOGGER_PROTOCOL  *LoggerProtocol = NULL;
  EFI_STATUS                Status;

  Status = gBS->LocateProtocol (
                  &gAdvancedLoggerProtocolGuid,
                  NULL,
                  (VOID **)&LoggerProtocol
                  );
  if (!EFI_ERROR (Status)) {
    mLoggerInfo  = LOGGER_INFO_FROM_PROTOCOL (LoggerProtocol);
    mLowAddress  = PA_FROM_PTR (LOG_BUFFER_FROM_ALI (mLoggerInfo));
    mHighAddress = PA_FROM_PTR (TOTAL_LOG_SIZE_WITH_ALI (mLoggerInfo));

    // Leave this debug message as ERROR.

    DEBUG ((DEBUG_ERROR, "Advanced Logger Info = %p, Min = %p, Max = %p\n", mLoggerInfo, mLowAddress, mHighAddress));
  }

  // Don't fail module load...
  return EFI_SUCCESS;
}

/**
  AdvancedLoggerAccessLibReset.

  Free allocated buffers for LineEntry.


  @param  AccessMessage          Information about the current message.

  @retval EFI_SUCCESS            Possible LineBuffer freed
          EFI_INVALID_PARAMETER  A Bad LineEntry pointer provided

**/
EFI_STATUS
EFIAPI
AdvancedLoggerAccessLibReset (
  IN  ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY  *LineEntry
  )
{
  if (LineEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (LineEntry->Message != NULL) {
    FreePool (LineEntry->Message);
    LineEntry->Message = NULL;
  }

  return EFI_SUCCESS;
}
