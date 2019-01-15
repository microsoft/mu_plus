/** @file
  Header file for MsEarlyDxeGraphics

  Copyright (c) 2015 - 2018, Microsoft Corporation

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

#ifndef __MS_EARLY_GRAPHICS_COMMON_H__
#define __MS_EARLY_GRAPHICS_COMMON_H__

#include <Uefi.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/MsEarlyGraphics.h>
#include <Protocol/MsUiThemeProtocol.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/MsUiThemeLib.h>

extern MS_UI_THEME_DESCRIPTION   *gPlatformTheme;

/**
 Parse all glyph blocks to find a glyph block specified by CharValue.
 If CharValue = (CHAR16) (-1), collect all default character cell information
 within this font package and backup its information.

 @param  CharValue               Unicode character value, which identifies a glyph
                                 block.
 @param  Cell                    Output cell information of the encoded bitmap.
 @param  GlyphBlock              Pointer to the static Glyph Block.

 @retval EFI_SUCCESS             The bitmap data is retrieved successfully.
 @retval EFI_NOT_FOUND           The specified CharValue does not exist in current
                                 database.
**/
EFI_STATUS
FindGlyph (
    IN  CHAR16                          CharValue,
    OUT EFI_HII_GLYPH_INFO            **Cell,
    OUT UINT8                         **GlyphBlock);

/**
  Convert bitmap data of the glyph to blt structure.

  This is a internal function.

  @param  GlyphBuffer             Buffer points to bitmap data of glyph.
  @param  Foreground              The color of the "on" pixels in the glyph in the
                                  bitmap.
  @param  Background              The color of the "off" pixels in the glyph in the
                                  bitmap.
  @param  ImageWidth              Width of the whole image in pixels.
  @param  BaseLine                BaseLine in the line.
  @param  RowWidth                The width of the text on the line, in pixels.
  @param  RowHeight               The height of the line, in pixels.
  @param  Cell                    Points to EFI_HII_GLYPH_INFO structure.
  @param  Origin                  Points to the origin of the output buffer for the
                                  displayed character.
**/
EFI_STATUS
GlyphToBlt (
    IN     UINT8                         *GlyphBuffer,
    IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Foreground,
    IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Background,
    IN     UINT16                         ImageWidth,
    IN     UINT16                         BaseLine,
    IN     UINT32                         RowWidth,
    IN     UINT32                         RowHeight,
    IN     CONST EFI_HII_GLYPH_INFO      *Cell,
    OUT    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Origin);

/**
    SimpleBlt

@param This             Pointer to MS_EARLY_GRAPHICS_PROTOCOL
@param Image            Pointer to bitmap
@param DestinationX     X location to display the bitmatp
@param DestinationY     Y location to display the bitmap
@param Width            Width of bitmap
@param Height           Height of bitmap

@return EFI_STATUS      Blt successful
 */
EFI_STATUS
EFIAPI
SimpleBlt (
    IN  MS_EARLY_GRAPHICS_PROTOCOL          *This,
    IN  CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Image,
    IN  UINT32                               DestinationX,
    IN  UINT32                               DestinationY,
    IN  UINT32                               Width,
    IN  UINT32                               Height);

/**
  SimpleFill

  @param This             Pointer to MS_EARLY_GRAPHICS_PROTOCOL
  @param Image            Pointer to bitmap
  @param DestinationX     X location to display the bitmatp
  @param DestinationY     Y location to display the bitmap
  @param Width            Width of bitmap
  @param Height           Height of bitmap

  @return EFI_STATUS      Blt successful
 */
EFI_STATUS
EFIAPI
SimpleFill (
    IN  MS_EARLY_GRAPHICS_PROTOCOL    *This,
    IN  UINT32                         Color,
    IN  UINT32                         DestinationX,
    IN  UINT32                         DestinationY,
    IN  UINT32                         Width,
    IN  UINT32                         Height);

/**
*  Print a line at the row specified. There is no line
*  wrapping, and \n and other special characters are not
*  supported.

   @param Row                    Row to display msg on
   @param Column                 Column to start display of message
   @param ForegroundColor        Color of text in foreground
   @param BackgroundColor        Color of background behind text
   @param Msg                    String to display

   @retval EFI_SUCCESS           String was written to display
*/
EFI_STATUS
EFIAPI
PrintLn(
  IN  MS_EARLY_GRAPHICS_PROTOCOL      *this,
  IN  UINT32                          Row,
  IN  UINT32                          Column,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ForegroundColor,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   BackgroundColor,
  IN  CONST CHAR8                     *Msg);

/**
 * GetCellHeight
 *
 *
 * @return UINT32
 */
UINT32
EFIAPI
GetCellHeight();

/**
 * GetCellWidth
 *
 *
 * @return UINT32
 */
UINT32
EFIAPI
GetCellWidth();

#endif // __MS_EARLY_GRAPHICS_COMMON_H__
