/**@file Library to interface with alternate boot variable

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MS_ALT_BOOT_LIB__
#define __MS_ALT_BOOT_LIB__

/**
  Clears the Alternate boot flag
**/
VOID
EFIAPI
ClearAltBoot (
  VOID
);

/**
  Set the Alternate boot flag

  @retval  EFI_SUCCESS  Set AltBoot successfully
  @retval  !EFI_SUCCESS Failed to set AltBoot
**/
EFI_STATUS
EFIAPI
SetAltBoot (
  VOID
);

#endif //__MS_ALT_BOOT_LIB__
