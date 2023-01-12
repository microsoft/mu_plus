/** @file -- DriverDependencyLogging.h

This library and toolset are used with the Core DXE dispatcher to log all DXE drivers' protocol usage and
dependency expression implementation during boot.

See the readme.md file for full information

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DRIVER_DEPENDENCY_LOGGING__H_
#define _DRIVER_DEPENDENCY_LOGGING__H_

// Tag printed at the beginning of every UEFI debug log message
#define DEBUG_TAG  "DEPEX_LOG"

// Debug print level for all messages that provide the dependency logging information through the UEFI log.
#define LOGGING_DEBUG_LEVEL  DEBUG_ERROR

// All logging messages are kept in a buffer that dynamically grows
#define MESSAGE_BUFFER_REALLOC_CHUNK_SZ  0x4000

// Max dependency log string length
#define MESSAGE_ASCII_MAX_STRING_SIZE  128

// Structure to track the message logging buffer
typedef struct _MESSAGE_BUFFER {
  CHAR8    *String;
  CHAR8    *CatPtr;
  UINTN    BufferSize;
} MESSAGE_BUFFER;

// Tags used in the log the python scripts used to find the pertinent data
#define DEPEX_LOG_BEGIN  "DEPEX_LOG_v1_BEGIN\n"
#define DEPEX_LOG_END    "DEPEX_LOG_v1_END\n"

// Structure used to store the protocols used during boot
typedef struct _DL_PROTOCOL_USAGE_ENTRY DL_PROTOCOL_USAGE_ENTRY;
typedef struct _DL_PROTOCOL_USAGE_ENTRY {
  DL_PROTOCOL_USAGE_ENTRY    *Next;       // Pointer to next entry in linked list
  EFI_GUID                   GuidName;    // GUID used in the LocateProtocol call recorded when used in case it resides in an allocated/freed buffer
  UINTN                      GuidAddress; // Address of where the GUID resided in memory
} DL_PROTOCOL_USAGE_ENTRY;

#endif // _DRIVER_DEPENDENCY_LOGGING__H_
