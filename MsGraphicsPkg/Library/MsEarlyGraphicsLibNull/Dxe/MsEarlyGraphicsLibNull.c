/** @file
  Dxe Ms Early Graphics Library instance

Copyright (c) 2015 - 2018, Microsoft Corporation.

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

#include <Library/BaseLib.h>
#include <Library/MsPlatformEarlyGraphicsLib.h>

/**
 * GetFrameBufferInfo - PEI - Only needs to be called once.
 * GetFrameBufferInfo - DXE - Must be called for every new upper layer call
 *                            as the frame buffer address will change after
 *                            PciBus scan
 *   In PEI mode, this Library is used to get the GOP_MODE or guild
 *   a GOP_MODE from the graphics adapter interface.
 *   In DXE mode, this Library is used to update the FrameBuffer Base.
 *
 *   The MsEarlyGraphics driver abstracts the graphics adapter though a portion
 *   of the GOP protocol - that is, the framebuffer address and the mode information
 *
 *
 * @param GraphicsMode            - Pointer to receive GOP_MODE structure
 *
 * @return EFI_SUCCESS            - FrameBufferInfo returned
 * @return EFI_INVALID_PARAMETERS - GraphicsMide is NULL
 * @return EFI_NOT_FOUND          - Underlying support of early graphics not found
 */
EFI_STATUS
EFIAPI
MsEarlyGraphicsGetFrameBufferInfo(
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE **GraphicsMode
)
{
  return EFI_NOT_FOUND;
}
