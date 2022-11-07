/** @file
  Definitions for Firmware Hot Reset.

  Copyright (c) Microsoft Corporation
  PDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FHR_H__
#define __FHR_H__

#include <Uefi.h>

#pragma pack(1)

//
// Structure used for the FHR platform specific reset.
//
// typedef struct {
//   // CHAR16 FriendlyResetTypeString[ANY_SIZE];
//   EFI_GUID                PlatformSpecificResetType;
//   OS_RESET_VECTOR         ResetVector;
//   EFI_PHYSICAL_ADDRESS    ResetData;
//   UINT64                  ResetDataSize;
// } FHR_RESET_DATA;

#define FHR_RESET_DATA_SIGNATURE   SIGNATURE_32('M', 'P', 'R', 'B')
#define FHR_RESUME_DATA_SIGNATURE  SIGNATURE_32('M', 'P', 'R', 'O')

#define FHR_RESET_DATA_REVISION  1

typedef struct _FHR_RESET_DATA {
  UINT32    Signature;
  UINT32    Length;
  UINT8     Revision;
  UINT8     Checksum;
  UINT8     Reserved0[6];
  UINT64    ResumeCodeBase;
  UINT64    ResumeCodeSize;
  UINT64    OsDataBase;
  UINT64    OsDataSize;
  UINT64    CompatabilityId;
  UINT64    StatusCode;
} FHR_RESET_DATA;

#define FHR_RESUME_DATA_REVISION  1

typedef struct _FHR_RESUME_DATA {
  UINT32    Signature;
  UINT32    Length;
  UINT8     Revision;
  UINT8     Checksum;
  UINT8     Reserved0[6];
  UINT64    ResumeCodeBase;
  UINT64    ResumeCodeSize;
  UINT64    OsDataBase;
  UINT64    OsDataSize;
  UINT64    Flags;
} FHR_RESUME_DATA;

//
// Double check the correct size of FHR structures
//

STATIC_ASSERT (sizeof (FHR_RESET_DATA) == 64, "Invalid FHR reset structure size!");
STATIC_ASSERT (sizeof (FHR_RESUME_DATA) == 56, "Invalid FHR resume structure size!");

#define FHR_ERROR                             0x80000000
#define FHR_ERROR_RESET_BAD_SIGNATURE         (FHR_ERROR | 0x01)
#define FHR_ERROR_RESET_BUFFER_TOO_SMALL      (FHR_ERROR | 0x02)
#define FHR_ERROR_RESET_BAD_CHECKSUM          (FHR_ERROR | 0x03)
#define FHR_ERROR_RESET_UNSUPPORTED_REVISION  (FHR_ERROR | 0x04)

//
// Feature flags for resume data.
//

#define FHR_MEMORY_PRESERVED  0x1

/**
  The re-entry point for the OS after a FHR resume.

  @param[in]  Handle            Unused for FHR.
  @param[in]  SystemTable       Pointer to the updated system table.
  @param[in]  ResetData         The FHR resume data structure.
**/
typedef
VOID
(EFIAPI *OS_RESET_VECTOR)(
  IN EFI_HANDLE        Handle,
  IN EFI_SYSTEM_TABLE  *SystemTable,
  IN FHR_RESUME_DATA   *ResumeData
  );

//
// GUID identifying the FHR platform specific reset.
//
#define  FHR_RESET_TYPE_GUID \
  {0xF89E4A82,0xB10B,0x4076,{0xBA,0x0D,0xBB,0xDE,0x70,0xD9,0x50,0x5A}}

//
// Signature identifying the firmware data in the firmware reserved region.
//
#define FHR_PAGE_SIGNATURE  SIGNATURE_64('F', 'H', 'R', 'F', 'W', 'D', 'A', 'T')

//
// Macro to check if a memory type is runtime memory.
//
#define FHR_IS_RUNTIME_MEMORY(_type)  (\
  (_type == EfiReservedMemoryType) || \
  (_type == EfiRuntimeServicesCode) || \
  (_type == EfiRuntimeServicesData) || \
  (_type == EfiReservedMemoryType) || \
  (_type == EfiMemoryMappedIO) || \
  (_type == EfiMemoryMappedIOPortSpace) || \
  (_type == EfiACPIMemoryNVS) \
  )

//
// Memory type for memory reserved for use by the OS.
//
#define FHR_MEMORY_TYPE_OS_RESERVED  (MEMORY_TYPE_OEM_RESERVED_MIN | 0x00FC0000)

//
// Macros that define FHR structure constants.
//
#define FHR_MAX_FW_DATA_SIZE  (0x8000)
#define FHR_MAX_MEMORY_BINS   (10)

//
// Failure codes for FHR.
//
typedef enum _FHR_FAILURE_REASON {
  FhrFailureNone                 = 0,
  FhrFailurePeiGeneric           = 1,
  FhrFailureDxeGeneric           = 2,
  FhrFailureResGeneric           = 3,
  FhrFailureUnexpectedBootOption = 4,
} FHR_FAILURE_REASON;

typedef struct _FHR_MEMORY_BIN {
  EFI_MEMORY_TYPE         Type;
  EFI_PHYSICAL_ADDRESS    BaseAddress;
  UINT32                  NumberOfPages;
} FHR_MEMORY_BIN;

typedef struct _FHR_FW_DATA {
  UINT64            Signature;
  UINT32            HeaderSize;
  UINT32            Size;
  UINT64            Checksum;
  UINT64            FwRegionBase;
  UINT64            FwRegionLength;
  UINT32            MemoryMapOffset;
  UINT32            MemoryMapDescriptorVersion;
  UINT64            MemoryMapSize;
  UINT64            MemoryMapDescriptorSize;
  UINT32            MemoryBinCount;
  UINT32            Reserved;
  FHR_MEMORY_BIN    MemoryBins[FHR_MAX_MEMORY_BINS];
} FHR_FW_DATA;

typedef struct _FHR_HOB {
  BOOLEAN               IsFhrBoot;
  UINT64                FhrReservedBase;
  UINT64                FhrReservedSize;

  //
  // OS provided data. Only valid in FHR boot.
  //

  FHR_RESET_DATA        ResetData;

  //
  // PEI failures. Only valid in FHR boot. Failures in PEI may not
  // have full persistent capabilities so the failures are persisted
  // to DXE.
  //

  FHR_FAILURE_REASON    PeiFailureReason;
  EFI_STATUS            PeiFailureStatus;
} FHR_HOB;

//
// May be used internally to indicate an FHR boot if the indicator page is used.
//

#define FHR_INDICATOR_SIGNATURE  SIGNATURE_64('F', 'H', 'R', 'R', 'E', 'S', 'U', 'M')

typedef struct _FHR_INDICATOR {
  UINT64     Signature;
  FHR_HOB    FhrHob;
} FHR_INDICATOR;

#pragma pack()

#endif
