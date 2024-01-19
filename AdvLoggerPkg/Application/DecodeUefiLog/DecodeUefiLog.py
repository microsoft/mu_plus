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

    # The dictionary entries for LoggerInfo is based on the UEFI structure.  Each version
    # has its own structure. The current structure is defined in
    # AdvLogger\Include\AdvancedLoggerInternal.h.  The structure as it was, or is, is included
    # here for reference.

    # Developement Log Version (pre versioning)
    #
    # typedef volatile struct {
    #     UINT32                Signature;
    #     UINT32                LogBufferSize;
    #     EFI_PHYSICAL_ADDRESS  LogBuffer;              // Fixed pointer to start of log
    #     EFI_PHYSICAL_ADDRESS  LogCurrent;             // Where to store next log entry.
    #     UINT32                DiscardedSize;          // Number of bytes of messages missed
    #     BOOLEAN               SerialInitialized;      // Serial port initialized
    #     BOOLEAN               InPermanentRAM;         // Log in permanent RAM
    #     BOOLEAN               ExitBootServices;       // Exit Boot Services occurred
    #     BOOLEAN               PeiAllocated;           // Pei allocated "Temp Ram"
    # } ADVANCED_LOGGER_INFO;
    #
    #
    #  Heuristic for V1 decoding:
    #   1. Version will appear as 0 as it is in the Low order 16 bits for the LogBufferSize.
    #   2. LogBuffer will be a constant value of BaseAddress + 32
    V0_LOGGER_INFO_SIZE = 32
    V0_LOGGER_INFO_VERSION = 0

    # First supported Log Version with versioning.
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
    #        UINT32                LogBufferSize;          // Size of allocated buffer
    #        BOOLEAN               InPermanentRAM;         // Log in permanent RAM
    #        BOOLEAN               AtRuntime;              // After ExitBootServices
    #        BOOLEAN               GoneVirtual;            // After VirtualAddressChange
    #        BOOLEAN               Reserved2[5];           //
    #        UINT64                TimerFrequency;         // Ticks per second for log timing
    #    } ADVANCED_LOGGER_INFO;
    #
    V1_LOGGER_INFO_SIZE = 48
    V1_LOGGER_INFO_VERSION = 1

    # ---------------------------------------------------------------------- #
    # typedef volatile struct {
    #     UINT32                Signature;           4  // Signature 'ALOG'
    #     UINT16                Version;             6  // Current Version
    #     UINT16                Reserved;            8  // Reserved for future
    #     EFI_PHYSICAL_ADDRESS  LogBuffer;          16  // Fixed pointer to start of log
    #     EFI_PHYSICAL_ADDRESS  LogCurrent;         24  // Where to store next log entry.
    #     UINT32                DiscardedSize;      28  // Number of bytes of messages missed
    #     UINT32                LogBufferSize;      32  // Size of allocated buffer
    #     BOOLEAN               InPermanentRAM;     33  // Log in permanent RAM
    #     BOOLEAN               AtRuntime;          34  // After ExitBootServices
    #     BOOLEAN               GoneVirtual;        35  // After VirtualAddressChage
    #     BOOLEAN               HdwPortInitialized; 36  // HdwPort initialized
    #     BOOLEAN               HdwPortDisabled;    37  // HdwPort is Disabled
    #     BOOLEAN               Reserved2[3];       40  //
    #     UINT64                TimerFrequency;     48  // Ticks per second for log timing
    #     UINT64                TicksAtTime;        56  // Ticks when Time Acquired
    #     EFI_TIME              Time;               72  // Uefi Time Field
    # } ADVANCED_LOGGER_INFO;
    V2_LOGGER_INFO_SIZE = 72
    V2_LOGGER_INFO_VERSION = 2

    # typedef volatile struct {
    # UINT32                  Signature;              // Signature 'ALOG'
    # UINT16                  Version;                // Current Version
    # UINT16                  Reserved;               // Reserved for future
    # EFI_PHYSICAL_ADDRESS    LogBuffer;              // Fixed pointer to start of log
    # EFI_PHYSICAL_ADDRESS    LogCurrent;             // Where to store next log entry.
    # UINT32                  DiscardedSize;          // Number of bytes of messages missed
    # UINT32                  LogBufferSize;          // Size of allocated buffer
    # BOOLEAN                 InPermanentRAM;         // Log in permanent RAM
    # BOOLEAN                 AtRuntime;              // After ExitBootServices
    # BOOLEAN                 GoneVirtual;            // After VirtualAddressChange
    # BOOLEAN                 HdwPortInitialized;     // HdwPort initialized
    # BOOLEAN                 HdwPortDisabled;        // HdwPort is Disabled
    # BOOLEAN                 Reserved2[3];           //
    # UINT64                  TimerFrequency;         // Ticks per second for log timing
    # UINT64                  TicksAtTime;            // Ticks when Time Acquired
    # EFI_TIME                Time;                   // Uefi Time Field
    # UINT32                  HwPrintLevel;           // Logging level to be printed at hw port
    # UINT32                  Reserved3;              //
    # } ADVANCED_LOGGER_INFO;
    V3_LOGGER_INFO_SIZE = 80
    V3_LOGGER_INFO_VERSION = 3

    # V4 and V3 are the same definition, just an indicator for single firmware systems that
    # starting from V4, all the message will have v2 message entry format.
    V4_LOGGER_INFO_SIZE = V3_LOGGER_INFO_SIZE
    V4_LOGGER_INFO_VERSION = 4

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
    # ---------------------------------------------------------------------- #
    #
    #
    # typedef struct {
    #   UINT32    Signature;                            // Signature
    #   UINT8     MajorVersion;                         // Major version of advanced logger message structure
    #   UINT8     MinorVersion;                         // Minor version of advanced logger message structure
    #   UINT32    DebugLevel;                           // Debug Level
    #   UINT64    TimeStamp;                            // Time stamp
    #   UINT16    Phase;                                // Boot phase that produced this message entry
    #   UINT16    MessageLen;                           // Number of bytes in Message
    #   UINT16    MessageOffset;                        // Offset of Message from start of structure,
    #                                                   // used to calculate the address of the Message
    #   CHAR8     MessageText[];                        // Message Text
    # } ADVANCED_LOGGER_MESSAGE_ENTRY_V2;
    #
    #
    MESSAGE_ENTRY_SIZE_V2 = 24
    ADVANCED_LOGGER_PHASE_UNSPECIFIED = 0
    ADVANCED_LOGGER_PHASE_SEC = 1
    ADVANCED_LOGGER_PHASE_PEI = 2
    ADVANCED_LOGGER_PHASE_PEI64 = 3
    ADVANCED_LOGGER_PHASE_DXE = 4
    ADVANCED_LOGGER_PHASE_RUNTIME = 5
    ADVANCED_LOGGER_PHASE_MM_CORE = 6
    ADVANCED_LOGGER_PHASE_MM = 7
    ADVANCED_LOGGER_PHASE_SMM_CORE = 8
    ADVANCED_LOGGER_PHASE_SMM = 9
    ADVANCED_LOGGER_PHASE_TFA = 10
    ADVANCED_LOGGER_PHASE_CNT = 11
    PHASE_STRING_LIST = ["[UNSPECIFIED] ", "[SEC] ", "[PEI] ", "[PEI64] ",
                         "[DXE] ", "[RUNTIME] ", "[MM_CORE] ", "[MM] ",
                         "[SMM_CORE] ", "[SMM] ", "[TFA] "]
    #
    # ---------------------------------------------------------------------- #
    #
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
    # Global Constants
    #
    # ---------------------------------------------------------------------- #
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
    def _Compute_Basetime(self, LoggerInfo):
        TicksAtTime = LoggerInfo["TicksAtTime"]
        if (0 == TicksAtTime):
            print("TicksAtTime is 0, real time is disabled")
        else:
            Hours = LoggerInfo["Hour"]
            Minutes = LoggerInfo["Minute"]
            Seconds = LoggerInfo["Second"]
            Nanoseconds = LoggerInfo["Nanosecond"]

            Temp = Hours * 60                # Hours to Minutes
            Temp = (Temp + Minutes) * 60     # Hours + Minutes to Seconds
            Temp += Seconds
            Temp *= 1000000000               # Hours/Minutes/Seconds to Nanoseconds
            Nanoseconds += Temp

            TimeInTicks = self._GetTimeInTicks(Nanoseconds, LoggerInfo["Frequency"])
            LoggerInfo["BaseTime"] = TimeInTicks - TicksAtTime

    def _InitializeLoggerInfo(self, InFile, StartLine):
        ''' Initialize log Header '''
        LoggerInfo = {}
        LoggerInfo["StartLine"] = StartLine
        LoggerInfo["CurrentLine"] = 0
        LoggerInfo["Signature"] = InFile.read(4).decode('utf-8', 'replace')
        Version = struct.unpack("=H", InFile.read(2))[0]
        LoggerInfo["Version"] = Version
        LoggerInfo["BaseTime"] = 0
        InFile.read(2)[0]           # Skip reserved field

        if LoggerInfo["Signature"] != "ALOG":
            raise Exception('Error initializing logger info. Invalid signature: %s' % LoggerInfo["Version"])

        if Version == self.V0_LOGGER_INFO_VERSION:
            InFile.seek(4)
            LoggerInfo["LogBufferSize"] = struct.unpack("=I", InFile.read(4))[0]
            BaseAddress = struct.unpack("=Q", InFile.read(8))[0] + self.V0_LOGGER_INFO_SIZE
            LoggerInfo["LogBuffer"] = self.V0_LOGGER_INFO_SIZE
            LoggerInfo["LogCurrent"] = struct.unpack("=Q", InFile.read(8))[0]
            LoggerInfo["DiscardedSize"] = struct.unpack("=I", InFile.read(4))[0]
            LoggerInfo["SerialInitialized"] = struct.unpack("=B", InFile.read(1))[0]
            LoggerInfo["InPermanentRAM"] = struct.unpack("=B", InFile.read(1))[0]
            LoggerInfo["ExitBootServices"] = struct.unpack("=B", InFile.read(1))[0]
            LoggerInfo["PeiAllocated"] = struct.unpack("=B", InFile.read(1))[0]
            LoggerInfo["LogCurrent"] += self.V0_LOGGER_INFO_SIZE
            LoggerInfo["Frequency"] = int(2.5e9)
            if InFile.tell() != (self.V0_LOGGER_INFO_SIZE):
                raise Exception('Error initializing logger info. AmountRead: %d' % InFile.tell())

        elif Version == self.V1_LOGGER_INFO_VERSION:
            BaseAddress = struct.unpack("=Q", InFile.read(8))[0] + self.V1_LOGGER_INFO_SIZE
            LoggerInfo["LogBuffer"] = self.V1_LOGGER_INFO_SIZE
            LoggerInfo["LogCurrent"] = struct.unpack("=Q", InFile.read(8))[0]
            LoggerInfo["DiscardedSize"] = struct.unpack("=I", InFile.read(4))[0]
            LoggerInfo["LogBufferSize"] = struct.unpack("=I", InFile.read(4))[0]
            LoggerInfo["InPermanentRAM"] = struct.unpack("=B", InFile.read(1))[0]
            LoggerInfo["AtRuntime"] = struct.unpack("=B", InFile.read(1))[0]
            LoggerInfo["GoneVirtual"] = struct.unpack("=B", InFile.read(1))[0]
            InFile.read(5)                 # skip reserved2 field
            LoggerInfo["Frequency"] = struct.unpack("=Q", InFile.read(8))[0]
            LoggerInfo["LogCurrent"] += self.V1_LOGGER_INFO_SIZE
            if InFile.tell() != (self.V1_LOGGER_INFO_SIZE):
                raise Exception('Error initializing logger info. AmountRead: %d' % InFile.tell())

        elif Version == self.V2_LOGGER_INFO_VERSION or Version == self.V3_LOGGER_INFO_VERSION or Version == self.V4_LOGGER_INFO_VERSION:
            Size = self.V2_LOGGER_INFO_SIZE if Version == self.V2_LOGGER_INFO_VERSION else self.V3_LOGGER_INFO_SIZE
            BaseAddress = struct.unpack("=Q", InFile.read(8))[0] + Size
            LoggerInfo["LogBuffer"] = Size
            LoggerInfo["LogCurrent"] = struct.unpack("=Q", InFile.read(8))[0]
            LoggerInfo["DiscardedSize"] = struct.unpack("=I", InFile.read(4))[0]
            LoggerInfo["LogBufferSize"] = struct.unpack("=I", InFile.read(4))[0]
            LoggerInfo["InPermanentRAM"] = struct.unpack("=B", InFile.read(1))[0]
            LoggerInfo["AtRuntime"] = struct.unpack("=B", InFile.read(1))[0]
            LoggerInfo["GoneVirtual"] = struct.unpack("=B", InFile.read(1))[0]
            LoggerInfo["HdwInitialized"] = struct.unpack("=B", InFile.read(1))[0]
            LoggerInfo["HdwDisabled"] = struct.unpack("=B", InFile.read(1))[0]
            InFile.read(3)                 # skip reserved2 field
            LoggerInfo["Frequency"] = struct.unpack("=Q", InFile.read(8))[0]
            LoggerInfo["TicksAtTime"] = struct.unpack("=Q", InFile.read(8))[0]

            # Reading in the EFI_TIME structure
            LoggerInfo["Year"] = struct.unpack("=H", InFile.read(2))[0]
            LoggerInfo["Month"] = struct.unpack("=B", InFile.read(1))[0]
            LoggerInfo["Day"] = struct.unpack("=B", InFile.read(1))[0]
            LoggerInfo["Hour"] = struct.unpack("=B", InFile.read(1))[0]
            LoggerInfo["Minute"] = struct.unpack("=B", InFile.read(1))[0]
            LoggerInfo["Second"] = struct.unpack("=B", InFile.read(1))[0]
            InFile.read(1)                 # skip Pad1 field
            LoggerInfo["Nanosecond"] = struct.unpack("=I", InFile.read(4))[0]
            LoggerInfo["TimeZone"] = struct.unpack("=H", InFile.read(2))[0]
            LoggerInfo["DayLight"] = struct.unpack("=B", InFile.read(1))[0]
            InFile.read(1)                 # skip Pad2 field

            # If at v3, there will be 8 bytes for print level and pads, which we do not care.
            if Version == self.V3_LOGGER_INFO_VERSION or Version == self.V4_LOGGER_INFO_VERSION:
                InFile.read(4)
                InFile.read(4)

            self._Compute_Basetime(LoggerInfo)

            LoggerInfo["LogCurrent"] += Size
            if InFile.tell() != (Size):
                raise Exception('Error initializing logger info. AmountRead: %d' % InFile.tell())

        else:
            raise Exception('Error initializing logger info. Unsupported version: 0x%X' % LoggerInfo["Version"])

        LoggerInfo["BaseAddress"] = BaseAddress
        LoggerInfo["LogCurrent"] -= BaseAddress
        LoggerInfo["InFile"] = InFile

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
        EntryStart = InFile.tell()
        MessageEntry["Signature"] = InFile.read(4).decode('utf-8', 'replace')

        if (MessageEntry["Signature"] == 'ALMS'):
            MessageEntry["DebugLevel"] = struct.unpack("=I", InFile.read(4))[0]
            MessageEntry["TimeStamp"] = struct.unpack("=Q", InFile.read(8))[0]
            MessageEntry["MessageLen"] = struct.unpack("=H", InFile.read(2))[0]
            MessageEntry["MessageText"] = InFile.read(MessageEntry["MessageLen"]).decode('utf-8', 'replace')
            MessageEntry["Phase"] = self.ADVANCED_LOGGER_PHASE_UNSPECIFIED
        elif (MessageEntry["Signature"] == 'ALM2'):
            MessageEntry["MajorVersion"] = struct.unpack("=B", InFile.read(1))[0]
            MessageEntry["MinorVersion"] = struct.unpack("=B", InFile.read(1))[0]
            MessageEntry["DebugLevel"] = struct.unpack("=I", InFile.read(4))[0]
            MessageEntry["TimeStamp"] = struct.unpack("=Q", InFile.read(8))[0]
            MessageEntry["Phase"] = struct.unpack("=H", InFile.read(2))[0]
            MessageEntry["MessageLen"] = struct.unpack("=H", InFile.read(2))[0]
            MessageEntry["MessageOffset"] = struct.unpack("=H", InFile.read(2))[0]
            # Offset is from the start of the structure, so we do that from the beginning of this file
            InFile.seek(EntryStart + MessageEntry["MessageOffset"])
            MessageEntry["MessageText"] = InFile.read(MessageEntry["MessageLen"]).decode('utf-8', 'replace')
        else:
            raise Exception("Message Block has wrong signature at offset 0x%X" % InFile.tell())

        Skip = InFile.tell()
        Norm = int((int((Skip + 7) / 8)) * 8)
        Skip = Norm - Skip
        if Skip != 0:
            InFile.read(Skip)

        NextMessage = InFile.tell()
        if MessageEntry["Signature"] == 'ALMS':
            NextMessageLen = self.MESSAGE_ENTRY_SIZE + int(int((MessageEntry["MessageLen"] + 7) / 8) * 8)
        elif MessageEntry["Signature"] == 'ALM2':
            NextMessageLen = MessageEntry["MessageOffset"] + int(int((MessageEntry["MessageLen"] + 7) / 8) * 8)
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

        # if InFile.tell() >= LoggerInfo["LogCurrent"]:
        #     print ("here 1")
        #     return (self.END_OF_FILE, MessageBlock)

        (MessageEntry, NextMessage) = self._ReadMessageEntry(LoggerInfo)

        if MessageEntry["Signature"] != 'ALMS' and MessageEntry["Signature"] != 'ALM2':
            print("Log signature was incorrect.  Should be either 'ALMS' or 'ALM2', was '%s'" % MessageEntry["Signature"])
            raise Exception("Message Block has wrong signature at offset 0x%X" % InFile.tell())

        MessageBlock["Message"] = MessageEntry["MessageText"]
        MessageBlock["DebugLevel"] = MessageEntry["DebugLevel"]
        MessageBlock["MessageLen"] = MessageEntry["MessageLen"]
        MessageBlock["TimeStamp"] = MessageEntry["TimeStamp"]
        MessageBlock["Phase"] = MessageEntry["Phase"]

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
            AccessMessageLineEntry["TimeStamp"] = 0

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
                    AccessMessageLineEntry["Message"] += '\n'
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
                AccessMessageLineEntry["TimeStamp"] = AccessMessageBlock["TimeStamp"]
                AccessMessageLineEntry["DebugLevel"] = AccessMessageBlock["DebugLevel"]
                AccessMessageLineEntry["Phase"] = AccessMessageBlock["Phase"]

        return (Status, AccessMessageLineEntry)

    #
    #   Convert Ticks to approximate time based of Frequency setting
    #
    def _GetTimeInNanoSecond(self, Ticks, Frequency):
        Nanosecond = int((Ticks / Frequency) * 1000000000)

        return Nanosecond

    #
    #   Convert Nanoseconds back to Ticks
    #
    def _GetTimeInTicks(self, Nanosecond, Frequency):
        Ticks = int((Nanosecond * Frequency) / 1000000000)

        return Ticks

    #
    #   Get the formatted timestamp
    #
    def _GetTimeStamp(self, Ticks, Frequency, BaseTime):
        Temp = self._GetTimeInNanoSecond(Ticks + BaseTime, Frequency)

        Temp = Temp // (1000 * 1000)
        Hours = Temp // (1000 * 60 * 60)
        Temp = Temp % (1000 * 60 * 60)
        Minutes = Temp // (1000 * 60)
        Temp = Temp % (1000 * 60)
        Seconds = Temp // 1000
        Milliseconds = Temp % 1000

        # define ADV_TIME_STAMP_FORMAT "%2d:%2.2d:%2.2d.%3.3d "

        Timestamp = f"{Hours:02}:{Minutes:02}:{Seconds:02}.{Milliseconds:03} : "

        return Timestamp

    #
    #   Get the formatted phase string
    #
    def _GetPhaseString(self, Phase):
        if Phase >= self.ADVANCED_LOGGER_PHASE_CNT:
            PhaseString = f"[{Phase:04}] "
        elif Phase <= self.ADVANCED_LOGGER_PHASE_UNSPECIFIED:
            PhaseString = ""
        else:
            PhaseString = self.PHASE_STRING_LIST[Phase]
        return PhaseString

    #
    #   Get all of the formated message lines
    #
    def _GetLines(self, lines, LoggerInfo):
        Status = self.SUCCESS
        MessageLine = None
        CurrentLine = LoggerInfo["CurrentLine"]
        StartLine = LoggerInfo["StartLine"]

        if LoggerInfo["Version"] == self.V4_LOGGER_INFO_VERSION:
            # There is a potential that we may have enabled auto wrap.
            # If so, we need to find the first legible line. Given that
            # we read the logger information from the head of the buffer,
            # we can start from this cursor as an acceptable estiamte.
            LogStart = LoggerInfo["LogCurrent"]
            InFile = LoggerInfo["InFile"]
            CurrentStart = InFile.tell()
            print (f"CurrentStart = {CurrentStart}")
            InFile.seek(LogStart)
            LogStream = InFile.read()
            LogStart = LogStream.find(b'ALM2')
            if LogStart == -1:
                print("DecodeUefiLog unable to find ALM2 signature. Using the beginning as start")
                LogStart = 0
            # We found the first legible line, so we can start from here.
            LoggerInfo["InFile"].seek(LoggerInfo["LogCurrent"] + LogStart)

        while (Status == self.SUCCESS):
            (Status, MessageLine) = self._GetNextFormattedLine(MessageLine, LoggerInfo)
            if Status != self.SUCCESS:
                if Status != self.END_OF_FILE:
                    print(f"Error {Status} from GetNextFormattedLine")
                break

            if CurrentLine >= StartLine:
                Ticks = MessageLine["TimeStamp"]
                PhaseString = self._GetPhaseString(MessageLine["Phase"])
                NewLine = self._GetTimeStamp(Ticks, LoggerInfo["Frequency"], LoggerInfo["BaseTime"]) + PhaseString + MessageLine["Message"].rstrip("\r\n")
                lines.append(NewLine + '\n')

            CurrentLine += 1

        LoggerInfo["CurrentLine"] = CurrentLine

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
    def ProcessMessages(self, InFile, StartLine):
        LoggerInfo = self._InitializeLoggerInfo(InFile, StartLine)

        Year = LoggerInfo["Year"]
        Month = LoggerInfo["Month"]
        Day = LoggerInfo["Day"]
        Hour = LoggerInfo["Hour"]
        Minute = LoggerInfo["Minute"]
        Second = LoggerInfo["Second"]

        DiscardedSize = LoggerInfo["DiscardedSize"]

        Title = f"Log from {Month:2}/{Day:02}/{Year:04} at {Hour:2}:{Minute:02}:{Second:02}\n\n"
        lines = []
        lines.append(Title)

        if (DiscardedSize != 0):
            Title2 = f"The memory space was short by {DiscardedSize} bytes. Some PEI messages are not in the in memory log.\n\n"

            lines.append(Title2)

        self._GetLines(lines, LoggerInfo)

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
        elif (Index == 0):
            print('Error initializing logger. No access to in memory log')
            raise SystemExit(1)
        else:
            print(f"Found {Index} variables worth of log")

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
    parser.add_argument("-r",  "--Raw OutFile", dest="RawFilePath", default=None,
                        help="Path to binary Output LogFile")
    parser.add_argument("-s",  "--StartLine", dest="StartLine", default=0, type=int,
                        help="Print starting at StartLine")

    options = parser.parse_args()

    # if we don't have a log file, read it in from memory
    if options.LogFilePath is None:
        InFile = ReadLogFromUefiInterface()
        if InFile is None:
            raise Exception('Unable to get log from system memory')
    else:
        InFile = open(options.LogFilePath, "rb")

    advlog = AdvLogParser()

    try:
        lines = advlog.ProcessMessages(InFile, options.StartLine)

        if options.OutFilePath is not None:
            OutFile = open(options.OutFilePath, "w", newline=None)
            OutFile.writelines(lines)
            OutFile.close()
            CountOfLines = len(lines)
            print(f"{CountOfLines} lines written to {options.OutFilePath}")

    except Exception as ex:
        print("Error processing log output.")
        traceback.print_exc()

    try:
        if options.RawFilePath is not None:
            RawFile = open(options.RawFilePath, "wb")
            InFile.seek(0)
            RawFile.write(InFile.read())
            RawFile.close()
            print("RawFile complete")

    except Exception as ex:
        print("Error processing raw file output.")
        traceback.print_exc()

    InFile.close()

    print("Log complete")


# --------------------------------------------------------------------------- #
#
#   Entry point
#
# --------------------------------------------------------------------------- #
if __name__ == '__main__':

    main()
