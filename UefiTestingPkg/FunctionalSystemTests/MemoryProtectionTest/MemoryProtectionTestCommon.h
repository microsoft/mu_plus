/** @file -- MemoryProtectionTestCommon.h
Shared definitions between the DXE and SMM drivers.
Used for context, communication to SMM, and to build the tests.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MEMORY_PROTECTION_TEST_COMMON_H_
#define _MEMORY_PROTECTION_TEST_COMMON_H_

#pragma pack(1)

CHAR8  *MEMORY_TYPES[] = {
  "ReservedMemoryType",      "LoaderCode",          "LoaderData",          "BootServicesCode",
  "BootServicesData",        "RuntimeServicesCode", "RuntimeServicesData", "ConventionalMemory",
  "UnusableMemory",          "ACPIReclaimMemory",   "ACPIMemoryNVS",       "MemoryMappedIO",
  "MemoryMappedIOPortSpace", "PalCode",             "PersistentMemory",    "EfiUnacceptedMemoryType"
};

STATIC_ASSERT (EfiMaxMemoryType == ARRAY_SIZE (MEMORY_TYPES), "MEMORY_TYPES array size does not match EfiMaxMemoryType");

////
// Reset:                   Test will be run by violating the memory protection policy with the expectation that the system
//                          will reboot each time. The test will take roughly 45 minutes to run with a strict protection policy.
//
// ClearFaults:             Test will be run by violating the memory protection policy with the expectation that the exception
//                          handler will clear the faulting page(s) and allow the test to continue. The test will take roughly
//                          5 seconds to run with a strict protection policy.
//
// MemoryAttributeProtocol: The protection policy will be validated by using the Memory Attribute Protocol to
//                          get the memory attributes of the page(s) which are expected to be protected. The test will take roughly
//                          5 seconds to run with a strict protection policy.
////
typedef enum {
  MemoryProtectionTestReset,
  MemoryProtectionTestClearFaults,
  MemoryProtectionTestMemoryAttributeProtocol,
  MemoryProtectionTestMax
} MEMORY_PROTECTION_TESTING_METHOD;

typedef struct _MEMORY_PROTECTION_TEST_CONTEXT {
  UINT64                              TargetMemoryType;
  UINT64                              TestProgress;
  UINT8                               GuardAlignment;
  MEMORY_PROTECTION_TESTING_METHOD    TestingMethod;
} MEMORY_PROTECTION_TEST_CONTEXT;

#define MEMORY_PROTECTION_TEST_POOL          1
#define MEMORY_PROTECTION_TEST_PAGE          2
#define MEMORY_PROTECTION_TEST_NULL_POINTER  3

typedef struct _MEMORY_PROTECTION_TEST_COMM_BUFFER {
  UINT16                            Function;
  MEMORY_PROTECTION_TEST_CONTEXT    Context;
  EFI_STATUS                        Status;
} MEMORY_PROTECTION_TEST_COMM_BUFFER;

#pragma pack()

// {F5419493-C44E-4ACC-BD26-D292EFA5A002}
#define MEMORY_PROTECTION_TEST_SMI_HANDLER_GUID \
  { 0xf5419493, 0xc44e, 0x4acc, { 0xbd, 0x26, 0xd2, 0x92, 0xef, 0xa5, 0xa0, 0x2 } };

EFI_GUID  gMemoryProtectionTestSmiHandlerGuid = MEMORY_PROTECTION_TEST_SMI_HANDLER_GUID;

STATIC CONST UINT16  mPoolSizeTable[] = {
  128, 256, 384, 640, 1024, 1664, 2688, 4352, 7040, 11392, 18432, 29824, 30000
};

#endif // _MEMORY_PROTECTION_TEST_COMMON_H_
