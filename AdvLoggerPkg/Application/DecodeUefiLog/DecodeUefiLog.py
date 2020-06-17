# @file
#
# Decode a memory capture of AdvLogger memory
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

import struct
import argparse
import tempfile

from win32com.shell import shell

from UefiVariablesSupportLib import UefiVariable


class AdvLogParser ():

    # ----------------------------------------------------------------------- #
    #
    # AdvLogParser - Parser for the in memory AdvLoggerPkg buffer using UEFI Get Variable, or
    #                from a captured AdvLoggerPkg buffer captured to a file.
    #
    # ----------------------------------------------------------------------- #

    # ----------------------------------------------------------------------- #
    #
    # AdvLoggerPkg structure definitions and their mapping as a Python Dictionary
    #
    # ----------------------------------------------------------------------- #

    # ----------------------------------------------------------------------- #
    #
    #
    # typedef struct
    #
    #   // Message is IN/OUT. On the first input, it must be NULL.  On subsequent
    #   // calls, the previously returned pointer to know where to get the next
    #   // message.  Message is a pointer into the physical memory buffer, and it NOT
    #   // properly terminated.
    #   CONST CHAR8    *Message;                // NULL (first), Pointer to Current  Message Text
    #   UINT32          DebugLevel;             // DEBUG Message Level
    #   UINT16          MessageLen;             // Number of bytes in Message
    #   UINT16          Reserved;
    #   UINT64          TimeStamp;              // Time stamp
    # } ADVANCED_LOGGER_ACCESS_MESSAGE_BLOCK_ENTRY;
    #
    # The dictionary entries for MessageBlock is based on the UEFI structure above
    #
    #   Members of MessageBlock:
    #
    #   Message         Offset into file/memory blob for current message block
    #   DebugLevel      DebugLevel
    #   MessageLen      Length of message
    #   TimeStamp       Timestamp               // Based on TSC rate
    #
    # ----------------------------------------------------------------------- #

    # ----------------------------------------------------------------------- #
    #
    #
    # typedef struct {
    #
    #    // Message is IN/OUT. On the first input, it must be NULL.  On subsequent
    #    // calls, it must be the value previously returned.  It is a LineBuffer to be used for
    #    // formatting a line.  Message will point to a properly NULL terminated ASCII STRING.
    #
    #    // BlockEntry is used to manage the current place in the physical memory buffer.
    #    CHAR8          *Message;                // Pointer to Message Text
    #    UINT32          DebugLevel;             // DEBUG Message Level
    #    UINT16          MessageLen;             // Number of bytes in Message
    #    UINT16          Reserved;
    #    UINT64          TimeStamp;              // Time stamp
    #
    #    // The following are private members for GetNextBlock.  Initialize these
    #    // members to NULL and 0 respectively.
    #
    #    CONST CHAR8    *ResidualChar;           // (Private) Initialize to NULL.
    #    UINT16          ResidualLen;            // (Private) Initialize to 0
    #    ADVANCED_LOGGER_ACCESS_MESSAGE_BLOCK_ENTRY BlockEntry;
    # } ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY;
    #
    # The dictionary entries for AccessMessageLineEntry is based on the UEFI structure above
    #
    #  Members of AccessMessageLineEntry
    #
    #  Message          Offset into file for current message block
    #  DebugLevel       DEBUG message level
    #  MessageLen       Number of bytes in Message
    #  TimeStamp        Timestamp                 // Based on TSC rate
    #  ResidualChar     Residual string in python
    #  ResidualLen      Length of remaining string
    #
    # ----------------------------------------------------------------------- #

    # ----------------------------------------------------------------------- #
    #
    #
    #    typedef volatile struct {
    #        UINT32                Signature;              // Signature 'ALOG'
    #        UINT16                Version;                // Current Version
    #        UINT16                Reserved;               // Reserved for future
    #        EFI_PHYSICAL_ADDRESS  LogBuffer;              // Fixed pointer to start of log
    #        EFI_PHYSICAL_ADDRESS  LogCurrent;             // Where to store next log entry.
    #        UINT32                DiscardedSize;          // Number of bytes of messages missed
    #        UINT32                LogBufferSize;
    #        BOOLEAN               SerialInitialized;      // Serial port initialized
    #        BOOLEAN               InPermanentRAM;         // Log in permanent RAM
    #        BOOLEAN               ExitBootServices;       // Exit Boot Services occurred
    #        BOOLEAN               PeiAllocated;           // Pei allocated "Temp Ram"
    #        UINT64                TimerFrequency;         // Ticks per second for log timing
    #    } ADVANCED_LOGGER_INFO;
    #
    LOGGER_INFO_SIZE = 52
    #
    # The dictionary entries for LoggerInfo is based on the UEFI structure above.
    #
    # Members of LoggerInfo
    #
    #  Signature
    #  LogBufferSize;
    #  LogBuffer;
    #  LogCurrent;
    #  DiscardedSize
    #  SerialInitialized
    #  InPermanentRAM
    #  ExitBootServices
    #  PeiAllocated
    #  BaseAdress
    #
    # ---------------------------------------------------------------------- #

    # ---------------------------------------------------------------------- #
    #
    #
    # typedef struct {
    #     UINT32                Signature;              // Signature
    #     UINT32                DebugLevel;             // Debug Level
    #     UINT64                TimeStamp;              // Time stamp
    #     UINT16                MessageLen;             // Number of bytes in Message
    #     CHAR8                 MessageText[];          // Message Text
    # } ADVANCED_LOGGER_MESSAGE_ENTRY;
    #
    MESSAGE_ENTRY_SIZE = 18
    MAX_MESSAGE_SIZE = 512
    #
    # The dictionary entries for MessageLineEntry is based on the UEFI structure above.
    #
    # Members of MessageLineEntry:
    #
    #  Signature
    #  DebugLevel
    #  TimeStamp
    #  MessageLen
    #  MessageText
    #
    # ---------------------------------------------------------------------- #

    # ---------------------------------------------------------------------- #
    #
    # Global Variables
    #
    # ---------------------------------------------------------------------- #
    StartLine = 0           # From argument StartLine
    CurrentLine = 0         # Internal counter tracing the number of lines printed
    Frequency = 2.5         # From argument Frequency and is in GigaHertz (GH)

    SUCCESS = 0
    END_OF_FILE = 1
    ABORTED = 2
    # ---------------------------------------------------------------------- #
    #
    # AdvLogParser Class Functions
    #
    # ---------------------------------------------------------------------- #

    # ---------------------------------------------------------------------- #
    #
    #  Initialize log Header
    #
    # ---------------------------------------------------------------------- #
    def _InitializeLoggerInfo(self, InFile):
        ''' Initialize log Header '''
        LoggerInfo = {}
        LoggerInfo["Signature"] = InFile.read(4).decode()
        LoggerInfo["LogBufferSize"] = struct.unpack("=I", InFile.read(4))[0]
        BaseAddress = struct.unpack("=Q", InFile.read(8))[0] + self.LOGGER_INFO_SIZE
        LoggerInfo["LogBuffer"] = self.LOGGER_INFO_SIZE
        LoggerInfo["LogCurrent"] = struct.unpack("=Q", InFile.read(8))[0]
        LoggerInfo["DiscardedSize"] = struct.unpack("=I", InFile.read(4))[0]
        LoggerInfo["SerialInitialized"] = struct.unpack("=B", InFile.read(1))[0]
        LoggerInfo["InPermanentRAM"] = struct.unpack("=B", InFile.read(1))[0]
        LoggerInfo["ExitBootServices"] = struct.unpack("=B", InFile.read(1))[0]
        LoggerInfo["PeiAllocated"] = struct.unpack("=B", InFile.read(1))[0]

        LoggerInfo["BaseAddress"] = BaseAddress
        LoggerInfo["LogCurrent"] -= BaseAddress
        LoggerInfo["LogCurrent"] += self.LOGGER_INFO_SIZE

        LoggerInfo["InFile"] = InFile

        if InFile.tell() != (self.LOGGER_INFO_SIZE):
            raise Exception('Error initializing logger info. AmountRead: 0x%X' % AmountRead)

        return LoggerInfo

    # ---------------------------------------------------------------------- #
    #
    # Main processing "private" functions
    #
    # ---------------------------------------------------------------------- #

    # ---------------------------------------------------------------------- #
    #
    #   _ReadMessageEntry - Read message segment from the file
    #
    # ---------------------------------------------------------------------- #
    def _ReadMessageEntry(self, LoggerInfo):
        MessageEntry = {}

        InFile = LoggerInfo["InFile"]

        MessageEntry["Signature"] = InFile.read(4).decode('utf-8', 'ignore')
        MessageEntry["DebugLevel"] = struct.unpack("=I", InFile.read(4))[0]
        MessageEntry["TimeStamp"] = struct.unpack("=Q", InFile.read(8))[0]
        MessageEntry["MessageLen"] = struct.unpack("=H", InFile.read(2))[0]
        MessageEntry["MessageText"] = InFile.read(MessageEntry["MessageLen"]).decode('utf-8', 'ignore')

        Skip = InFile.tell()
        Norm = int((int((Skip + 7) / 8)) * 8)
        Skip = Norm - Skip
        if Skip != 0:
            InFile.read(Skip)

        NextMessage = InFile.tell()
        NextMessageLen = self.MESSAGE_ENTRY_SIZE + int(int((MessageEntry["MessageLen"] + 7) / 8) * 8)
        NextMessage = NextMessage + NextMessageLen

        return (MessageEntry, NextMessage)

    # ---------------------------------------------------------------------- #
    #
    #   Read the next message block
    #
    # ---------------------------------------------------------------------- #
    def _GetNextMessageBlock(self, LoggerInfo):
        #
        # Unlike the UEFI version which needs to keep track of the memory
        # location, the InFile object has its own "current" file position.
        #
        MessageEntry = {}
        MessageBlock = {}

        InFile = LoggerInfo["InFile"]
        if LoggerInfo["LogBuffer"] == LoggerInfo["LogCurrent"]:
            return (self.END_OF_FILE, MessageBlock)

        if InFile.tell() >= LoggerInfo["LogCurrent"]:
            return (self.END_OF_FILE, MessageBlock)

        (MessageEntry, NextMessage) = self._ReadMessageEntry(LoggerInfo)

        if MessageEntry["Signature"] != 'ALMS':
            print("Log signature was incorrect.  Should be 'ALMS', was '%s'" % MessageEntry["Signature"])
            raise Exception("Message Block has wrong signature at offset 0x%X" % InFile.tell())

        MessageBlock["Message"] = MessageEntry["MessageText"]
        MessageBlock["DebugLevel"] = MessageEntry["DebugLevel"]
        MessageBlock["MessageLen"] = MessageEntry["MessageLen"]
        MessageBlock["TimeStamp"] = MessageEntry["TimeStamp"]

        return (self.SUCCESS, MessageBlock)

    # ---------------------------------------------------------------------- #
    #
    #   Read the next formatted line.  Combines multiple message blocks to
    #   make a complete line.
    #
    # ---------------------------------------------------------------------- #
    def _GetNextFormattedLine(self, AccessMessageLineEntry, LoggerInfo):
        AccessMessageBlock = {}
        AccessMessageBlock["TimeStamp"] = 0
        AccessMessageBlock["DebugLevel"] = 0

        if AccessMessageLineEntry is None:
            AccessMessageLineEntry = {}
            AccessMessageLineEntry["Message"] = 0
            AccessMessageLineEntry["ResidualLen"] = 0

        AccessMessageLineEntry["Message"] = 0
        Status = self.SUCCESS
        while (Status == self.SUCCESS):
            TargetLen = 0

            if AccessMessageLineEntry["ResidualLen"] > 0:
                LastChar = 0
                while (AccessMessageLineEntry["ResidualLen"] > 0 and
                       (LastChar != '\n') and
                       (TargetLen < (self.MAX_MESSAGE_SIZE - 2))):
                    TargetLen = TargetLen + 1
                    LastChar = AccessMessageLineEntry["ResidualChar"][0]
                    if AccessMessageLineEntry["ResidualLen"] > 0:
                        AccessMessageLineEntry["ResidualChar"] = AccessMessageLineEntry["ResidualChar"][1:]
                        AccessMessageLineEntry["ResidualLen"] -= 1

                    if AccessMessageLineEntry["Message"] == 0:
                        AccessMessageLineEntry["Message"] = LastChar
                    else:
                        AccessMessageLineEntry["Message"] += LastChar

                    AccessMessageLineEntry["MessageLen"] = len(AccessMessageLineEntry["Message"])

                if LastChar == '\n':
                    break

                if TargetLen >= (self.MAX_MESSAGE_SIZE - 2):
                    AccessMessageLineEntry["Message"]
                    TargetLen = TargetLen + 1
                    break

                if AccessMessageLineEntry["ResidualLen"] != 0:
                    Status = self.ABORTED
                    break

            (Status, AccessMessageBlock) = self._GetNextMessageBlock(LoggerInfo)

            if Status == self.END_OF_FILE:
                if (TargetLen > 0):
                    Status = self.SUCCESS
                break

            if Status == self.SUCCESS:
                AccessMessageLineEntry["ResidualLen"] = AccessMessageBlock["MessageLen"]
                AccessMessageLineEntry["ResidualChar"] = AccessMessageBlock["Message"]

        if Status == self.SUCCESS:
            AccessMessageLineEntry["TimeStamp"] = AccessMessageBlock["TimeStamp"]
            AccessMessageLineEntry["DebugLevel"] = AccessMessageBlock["DebugLevel"]

        return (Status, AccessMessageLineEntry)

    #
    #   Convert Ticks to approximate time based of Frequency setting
    #
    def _GetTimeInNanoSecond(self, Ticks):
        Frequency = int(self.Frequency)
        NanoSeconds = (Ticks // Frequency) * 1000000000
        Remainder = int(Ticks % Frequency)
        BitLength = Remainder.bit_length()

        ##
        ## Ensure (Remainder * 1,000,000,000) will not overflow 64-bit.
        ## Since 2^29 < 1,000,000,000 = 0x3B9ACA00 < 2^30, Remainder should < 2^(64-30) = 2^34,
        ## i.e. highest bit set in Remainder should <= 33.
        ##
        Shift = max((BitLength - 1 - 33), 0)
        Remainder = Remainder >> Shift
        Frequency = Frequency >> Shift
        NanoSeconds += (Remainder * 1000000000) // Frequency

        return NanoSeconds

    #
    #   Get the formatted timestamp
    #
    def _GetTimeStamp(self, Ticks):
        Temp = self._GetTimeInNanoSecond(Ticks)

        Temp = Temp // (1000 * 1000)
        Hours = Temp // (1000 * 60 * 60)
        Temp = Temp % (1000 * 60 * 60)
        Minutes = Temp // (1000 * 60)
        Temp = Temp % (1000 * 60)
        Seconds = Temp // 1000
        Milliseconds = Temp % 1000

        # define ADV_TIME_STAMP_FORMAT "%2d:%2.2d:%2.2d.%3.3d "

        Timestamp = f"{Hours:02}:{Minutes:02}:{Seconds:02}:{Milliseconds:03} "

        return Timestamp

    #
    #   Get all of the formated message lines
    #
    def _GetLines(self, Messages, LoggerInfo):
        lines = []
        Status = self.SUCCESS
        MessageLine = None

        while (Status == self.SUCCESS):
            (Status, MessageLine) = self._GetNextFormattedLine(MessageLine, LoggerInfo)
            if Status != self.SUCCESS:
                break

            if self.CurrentLine >= self.StartLine:
                Ticks = MessageLine["TimeStamp"]
                NewLine = self._GetTimeStamp(Ticks) + MessageLine["Message"]
                lines.append(NewLine.rstrip() + '\n')

            self.CurrentLine += 1

        return lines

    # ----------------------------------------------------------------------- #
    #
    # External Interfaces
    #
    # ----------------------------------------------------------------------- #

    # ----------------------------------------------------------------------- #
    #
    # ProcessMessages - Process the message buffer
    #
    # ----------------------------------------------------------------------- #
    def ProcessMessages(self, InFile, StartLine, ProcessorFrequency):
        self.StartLine = StartLine
        self.Frequency = ProcessorFrequency * 1000000000

        LoggerInfo = self._InitializeLoggerInfo(InFile)
        Messages = None
        lines = self._GetLines(Messages, LoggerInfo)

        return lines


# ------------------------------------------------- ------------------------- #
#
#   Read the complete in memory log and write it to a temporary file.
#
# ---------------------------------------------- ---------------------------- #
def ReadLogFromUefiInterface():
    if not shell.IsUserAnAdmin():
        print("""DecodeUefiLog is not running as an administrator. Please run
                 DecodeUefiLog in an administrator command prompt.""")
        raise SystemExit(1)

    InFile = tempfile.TemporaryFile()
    Index = 0
    UefiVar = UefiVariable()
    rc = 0

    while rc == 0:
        VariableName = 'V'+str(Index)
        (rc, var, errorstring) = UefiVar.GetUefiVar(VariableName, 'a021bf2b-34ed-4a98-859c-420ef94f3e94')
        if (rc == 0):
            Index += 1
            InFile.write(var)

    InFile.seek(0)

    return InFile


# --------------------------------------------------------------------------- #
#
#   Main processing for DecodeUefiLog
#
# --------------------------------------------------------------------------- #
def main():
    print("Reading the Advanced Logger log")

    parser = argparse.ArgumentParser(description="""Copy AdvancedLogger in memory log to a file""")

    parser.add_argument("-l",  "--LogFile", dest="LogFilePath", default=None,
                        help="""Path to binary LogFile. If not specified, obtain the
                              Advanced Logger in memory log from UEFI""")
    parser.add_argument("-o",  "--OutFile", dest="OutFilePath", default=None,
                        help="Path to Output LogFile")
    parser.add_argument("-s",  "--StartLine", dest="StartLine", default=0, type=int,
                        help="Print starting at StartLine")
    parser.add_argument("-f",  "--Frequency", dest="Frequency", default=2.5, type=float,
                        help="Processor frequency in GHz")

    options = parser.parse_args()

    # if we don't have a log file, read it in from memory
    if options.LogFilePath is None:
        InFile = ReadLogFromUefiInterface()
        if InFile is None:
            raise Exception('Unable to get log from system memory')
    else:
        InFile = open(options.LogFilePath, "rb")

    advlog = AdvLogParser()

    lines = advlog.ProcessMessages(InFile,
                                   options.StartLine,
                                   options.Frequency)

    if options.OutFilePath is not None:
        OutFile = open(options.OutFilePath, "w", newline=None)
        OutFile.writelines(lines)
        OutFile.close()

    InFile.close()

    print("Log complete")


# --------------------------------------------------------------------------- #
#
#   Entry point
#
# --------------------------------------------------------------------------- #
if __name__ == '__main__':

    main()
