/** @file -- HeapGuardTestCommon.h
Shared definitions between the DXE and SMM drivers.
Used for context, communication to SMM, and to build the tests.

Copyright (c) 2017, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#ifndef _HEAP_GUARD_TEST_COMMON_H_
#define _HEAP_GUARD_TEST_COMMON_H_

#pragma pack(1)


CHAR16* MEMORY_TYPES[] = {L"ReservedMemoryType", L"LoaderCode", L"LoaderData", L"BootServicesCode", L"BootServicesData", L"RuntimeServicesCode", L"RuntimeServicesData", L"ConventionalMemory", L"UnusableMemory", L"ACPIReclaimMemory", L"ACPIMemoryNVS", L"MemoryMappedIO", L"MemoryMappedIOPortSpace", L"PalCode", L"PersistentMemory"};

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
