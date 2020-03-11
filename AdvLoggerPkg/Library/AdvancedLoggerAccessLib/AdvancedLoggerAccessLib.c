/** @file
  Implementation of Advanced Logger Access Library.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/AdvancedLogger.h>

#include <Library/AdvancedLoggerAccessLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <AdvancedLoggerInternal.h>


STATIC  ADVANCED_LOGGER_INFO          *mLoggerInfo = NULL;
STATIC  ADVANCED_LOGGER_MESSAGE_ENTRY *mLowAddress = NULL;
STATIC  ADVANCED_LOGGER_MESSAGE_ENTRY *mHighAddress = NULL;
STATIC  UINTN                          mMaxMessageSize = ADVANCED_LOGGER_MAX_MESSAGE_SIZE;

#define ADV_TIME_STAMP_FORMAT "%2d:%2.2d:%2.2d.%3.3d "
#define ADV_TIME_STAMP_RESULT "hh:mm:ss:ttt "


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
    IN UINTN    MessageBufferSize,
    IN UINT64   TimeStamp
  ) {
    UINTN   Hours;
    UINTN   Minutes;
    UINTN   Seconds;
    UINTN   Milliseconds;
    UINTN   Temp;
    UINTN   TimeStampLen;

    Temp = GetTimeInNanoSecond (TimeStamp);
    Temp = Temp / (1000 * 1000);              // Get time in ms.
    Hours = Temp / (1000 * 60 * 60);
    Temp = Temp % (1000 * 60 * 60);
    Minutes = Temp / (1000 * 60);
    Temp = Temp % (1000 * 60);
    Seconds = Temp / 1000;
    Milliseconds = Temp % 1000;

//             prints        "hh:mm:ss:ttt "

    TimeStampLen = AsciiSPrint(MessageBuffer,
                               MessageBufferSize,
                               ADV_TIME_STAMP_FORMAT,
                               Hours,
                               Minutes,
                               Seconds,
                               Milliseconds);

    ASSERT (TimeStampLen == AsciiStrLen (ADV_TIME_STAMP_RESULT));

    return (UINT16) TimeStampLen;

}

/**
  Get Next Message Block.

  Get the next content of a DEBUG(()) message from the in memory buffer.

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
    IN  ADVANCED_LOGGER_ACCESS_MESSAGE_BLOCK_ENTRY *BlockEntry
  ) {
    ADVANCED_LOGGER_MESSAGE_ENTRY           *LogEntry;

    if (mLoggerInfo == NULL) {
        return EFI_NOT_STARTED;
    }

    if (BlockEntry == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    if (mLoggerInfo->LogCurrent == mLoggerInfo->LogBuffer) {
        return EFI_END_OF_FILE;
    }

    if (BlockEntry->Message == NULL) {
        LogEntry = (ADVANCED_LOGGER_MESSAGE_ENTRY *) PTR_FROM_PA(mLoggerInfo->LogBuffer);
    } else {
        LogEntry = (ADVANCED_LOGGER_MESSAGE_ENTRY *) MESSAGE_ENTRY_FROM_MSG(BlockEntry->Message);
        if (LogEntry->Signature != MESSAGE_ENTRY_SIGNATURE) {
            DEBUG((DEBUG_ERROR, "Resume LogEntry invalid signature at %p\n", LogEntry));
            DEBUG_BUFFER(DEBUG_INFO, (CHAR8 *)LogEntry - 128, 256, DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
            return EFI_INVALID_PARAMETER;
        }
        LogEntry = NEXT_LOG_ENTRY (LogEntry);
    }

    // Validate that LogEntry points within the proper Memory Log region
    // in memory log buffer
    if  ((LogEntry != (ADVANCED_LOGGER_MESSAGE_ENTRY *) ALIGN_POINTER(LogEntry, 8)) ||   // Insure pointer is on boundary
         (LogEntry < mLowAddress) ||                   // and within the log region
         (LogEntry > mHighAddress)) {
        DEBUG((DEBUG_ERROR, "Invalid Address for LogEntry %p. Low=%p, High=%p\n", LogEntry, mLowAddress, mHighAddress));
        return EFI_INVALID_PARAMETER;
    }

    if (LogEntry >= (ADVANCED_LOGGER_MESSAGE_ENTRY *) PTR_FROM_PA(mLoggerInfo->LogCurrent)) {
        return EFI_END_OF_FILE;
    }

    if (LogEntry->Signature != MESSAGE_ENTRY_SIGNATURE) {
        DEBUG((DEBUG_ERROR, "Next LogEntry invalid signature at %p, Last=%p\n", LogEntry, BlockEntry->Message));
        DEBUG_BUFFER(DEBUG_INFO, (CHAR8 *)BlockEntry->Message - 128, 256, DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
        DEBUG_BUFFER(DEBUG_INFO, (CHAR8 *)LogEntry - 128, 256, DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
        return EFI_COMPROMISED_DATA;
    }

    BlockEntry->TimeStamp = LogEntry->TimeStamp;
    BlockEntry->DebugLevel = LogEntry->DebugLevel;
    BlockEntry->Message = LogEntry->MessageText;
    BlockEntry->MessageLen = LogEntry->MessageLen;

    return EFI_SUCCESS;
}

/**
  Get Next Formatted line.

  Get the next set of DEBUG(()) output characters up to and including the next \n.  The
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
    IN  ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY *LineEntry
  ) {
    CHAR8                                    LastChar;
    CHAR8                                   *LineBuffer;
    EFI_STATUS                               Status;
    CHAR8                                   *TargetPtr;
    UINT16                                   TargetLen;
    CHAR8                                    TimeStampString[] = {ADV_TIME_STAMP_RESULT};

    if (LineEntry == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    //
    // Only allocate one LineBuffer for an BlockEntry.  Once it is allocated,
    // reuse the previous LineBuffer
    //
    if (LineEntry->Message == NULL) {
        LineBuffer = AllocatePool (mMaxMessageSize+sizeof(TimeStampString));
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

    // In case this is a restart of the same Message, initialize the time stamp.
    if (LineEntry->BlockEntry.Message != NULL) {
        FormatTimeStamp (TimeStampString, sizeof(TimeStampString), LineEntry->BlockEntry.TimeStamp);
        CopyMem (LineBuffer, TimeStampString, sizeof(TimeStampString) - sizeof(CHAR8));
    }

    TargetPtr = &LineBuffer[sizeof(TimeStampString) - sizeof(CHAR8)];
    TargetLen = 0;
    Status = EFI_SUCCESS;

    do {
        // Check for existing data.

        if (LineEntry->ResidualLen > 0) {
            LastChar = '\0';
            while ((LineEntry->ResidualLen > 0) &&
                   (LastChar != '\n') &&
                   (TargetLen < (mMaxMessageSize - 2))) {
                LastChar = *LineEntry->ResidualChar++;
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
               *TargetPtr = '\0';
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

        if (!EFI_ERROR(Status)) {
            LineEntry->ResidualChar = LineEntry->BlockEntry.Message;
            LineEntry->ResidualLen = LineEntry->BlockEntry.MessageLen;
            FormatTimeStamp (TimeStampString, sizeof(TimeStampString), LineEntry->BlockEntry.TimeStamp);
            CopyMem (LineBuffer, TimeStampString, sizeof(TimeStampString) - sizeof(CHAR8));
        }

    } while (!EFI_ERROR(Status));


    if (!EFI_ERROR(Status)) {
        LineEntry->MessageLen = TargetLen + sizeof(TimeStampString) - sizeof(CHAR8);
        LineEntry->TimeStamp = LineEntry->BlockEntry.TimeStamp;
        LineEntry->DebugLevel = LineEntry->BlockEntry.DebugLevel;
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
    IN ADVANCED_LOGGER_PROTOCOL *TestProtocol  OPTIONAL,
    IN UINTN                     MaxMessageSize
  ) {
    ADVANCED_LOGGER_PROTOCOL   *LoggerProtocol = NULL;
    EFI_STATUS                  Status;

    if (TestProtocol == NULL) {
        Status = gBS->LocateProtocol (&gAdvancedLoggerProtocolGuid,
                                       NULL,
                                      (VOID **) &LoggerProtocol);
    } else {
        LoggerProtocol = TestProtocol;
        Status = EFI_SUCCESS;
    }

    if (MaxMessageSize == 0) {
        mMaxMessageSize = ADVANCED_LOGGER_MAX_MESSAGE_SIZE;
    } else {
        mMaxMessageSize = MaxMessageSize;
    }

    if (!EFI_ERROR(Status)) {
        mLoggerInfo = (ADVANCED_LOGGER_INFO *) LoggerProtocol->Context;
        mLowAddress = (ADVANCED_LOGGER_MESSAGE_ENTRY *) PTR_FROM_PA(mLoggerInfo->LogBuffer);
        mHighAddress = (ADVANCED_LOGGER_MESSAGE_ENTRY *) PTR_FROM_PA(mLoggerInfo->LogBuffer + mLoggerInfo->LogBufferSize);
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
  ) {
    ADVANCED_LOGGER_PROTOCOL   *LoggerProtocol = NULL;
    EFI_STATUS                  Status;

    Status = gBS->LocateProtocol (&gAdvancedLoggerProtocolGuid,
                                   NULL,
                                  (VOID **) &LoggerProtocol);
    if (!EFI_ERROR(Status)) {
        mLoggerInfo = (ADVANCED_LOGGER_INFO *) LoggerProtocol->Context;
        mLowAddress = (ADVANCED_LOGGER_MESSAGE_ENTRY *) PTR_FROM_PA(mLoggerInfo->LogBuffer);
        mHighAddress = (ADVANCED_LOGGER_MESSAGE_ENTRY *) PTR_FROM_PA(mLoggerInfo->LogBuffer + mLoggerInfo->LogBufferSize);

        // Leave this debug message as ERROR.

        DEBUG((DEBUG_ERROR, "Advanced Logger Info = %p, Max = %p\n", mLoggerInfo, mHighAddress));
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
AdvancedLoggerAccessLibReset(
    IN  ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY *LineEntry
  ) {

    if (LineEntry == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    if (LineEntry->Message != NULL) {
        FreePool (LineEntry->Message);
        LineEntry->Message = NULL;
    }

    return EFI_SUCCESS;
}
