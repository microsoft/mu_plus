/** @file
  Null Pei Ms Early Graphics Library.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
