/** @file
This BootGraphicsLib  is only intended to be used by BDS to draw
and the main boot graphics to the screen.

This is a NULL Instance for compile testing

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/BootGraphicsProviderLib.h>


EFI_STATUS
EFIAPI
DisplayBootGraphic(
    BOOT_GRAPHIC Graphic)
{ 
  return EFI_UNSUPPORTED;
}