/** @file -- HeapGuardTestCommon.h
Shared definitions between the DXE and SMM drivers.
Used for context, communication to SMM, and to build the tests.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _HEAP_GUARD_TEST_COMMON_H_
#define _HEAP_GUARD_TEST_COMMON_H_

#pragma pack(1)


CHAR8* MEMORY_TYPES[] = {"ReservedMemoryType", "LoaderCode", "LoaderData", "BootServicesCode", "BootServicesData", "RuntimeServicesCode", "RuntimeServicesData", "ConventionalMemory", "UnusableMemory", "ACPIReclaimMemory", "ACPIMemoryNVS", "MemoryMappedIO", "MemoryMappedIOPortSpace", "PalCode", "PersistentMemory"};

typedef struct _HEAP_GUARD_TEST_CONTEXT {
  UINT64                TargetMemoryType;
  UINT64                TestProgress;
} HEAP_GUARD_TEST_CONTEXT;

#define HEAP_GUARD_TEST_POOL              1
#define HEAP_GUARD_TEST_PAGE              2
#define HEAP_GUARD_TEST_NULL_POINTER      3

typedef struct _HEAP_GUARD_TEST_COMM_BUFFER {
  UINT16                    Function;
  HEAP_GUARD_TEST_CONTEXT   Context;
  EFI_STATUS                Status;
} HEAP_GUARD_TEST_COMM_BUFFER;

#pragma pack()

// {F5419493-C44E-4ACC-BD26-D292EFA5A002}
#define HEAP_GUARD_TEST_SMI_HANDLER_GUID \
  { 0xf5419493, 0xc44e, 0x4acc, { 0xbd, 0x26, 0xd2, 0x92, 0xef, 0xa5, 0xa0, 0x2 } };

EFI_GUID  gHeapGuardTestSmiHandlerGuid = HEAP_GUARD_TEST_SMI_HANDLER_GUID;

#define NUM_MEMORY_TYPES 15
#define MAX_STRING_SIZE 0x1000
#define ADDRESS_BITS 0x0000007FFFFFF000ull

STATIC CONST UINT16 mPoolSizeTable[] = {
  128, 256, 384, 640, 1024, 1664, 2688, 4352, 7040, 11392, 18432, 29824, 30000
};

#define NUM_POOL_SIZES 13

#endif // _HEAP_GUARD_TEST_COMMON_H_
