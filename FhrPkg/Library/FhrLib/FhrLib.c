/**
  A library for FHR helper functions.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include <Fhr.h>
#include <Library/FhrLib.h>

EFI_STATUS
EFIAPI
FhrValidateFwData (
  IN  FHR_FW_DATA  *FhrFwData
  )
{
  UINT64  OriginalChecksum;
  UINT64  Checksum;

  if (FhrFwData->Signature != FHR_PAGE_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "[FHR] Invalid firmware data signature!\n"));
    return RETURN_INVALID_PARAMETER;
  }

  if (FhrFwData->Size == 0) {
    DEBUG ((DEBUG_ERROR, "[FHR] Invalid firmware data size (0x%x)!\n", FhrFwData->Size));
    return RETURN_INVALID_PARAMETER;
  }

  if (FhrFwData->MemoryMapOffset + FhrFwData->MemoryMapSize > FhrFwData->Size) {
    DEBUG ((
      DEBUG_ERROR,
      "[FHR] Invalid memory map offset or size. Offset: %d Size: %d\n",
      FhrFwData->MemoryMapOffset,
      FhrFwData->MemoryMapSize
      ));
    return RETURN_INVALID_PARAMETER;
  }

  OriginalChecksum    = FhrFwData->Checksum;
  FhrFwData->Checksum = 0;
  Checksum            = CalculateCheckSum64 ((VOID *)FhrFwData, FhrFwData->Size);
  FhrFwData->Checksum = OriginalChecksum;
  if (OriginalChecksum != Checksum) {
    DEBUG ((
      DEBUG_ERROR,
      "[FHR] Invalid firmware data checksum! Expected: 0x%llx Found: 0x%llx\n",
      Checksum,
      OriginalChecksum
      ));

    return RETURN_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}
