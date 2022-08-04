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

/**
  Validates a FHR firmware data block.

  @param[in]  FhrFwData     The firmware data block to be validated.

  @retval   EFI_SUCCESS               The data block was successfully validated.
  @retval   RETURN_INVALID_PARAMETER  Failed to validated data block.
**/
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

  if (FhrFwData->HeaderSize == 0) {
    DEBUG ((DEBUG_ERROR, "[FHR] Invalid firmware header size (0x%x)!\n", FhrFwData->HeaderSize));
    return RETURN_INVALID_PARAMETER;
  }

  if ((FhrFwData->Size < FhrFwData->HeaderSize) ||
      (FhrFwData->Size > FHR_MAX_FW_DATA_SIZE))
  {
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

/**
  Recalculates the firmware data block checksum.

  @param[in,out]  FhrFwData     The firmware data block be updated.

  @retval   EFI_SUCCESS               The data block was successfully validated.
  @retval   RETURN_INVALID_PARAMETER  Failed to validated data block.
**/
VOID
EFIAPI
FhrUpdateFwDataChecksum (
  IN OUT FHR_FW_DATA  *FhrFwData
  )
{
  UINT64  Checksum;

  ASSERT ((FhrFwData->Signature == FHR_PAGE_SIGNATURE));
  ASSERT (FhrFwData->HeaderSize > 0);
  ASSERT (FhrFwData->Size >= FhrFwData->HeaderSize);
  ASSERT (FhrFwData->Size <= FHR_MAX_FW_DATA_SIZE);

  FhrFwData->Checksum = 0;
  Checksum            = CalculateCheckSum64 ((VOID *)FhrFwData, FhrFwData->Size);
  FhrFwData->Checksum = Checksum;
  ASSERT (!EFI_ERROR (FhrValidateFwData (FhrFwData)));
}
