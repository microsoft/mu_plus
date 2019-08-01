/** @file
This BootGraphicsProviderLib provides a method to
supply a requested graphic to the caller

Copyright (C) Microsoft Corporation. All rights reserved..
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BOOT_GRAPHICS_PROVIDER_LIB_H_
#define _BOOT_GRAPHICS_PROVIDER_LIB_H_

#include <Library/BootGraphicsLib.h>

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
  );


/**
  Get the pixel color for the background

**/
UINT32
EFIAPI
GetBackgroundColor();


#endif
