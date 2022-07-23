/** @file
  Definitions for Firmware Hot Reset.

  Copyright (c) Microsoft Corporation
  PDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FHR_H__
#define __FHR_H__

#include <Uefi.h>

typedef
VOID
(EFIAPI *OS_RESET_VECTOR)(
  IN EFI_SYSTEM_TABLE *SystemTable,
  IN VOID *ResetData,
  IN UINT64 ResetDataSize
  );

typedef struct {
  // CHAR16 FriendlyResetTypeString[ANY_SIZE];
  EFI_GUID                PlatformSpecificResetType;
  OS_RESET_VECTOR         ResetVector;
  EFI_PHYSICAL_ADDRESS    ResetData;
  UINT64                  ResetDataSize;
} FHR_RESET_DATA;

#define  FHR_RESET_TYPE_GUID \
  {0xF89E4A82,0xB10B,0x4076,{0xBA,0x0D,0xBB,0xDE,0x70,0xD9,0x50,0x5A}}

#define FHR_PAGE_SIGNATURE  SIGNATURE_64('F', 'H', 'R', 'F', 'W', 'D', 'A', 'T')

#define FHR_IS_RUNTIME_MEMORY(_type)  (\
  (_type == EfiReservedMemoryType) || \
  (_type == EfiRuntimeServicesCode) || \
  (_type == EfiRuntimeServicesData) || \
  (_type == EfiReservedMemoryType) || \
  (_type == EfiMemoryMappedIO) || \
  (_type == EfiMemoryMappedIOPortSpace) || \
  (_type == EfiACPIMemoryNVS) \
  )

#define FHR_MEMORY_TYPE_FLAGS  (MEMORY_TYPE_OEM_RESERVED_MIN | 0x00FC0000)

typedef enum _FHR_STATE {
  FhrNone,
  FhrInitialized,
  FhrReboot,
  FhrFailed
} FHR_STATE;

#define FHR_MAX_FW_DATA_SIZE  (0x8000)
#define FHR_MAX_MEMORY_BINS   (10)

#pragma pack(1)

typedef struct _FHR_MEMORY_BIN {
  EFI_MEMORY_TYPE         Type;
  EFI_PHYSICAL_ADDRESS    BaseAddress;
  UINT32                 NumberOfPages;
} FHR_MEMORY_BIN;

typedef struct _FHR_FW_DATA {
  UINT64            Signature;
  UINT32            HeaderSize;
  UINT32            Size;
  UINT64            Checksum;
  UINT64            FwRegionBase;
  UINT64            FwRegionLength;
  UINT32            MemoryBinCount;
  UINT32            Reserved;
  FHR_MEMORY_BIN    MemoryBins[FHR_MAX_MEMORY_BINS];

  // Memory map collected at cold boot BDS.
  UINT32            MemoryMapOffset;
  UINT32            MemoryMapDescriptorVersion;
  UINT64            MemoryMapSize;
  UINT64            MemoryMapDescriptorSize;
} FHR_FW_DATA;

typedef struct _FHR_HOB {
  BOOLEAN                 IsFhrBoot;
  UINT64                  FhrReservedBase;
  UINT64                  FhrReservedSize;

  //
  // OS provided data. Only valid in FHR boot.
  //

  EFI_PHYSICAL_ADDRESS    ResetVector;
  EFI_PHYSICAL_ADDRESS    ResetData;
  UINT64                  ResetDataSize;
} FHR_HOB;

#pragma pack()

#endif
