/**@file Library to interface with alternate boot variable

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

/**
  Clears the Alternate boot flag
**/
VOID
EFIAPI
ClearAltBoot (
  VOID
)
{
  return;
}

/**
  Set the Alternate boot flag

  @retval  EFI_SUCCESS  Set AltBoot successfully
  @retval  !EFI_SUCCESS Failed to set AltBoot
**/
EFI_STATUS
EFIAPI
SetAltBoot (
  VOID
)
{
  return EFI_SUCCESS;
}
