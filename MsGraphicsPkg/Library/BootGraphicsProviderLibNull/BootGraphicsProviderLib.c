/** @file
This BootGraphicsProviderLib

This is a NULL Instance for compile testing

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/BootGraphicsProviderLib.h>

/**
  Get the requested boot graphic

On success
@retval will be EFI_SUCCESS
    ImageSize will be set to size of allocated buffer
    ImageData will be BootServicesData Allocated memory containing a BMP
     file.  ImageData must be freed by caller.

**/
EFI_STATUS
EFIAPI
GetBootGraphic (
  BOOT_GRAPHIC Graphic,
  OUT UINTN    *ImageSize,
  OUT UINT8    **ImageData
  )
  {
      return EFI_UNSUPPORTED;
  }


/**
  Get the pixel color for the background

**/
UINT32
EFIAPI
GetBackgroundColor()
{
    return 0;
}