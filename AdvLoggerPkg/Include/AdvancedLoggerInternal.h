/** @file AdvancedLoggerInternal.h

    Advanced Logger internal data structures


    Copyright (C) Microsoft Corporation. All rights reserved.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ADVANCED_LOGGER_INTERNAL_H__
#define __ADVANCED_LOGGER_INTERNAL_H__

#define ADVANCED_LOGGER_SIGNATURE    SIGNATURE_32('A','L','O','G')
#define ADVANCED_LOGGER_HW_LVL_VER   5
#define ADVANCED_LOGGER_MSG_MAJ_VER  2
#define ADVANCED_LOGGER_MSG_MIN_VER  0

#define ADVANCED_LOGGER_VERSION  ADVANCED_LOGGER_HW_LVL_VER

#define ADVANCED_LOGGER_PHASE_UNSPECIFIED  0
#define ADVANCED_LOGGER_PHASE_SEC          1
#define ADVANCED_LOGGER_PHASE_PEI          2
#define ADVANCED_LOGGER_PHASE_PEI64        3
#define ADVANCED_LOGGER_PHASE_DXE          4
#define ADVANCED_LOGGER_PHASE_RUNTIME      5
#define ADVANCED_LOGGER_PHASE_MM_CORE      6
#define ADVANCED_LOGGER_PHASE_MM           7
#define ADVANCED_LOGGER_PHASE_SMM_CORE     8
#define ADVANCED_LOGGER_PHASE_SMM          9
#define ADVANCED_LOGGER_PHASE_TFA          10
#define ADVANCED_LOGGER_PHASE_CNT          11

//
// These Pcds are used to carve out a PEI memory buffer from the temporary RAM.
//
//  PcdAdvancedLoggerBase -        NULL = UEFI starts with PEI, and SEC provides no memory log buffer
//                              Address = UEFI starts with SEC, and SEC provided LogInfoPtr is at this address
//  PcdAdvancedLoggerPreMemPages - Size = Pages to allocate from temporary RAM (SEC or PEI Pre-memory)
//

//
// Logger Info structure
//

#pragma pack (push, 1)

typedef volatile struct {
  UINT32      Signature;                          // Signature 'ALOG'
  UINT16      Version;                            // Current Version
  UINT16      Reserved[3];                        // Reserved for future
  UINT32      LogBufferOffset;                    // Offset from LoggerInfo to start of log, expected to be the size of this structure 8 byte aligned
  UINT32      Reserved4;
  UINT32      LogCurrentOffset;                   // Offset from LoggerInfo to where to store next log entry.
  UINT32      DiscardedSize;                      // Number of bytes of messages missed
  UINT32      LogBufferSize;                      // Size of allocated buffer
  BOOLEAN     InPermanentRAM;                     // Log in permanent RAM
  BOOLEAN     AtRuntime;                          // After ExitBootServices
  BOOLEAN     GoneVirtual;                        // After VirtualAddressChange
  BOOLEAN     HdwPortInitialized;                 // HdwPort initialized
  BOOLEAN     HdwPortDisabled;                    // HdwPort is Disabled
  BOOLEAN     Reserved2[3];                       //
  UINT64      TimerFrequency;                     // Ticks per second for log timing
  UINT64      TicksAtTime;                        // Ticks when Time Acquired
  EFI_TIME    Time;                               // Uefi Time Field
  UINT32      HwPrintLevel;                       // Logging level to be printed at hw port
  UINT32      Reserved3;                          //
} ADVANCED_LOGGER_INFO;

typedef struct {
  UINT32    Signature;                            // Signature
  UINT32    DebugLevel;                           // Debug Level
  UINT64    TimeStamp;                            // Time stamp
  UINT16    MessageLen;                           // Number of bytes in Message
  CHAR8     MessageText[];                        // Message Text
} ADVANCED_LOGGER_MESSAGE_ENTRY;

typedef struct {
  UINT32    Signature;                            // Signature
  UINT8     MajorVersion;                         // Major version of advanced logger message structure
  UINT8     MinorVersion;                         // Minor version of advanced logger message structure
  UINT32    DebugLevel;                           // Debug Level
  UINT64    TimeStamp;                            // Time stamp
  UINT16    Phase;                                // Boot phase that produced this message entry
  UINT16    MessageLen;                           // Number of bytes in Message
  UINT16    MessageOffset;                        // Offset of Message from start of structure,
                                                  // used to calculate the address of the Message
  CHAR8     MessageText[];                        // Message Text
} ADVANCED_LOGGER_MESSAGE_ENTRY_V2;

#define MESSAGE_ENTRY_SIZE(LenOfMessage)                 (ALIGN_VALUE(sizeof(ADVANCED_LOGGER_MESSAGE_ENTRY) + LenOfMessage, 8))
#define MESSAGE_ENTRY_SIZE_V2(LenOfEntry, LenOfMessage)  (ALIGN_VALUE(LenOfEntry + LenOfMessage, 8))

#define NEXT_LOG_ENTRY(LogEntry)       ((ADVANCED_LOGGER_MESSAGE_ENTRY *)((UINTN)LogEntry + MESSAGE_ENTRY_SIZE(LogEntry->MessageLen)))
#define NEXT_LOG_ENTRY_V2(LogEntryV2)  ((ADVANCED_LOGGER_MESSAGE_ENTRY_V2 *)((UINTN)LogEntryV2 + MESSAGE_ENTRY_SIZE_V2(LogEntryV2->MessageOffset, LogEntryV2->MessageLen)))

#define MESSAGE_ENTRY_SIGNATURE     SIGNATURE_32('A','L','M','S')
#define MESSAGE_ENTRY_SIGNATURE_V2  SIGNATURE_32('A','L','M','2')

#define MESSAGE_ENTRY_FROM_MSG(a)             BASE_CR (a, ADVANCED_LOGGER_MESSAGE_ENTRY, MessageText)
#define MESSAGE_ENTRY_FROM_MSG_V2(a, offset)  ((UINTN)a - offset)

//
//  Insure the size of is a multiple of 8 bytes
//
STATIC_ASSERT (sizeof (ADVANCED_LOGGER_INFO) % 8 == 0, "Logger Info Misaligned");

#pragma pack (pop)

//
// Access methods to convert between EFI_PHYSICAL_ADDRESS and UINT64 or CHAR8*
//
#define UINT64_FROM_PA(Address)  ((UINT64) (UINTN) (Address))
#define ALI_FROM_PA(Address)     ((ADVANCED_LOGGER_INFO *) (UINTN) (Address))
#define CHAR8_FROM_PA(Address)   ((CHAR8 *) (UINTN) (Address))

#define PA_FROM_PTR(Address)  ((EFI_PHYSICAL_ADDRESS) (UINTN) (Address))
#define PTR_FROM_PA(Address)  ((VOID *) (UINTN) (Address))

#define LOG_BUFFER_FROM_ALI(LoggerInfo)         ((UINT8 *)LoggerInfo + LoggerInfo->LogBufferOffset)
#define LOG_CURRENT_FROM_ALI(LoggerInfo)        ((UINT8 *)LoggerInfo + LoggerInfo->LogCurrentOffset)
#define USED_LOG_SIZE(LoggerInfo)               (LoggerInfo->LogCurrentOffset < LoggerInfo->LogBufferOffset ? 0 : LoggerInfo->LogCurrentOffset - LoggerInfo->LogBufferOffset)
#define TOTAL_LOG_SIZE_WITH_ALI(LoggerInfo)     (LoggerInfo->LogBufferOffset + LoggerInfo->LogBufferSize)
#define LOG_MAX_ADDRESS(LoggerInfo)             (PA_FROM_PTR ((UINT8 *)LoggerInfo + TOTAL_LOG_SIZE_WITH_ALI (LoggerInfo)))
#define EXPECTED_LOG_BUFFER_OFFSET(LoggerInfo)  (ALIGN_VALUE (sizeof (*LoggerInfo), 8))// 8 byte align the log buffer offset

//
// Log Buffer Base PCD points to this structure.  This is also the structure of the
// Advanced Logger HOB.
//

#define ADVANCED_LOGGER_PTR_SIGNATURE  SIGNATURE_64('A','l','o','g','_','P','t','r')

typedef struct {
  EFI_PHYSICAL_ADDRESS    LogBuffer;
  UINT64                  Signature;                // Signature 'Alog_Ptr'
} ADVANCED_LOGGER_PTR;

//
// Bit flags for PcdAdvancedLoggerHdwDisable
//
#define ADV_PCD_DISABLE_HDW_PORT_FLAGS_NEVER                   0x00
#define ADV_PCD_DISABLE_HDW_PORT_FLAGS_EXIT_BOOT_SERVICES      0x02
#define ADV_PCD_DISABLE_HDW_PORT_FLAGS_VIRTUAL_ADDRESS_CHANGE  0x04

//
// Bit flags for PcdAdvancedFileLoggerFlush
//
#define ADV_PCD_FLUSH_TO_MEDIA_FLAGS_NEVER               0x00
#define ADV_PCD_FLUSH_TO_MEDIA_FLAGS_READY_TO_BOOT       0x01
#define ADV_PCD_FLUSH_TO_MEDIA_FLAGS_EXIT_BOOT_SERVICES  0x02

//
// Address of LoggerInfo block for script access to in memory log
//
#define ADVANCED_LOGGER_LOCATOR_NAME  L"AdvLoggerLocator"

extern EFI_GUID  gAdvancedLoggerHobGuid;

#endif // __ADVANCED_LOGGER_INTERNAL_H__
