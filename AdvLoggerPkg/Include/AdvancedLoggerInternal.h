/** @file AdvancedLoggerInternal.h

    Advanced Logger internal data structures


    Copyright (C) Microsoft Corporation. All rights reserved.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ADVANCED_LOGGER_INTERNAL_H__
#define __ADVANCED_LOGGER_INTERNAL_H__

#define ADVANCED_LOGGER_SIGNATURE     SIGNATURE_32('A','L','O','G')
#define ADVANCED_LOGGER_VERSION       1

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
    UINT32                Signature;              // Signature 'ALOG'
    UINT16                Version;                // Current Version
    UINT16                Reserved;               // Reserved for future
    EFI_PHYSICAL_ADDRESS  LogBuffer;              // Fixed pointer to start of log
    EFI_PHYSICAL_ADDRESS  LogCurrent;             // Where to store next log entry.
    UINT32                DiscardedSize;          // Number of bytes of messages missed
    UINT32                LogBufferSize;          // Size of allocated buffer
    BOOLEAN               InPermanentRAM;         // Log in permanent RAM
    BOOLEAN               AtRuntime;              // After ExitBootServices
    BOOLEAN               GoneVirtual;            // After VirtualAddressChage
    BOOLEAN               Reserved2[5];           //
    UINT64                TimerFrequency;         // Ticks per second for log timing
} ADVANCED_LOGGER_INFO;

typedef struct {
    UINT32                Signature;              // Signature
    UINT32                DebugLevel;             // Debug Level
    UINT64                TimeStamp;              // Time stamp
    UINT16                MessageLen;             // Number of bytes in Message
    CHAR8                 MessageText[];          // Message Text
} ADVANCED_LOGGER_MESSAGE_ENTRY;

#define MESSAGE_ENTRY_SIZE(LenOfMessage) (ALIGN_VALUE(sizeof(ADVANCED_LOGGER_MESSAGE_ENTRY) + LenOfMessage ,8))

#define NEXT_LOG_ENTRY(LogEntry) ((ADVANCED_LOGGER_MESSAGE_ENTRY *) ((UINTN) LogEntry + MESSAGE_ENTRY_SIZE(LogEntry->MessageLen)))

#define MESSAGE_ENTRY_SIGNATURE SIGNATURE_32('A','L','M','S')

#define MESSAGE_ENTRY_FROM_MSG(a)  BASE_CR (a, ADVANCED_LOGGER_MESSAGE_ENTRY, MessageText)

//
//  Insure the size of is a multiple of 8 bytes
//
STATIC_ASSERT (sizeof(ADVANCED_LOGGER_INFO) % 8 == 0, "Logger Info Misaligned" );

#pragma pack (pop)

//
// Access methods to convert between EFI_PHYSICAL_ADDRESS and UINT64 or CHAR8*
//
#define UINT64_FROM_PA(Address) ((UINT64) (UINTN) (Address))
#define ALI_FROM_PA(Address) ((ADVANCED_LOGGER_INFO *) (UINTN) (Address))
#define CHAR8_FROM_PA(Address) ((CHAR8 *) (UINTN) (Address))

#define PA_FROM_PTR(Address) ((EFI_PHYSICAL_ADDRESS) (UINTN) (Address))
#define PTR_FROM_PA(Address) ((VOID *) (UINTN) (Address))

//
// Log Buffer Base PCD points to this structure.  This is also the structure of the
// Advanced Logger HOB.
//
typedef struct {
    EFI_PHYSICAL_ADDRESS    LoggerInfo;
} ADVANCED_LOGGER_PTR;

//
// Bit flags for PcdAdvancedSerialDisable
//
#define ADV_PCD_DISABLE_SERIAL_FLAGS_NEVER                  0x00
#define ADV_PCD_DISABLE_SERIAL_FLAGS_EXIT_BOOT_SERVICES     0x02
#define ADV_PCD_DISABLE_SERIAL_FLAGS_VIRTUAL_ADDRESS_CHANGE 0x04

//
// Bit flags for PcdAdvancedFileLoggerFlush
//
#define ADV_PCD_FLUSH_TO_MEDIA_FLAGS_NEVER                  0x00
#define ADV_PCD_FLUSH_TO_MEDIA_FLAGS_READY_TO_BOOT          0x01
#define ADV_PCD_FLUSH_TO_MEDIA_FLAGS_EXIT_BOOT_SERVICES     0x02

//
// Address of mLoggerInfo block for script access to in memory log
//
#define ADVANCED_LOGGER_LOCATOR_NAME L"AdvLoggerLocator"

extern EFI_GUID gAdvancedLoggerHobGuid;

#endif  // __ADVANCED_LOGGER_INTERNAL_H__
