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
FhrCheckResetData (
  OUT BOOLEAN         *IsFhr,
  OUT FHR_RESET_DATA  **FhrResetData,
  IN UINTN            DataSize,
  IN VOID             *ResetData
  )

{
  *IsFhr        = FALSE;
  *FhrResetData = NULL;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FhrSetIndicator (
  IN  FHR_RESET_DATA  *FhrResetData
  )
{
  return EFI_SUCCESS;
}
