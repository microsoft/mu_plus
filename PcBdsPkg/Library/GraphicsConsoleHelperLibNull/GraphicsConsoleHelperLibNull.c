/** @file
This GraphicsConsoleHelper is only intended to be used by BDS to configure 
the console mode / graphic mode

Null Instance which is just for compiling

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/GraphicsConsoleHelperLib.h>

EFI_STATUS
EFIAPI
SetGraphicsConsoleMode(GRAPHICS_CONSOLE_MODE Mode)
{
  return EFI_UNSUPPORTED;
}
