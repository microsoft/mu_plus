/** @file AdvancedFileLogger.h

  This file contains functions for logging debug print messages to a file.

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ADVANCED_FILE_LOGGER_H__
#define __ADVANCED_FILE_LOGGER_H__

#include <PiDxe.h>

#include <AdvancedLoggerInternal.h>

#include <Guid/EventGroup.h>
#include <Guid/AdvancedFileLoggerPolicy.h>

#include <Protocol/AdvancedLogger.h>
#include <Protocol/DevicePath.h>
#include <Protocol/ResetNotification.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/AdvancedLoggerAccessLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PrintLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PolicyLib.h>

#define LOG_DEVICE_SIGNATURE  SIGNATURE_32('D','L','o','g')

#define LOG_DEVICE_FROM_LINK(a)  CR (a, LOG_DEVICE, Link, LOG_DEVICE_SIGNATURE)

typedef struct {
  UINT32                                       Signature;
  LIST_ENTRY                                   Link;
  EFI_HANDLE                                   Handle;
  UINTN                                        FileIndex;
  UINT64                                       CurrentOffset;         // Current offset to start writing
  ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY    AccessEntry;
  BOOLEAN                                      Valid;
} LOG_DEVICE;

typedef struct {
  CHAR16    *LogFileName;
  UINT64    LogFileSize;
} DEBUG_LOG_FILE_INFO;

// When creating the Index file, create it with Index 0.  This means there
// are no logs with valid data. The sequence of log files will be
// 0->1->2->3->4->5->6->7->8->9->1......

#define INDEX_FILE_VALUE  "0\n"
#define INDEX_FILE_SIZE   (sizeof(INDEX_FILE_VALUE) - 1)

// The code depends on the Chunk size being a multiple of the page size, and
// the log file size being a multiple of the chunk size.

#define DEBUG_LOG_CHUNK_SIZE  (EFI_PAGE_SIZE * 16)                                                   // # of pages per write to log (64KB)
#define DEBUG_LOG_FILE_SIZE   (DEBUG_LOG_CHUNK_SIZE * (FixedPcdGet32 (PcdAdvancedLoggerPages) / 16)) // # chunks per log file

#define LOG_DIRECTORY_NAME  L"\\UefiLogs"

//
// Iterate through the double linked list. NOT delete safe
//
#define EFI_LIST_FOR_EACH(Entry, ListHead)    \
  for(Entry = (ListHead)->ForwardLink; Entry != (ListHead); Entry = Entry->ForwardLink)

//
// Iterate through the double linked list. This is delete-safe.
// Don't touch NextEntry
//
#define EFI_LIST_FOR_EACH_SAFE(Entry, NextEntry, ListHead)            \
  for(Entry = (ListHead)->ForwardLink, NextEntry = Entry->ForwardLink;\
      Entry != (ListHead); Entry = NextEntry, NextEntry = Entry->ForwardLink)

/**
  WriteALogFile

  Writes the currently unwritten part of the log file.

  @param   LogDevice        Which log device to write the log to

  @retval  EFI_SUCCESS      The log was updated
  @retval  other            An error occurred.  The log device was disabled

  **/
EFI_STATUS
WriteALogFile (
  IN LOG_DEVICE  *LogDevice
  );

/**
  Enable Logging on this device

  Validate or create the UEFI Log Files.  10 files are created:

  UEFI_Index.txt
  UEFI_Log1.txt
  UEFI_Log2.txt
  UEFI_Log3.txt
  ...
  UEFI_Log9.txt

  @param[in] FileSystem   Handle where FileSystemProtocol is installed

  @retval   EFI_SUCCESS   All four log files created

  **/
EFI_STATUS
EFIAPI
EnableLoggingOnThisDevice (
  IN  LOG_DEVICE  *LogDevice
  );

#endif // __ADVANCED_FILE_LOGGER_H__
