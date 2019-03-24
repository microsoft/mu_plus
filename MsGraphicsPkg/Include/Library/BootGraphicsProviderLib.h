/** @file
This BootGraphicsProviderLib provides a method to
supply a requested graphic to the caller

Copyright (c) 2018, Microsoft Corporation.

All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
