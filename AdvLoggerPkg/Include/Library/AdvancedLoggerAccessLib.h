/** @file AdvancedLoggerAccessLib.h

  Advanced Logger Access Library interface

  Copyright (C) Microsoft Corporation. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ADVANCED_LOGGER_ACCESS_LIB_H__
#define __ADVANCED_LOGGER_ACCESS_LIB_H__


/**

This file logger access method will break up lines longer than the
MAX MESSAGE size.
*/

#define ADVANCED_LOGGER_MAX_MESSAGE_SIZE    512

//
// NOTE:
//
//     ACCESS_MESSAGE structures MUST be zeroed before first use.
//
//     The private fields are used to maintain the current position when accessing the
//     memory log buffer.
//
// The Message field is different between BLOCK_ENTRY and LINE_ENTRY.  For BLOCK_ENTRY,
// the returned Message pointer points to raw text in the reserved memory space. MessageLen
// is the number of valid characters.  There is no NULL guaranteed to be present.
//
// For LINE_ENTRY, the returned Message pointer is a one time allocated buffer.  The built
// line will be NULL terminated.
//
// If desired, an application may call AdvancedLoggerAccessLibReset to free any memory
// allocated for the one time allocated lineBuffer.
//

typedef struct {

    // Message is IN/OUT. On the first input, it must be NULL.  On subsequent
    // calls, the previously returned pointer to know where to get the next
    // message.  Message is a pointer into the physical memory buffer, and it NOT
    // properly terminated.
    CONST CHAR8    *Message;                // NULL (first), Pointer to Current  Message Text
    UINT32          DebugLevel;             // DEBUG Message Level
    UINT16          MessageLen;             // Number of bytes in Message
    UINT16          Reserved;
    UINT64          TimeStamp;              // Time stamp
} ADVANCED_LOGGER_ACCESS_MESSAGE_BLOCK_ENTRY;

typedef struct {

    // Message is IN/OUT. On the first input, it must be NULL.  On subsequent
    // calls, it must be the value previously returned.  It is a LineBuffer to be used for
    // formatting a line.  Message will point to a properly NULL terminated ASCII STRING.

    // BlockEntry is used to manage the current place in the physical memory buffer.
    CHAR8          *Message;                // Pointer to Message Text
    UINT32          DebugLevel;             // DEBUG Message Level
    UINT16          MessageLen;             // Number of bytes in Message
    UINT16          Reserved;
    UINT64          TimeStamp;              // Time stamp

    // The following are private members for GetNextBlock.  Initialize these
    // members to NULL and 0 respectively.

    CONST CHAR8    *ResidualChar;           // (Private) Initialize to NULL.
    UINT16          ResidualLen;            // (Private) Initialize to 0
    ADVANCED_LOGGER_ACCESS_MESSAGE_BLOCK_ENTRY BlockEntry;
} ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY;


/**
  Get Next Message Block.

  Get the next content of a DEBUG(()) message from the in memory buffer.

  When the AccessEntry structure is initialized to NULL, the first message is returned. While
  not expected during normal use, the Context field may be freed with FreePool, and the Context
  field set to NULL, to start reading from the beginning of the log again.

  NOTE:  The message pointed to by AccessEntry->Message is NOT NULL terminated.

  @param  BlockEntry             Information about the current message block.

  @retval EFI_SUCCESS            AccessEntry->Message points to a Message Length message that
                                 is NOT NULL terminated. The Message must NOT be freed, and
                                 must treated as a CONSTANT.
          EFI_NOT_STARTED        Error occurred during library constructor
          EFI_INVALID_PARAMETER  A Bad AccessEntry pointer provided
          EFI_END_OF_FILE        No more messages in the memory buffer.
                                 AccessEntry->Context is still valid to check for more messages.

**/
EFI_STATUS
EFIAPI
AdvancedLoggerAccessLibGetNextMessageBlock (
    IN ADVANCED_LOGGER_ACCESS_MESSAGE_BLOCK_ENTRY *BlockEntry
  );


/**
  Get Next Formatted line.

  Get the next set of DEBUG(()) output characters up to and including the next \n.  The
  message is formatted with a time stamp.


  @param  LineEntry              Information about the current message line.

  @retval EFI_SUCCESS            BlockEntry->Message points to a Message Length message that
                                 is properly NULL terminated. The NULL is not counted in the
                                 MessageLen field.

          EFI_NOT_STARTED        Error occurred during constructor
          EFI_INVALID_PARAMETER  A Bad AccessEntry pointer provided
          EFI_END_OF_FILE        No more messages in the memory buffer.
                                 AccessEntry->Context is still valid to check for more messages.

**/
EFI_STATUS
EFIAPI
AdvancedLoggerAccessLibGetNextFormattedLine (
    IN  ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY *LineEntry
  );

/**
  AdvancedLoggerAccessLibReset.

  Free allocated buffers for AccessEntry.


  @param  AccessMessage          Information about the current message.

  @retval EFI_SUCCESS            AccessEntry->Message points to a Message Length message that
                                 is properly NULL terminated. The NULL is not counted in the
                                 MessageLen field. The caller is expected to FreePool() this
                                 message.

          EFI_INVALID_PARAMETER  A Bad AccessEntry pointer provided

**/
EFI_STATUS
EFIAPI
AdvancedLoggerAccessLibReset(
    IN  ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY *AccessEntry
  );

/**
  Advanced Logger Unit Test Initialize

  ** Meant for UNIT TEST purposes.  Use at your own risk **

  Allows the Unit Test to reset internal operation and to provide its own internal
  memory log and maximum message size to insure proper operation.

*/
EFI_STATUS
EFIAPI
AdvancedLoggerAccessLibUnitTestInitialize (
    IN  ADVANCED_LOGGER_PROTOCOL *TestProtocol  OPTIONAL,
    IN  UINTN                     MaxMessageSize
  );

#endif  // __ADVANCED_LOGGER_ACCESS_LIB_H__
