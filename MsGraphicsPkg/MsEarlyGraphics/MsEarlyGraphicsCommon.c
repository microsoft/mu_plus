/** @file
  These routines are common between Pei and Dxe versions of MsEarlyGraphics.

  Copyright (c) 2016 - 2018, Microsoft Corporation

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


#include "MsEarlyGraphicsCommon.h"


#define MS_EARLY_GRAPHICS_FONT          MsUiGetFixedFontGlyphs ()
#define MS_EARLY_GRAPHICS_CELL_HEIGHT   MsUiGetFixedFontHeight ()
#define MS_EARLY_GRAPHICS_CELL_WIDTH    MsUiGetFixedFontWidth ()
#define MS_EARLY_GRAPHICS_CELL_ADVANCE  MsUiGetFixedFontMaxAdvance ()

#define BITMAP_LEN_1_BIT(Width, Height)  (((Width) + 7) / 8 * (Height))


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
    OUT UINT8                         **GlyphBlock
    ) {

    UINT8                              *BlockPtr;
    EFI_HII_GIBT_GLYPHS_BLOCK          *BlockGlyphs;
    UINT16                              CharCurrent;
    UINT16                              Length16;
    UINTN                               BufferLen;
    EFI_HII_GLYPH_INFO                 *DefaultCell;


    BlockPtr    = MS_EARLY_GRAPHICS_FONT;
    CharCurrent = 1;
    BufferLen   = 0;
    DefaultCell = NULL;

    while (*BlockPtr != EFI_HII_GIBT_END) {
        switch (*BlockPtr) {
        case EFI_HII_GIBT_DEFAULTS:
            //
            // Collect all default character cell information specified by
            // EFI_HII_GIBT_DEFAULTS.
            //
            //        AsciiPrint("Ignoring GIBT_DEFAULTS\n");
            DefaultCell = &((EFI_HII_GIBT_DEFAULTS_BLOCK *)BlockPtr)->Cell;
            BlockPtr += sizeof(EFI_HII_GIBT_DEFAULTS_BLOCK);
            break;

        case EFI_HII_GIBT_GLYPH_DEFAULT:
            if (DefaultCell == NULL) 
            {
              ASSERT(DefaultCell != NULL);
              return EFI_NOT_FOUND;  //mschange - check with MT.  What is best way to exit this func
            }
            BufferLen = BITMAP_LEN_1_BIT(DefaultCell->Width, DefaultCell->Height);
            if (CharCurrent == CharValue) {
                *GlyphBlock = (UINT8 *)((UINTN)BlockPtr + sizeof(EFI_HII_GIBT_GLYPH_DEFAULT_BLOCK) - sizeof(UINT8));
                *Cell = DefaultCell;
                //  DumpMemory(*GlyphBlock, 16);
                return EFI_SUCCESS;
            }
            CharCurrent++;
            BlockPtr += sizeof(EFI_HII_GIBT_GLYPH_DEFAULT_BLOCK) - sizeof(UINT8) + BufferLen;
            break;

        case EFI_HII_GIBT_GLYPH:
            BlockGlyphs = (EFI_HII_GIBT_GLYPHS_BLOCK *)BlockPtr;
            *Cell = &BlockGlyphs->Cell;
            BufferLen = BITMAP_LEN_1_BIT(BlockGlyphs->Cell.Width, BlockGlyphs->Cell.Height);
            if (CharCurrent == CharValue) {
                *GlyphBlock = (UINT8 *)((UINTN)BlockPtr + sizeof(EFI_HII_GIBT_GLYPH_BLOCK) - sizeof(UINT8));
                *Cell = &BlockGlyphs->Cell;
                //DumpMemory (*GlyphBlock,16);
                return EFI_SUCCESS;
            }
            CharCurrent++;
            BlockPtr += sizeof(EFI_HII_GIBT_GLYPH_BLOCK) - sizeof(UINT8) + BufferLen;
            break;

        case EFI_HII_GIBT_SKIP1:
            CharCurrent = (UINT16)(CharCurrent + (UINT16)(*(BlockPtr + sizeof(EFI_HII_GLYPH_BLOCK))));
            BlockPtr    += sizeof(EFI_HII_GIBT_SKIP1_BLOCK);
            break;

        case EFI_HII_GIBT_SKIP2:
            CopyMem(&Length16, BlockPtr + sizeof(EFI_HII_GLYPH_BLOCK), sizeof(UINT16));
            CharCurrent = (UINT16)(CharCurrent + Length16);
            BlockPtr    += sizeof(EFI_HII_GIBT_SKIP2_BLOCK);
            break;

        default:
            return EFI_NOT_FOUND;
            break;
        }

        if (CharValue < CharCurrent) {
            return EFI_NOT_FOUND;
        }
    }
    return EFI_NOT_FOUND;
}


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
    OUT    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Origin
    ) {

    UINT16                                Xpos;
    UINT16                                Ypos;
    UINT8                                 Data;
    UINT16                                Index;
    UINT16                                YposOffset;
    UINTN                                 OffsetY;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer;

    if (GlyphBuffer == NULL || Cell == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    // Move position to the left-top corner of char.
    //
    BltBuffer  = Origin + Cell->OffsetX - (Cell->OffsetY + Cell->Height) * ImageWidth;
    YposOffset = (UINT16)(BaseLine - (Cell->OffsetY + Cell->Height));

    //
    // The glyph's upper left hand corner pixel is the most significant bit of the
    // first bitmap byte.
    //
    for (Ypos = 0; Ypos < Cell->Height && ((UINTN)(Ypos + YposOffset) < RowHeight); Ypos++) {
        OffsetY = BITMAP_LEN_1_BIT(Cell->Width, Ypos);

        //
        // All bits in these bytes are meaningful.
        //
        for (Xpos = 0; Xpos < Cell->Width / 8; Xpos++) {
            Data  = *(GlyphBuffer + OffsetY + Xpos);
            for (Index = 0; Index < 8 && ((UINTN)(Xpos * 8 + Index + Cell->OffsetX) < RowWidth); Index++) {
                if ((Data & (1 << (8 - Index - 1))) != 0) {
                    BltBuffer[Ypos * ImageWidth + Xpos * 8 + Index] = Foreground;
                } else {
                    BltBuffer[Ypos * ImageWidth + Xpos * 8 + Index] = Background;
                }
            }
        }

        if (Cell->Width % 8 != 0) {
            //
            // There are some padding bits in this byte. Ignore them.
            //
            Data  = *(GlyphBuffer + OffsetY + Xpos);
            for (Index = 0; Index < Cell->Width % 8 && ((UINTN)(Xpos * 8 + Index + Cell->OffsetX) < RowWidth); Index++) {
                if ((Data & (1 << (8 - Index - 1))) != 0) {
                    BltBuffer[Ypos * ImageWidth + Xpos * 8 + Index] = Foreground;
                } else {
                    BltBuffer[Ypos * ImageWidth + Xpos * 8 + Index] = Background;
                }
            }
        } // end of if (Width % 8...)
    } // end of for (Ypos=0...)
    return EFI_SUCCESS;
}

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
    IN  MS_EARLY_GRAPHICS_PROTOCOL          *this,
    IN  CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Image,
    IN  UINT32                               DestinationX,
    IN  UINT32                               DestinationY,
    IN  UINT32                               Width,
    IN  UINT32                               Height
    ) {

    UINT32           Row;
    UINT32          *Dest;
    UINT32          *Src = (UINT32 *)Image;

    this->UpdateFrameBufferBase(this);
//    if (this->Mode->Info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor) {
//        DEBUG((DEBUG_ERROR,__FUNCTION__ " Invalid Pixel formal %d",this->Mode->Info->PixelFormat));
//        return EFI_DEVICE_ERROR;
//    }
// FrameBuffer has to be in low 4GB to work in PEI anyway.  Allw full 64 bit memory address in DXE
    Dest = (UINT32 *)((UINTN)this->Mode->FrameBufferBase +
                      DestinationX *sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) +
                      DestinationY * this->Mode->Info->PixelsPerScanLine * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

    for (Row = 0; Row < Height; Row++) {
        CopyMem(Dest, Src, Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
        Src = Src + Width;
        Dest = Dest + this->Mode->Info->PixelsPerScanLine;
    }
    return EFI_SUCCESS;
}

/**
  SimpleFill

  @param this             Pointer to MS_EARLY_GRAPHICS_PROTOCOL
  @param Image            Pointer to bitmap
  @param DestinationX     X location to display the bitmatp
  @param DestinationY     Y location to display the bitmap
  @param Width            Width of bitmap
  @param Height           Height of bitmap

  @return EFI_STATUS      Blt successful
 */
EFI_STATUS
EFIAPI
SimpleFill(
    IN  MS_EARLY_GRAPHICS_PROTOCOL    *this,
    IN  UINT32                         Color,
    IN  UINT32                         DestinationX,
    IN  UINT32                         DestinationY,
    IN  UINT32                         Width,
    IN  UINT32                         Height
    ) {

    UINT32           Row;
    UINT32          *p;

    this->UpdateFrameBufferBase(this);
//  if (this->Mode->Info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor) {
//        DEBUG((DEBUG_ERROR,__FUNCTION__ " Invalid Pixed formal %d",this->Mode->Info->PixelFormat));
//        return EFI_DEVICE_ERROR;
//    }
// FrameBuffer has to be in low 4GB to work in PEI anyway.
    p = (UINT32 *)((UINTN)this->Mode->FrameBufferBase +
                   DestinationX * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) +
                   DestinationY * this->Mode->Info->PixelsPerScanLine * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

    for (Row = 0; Row < Height; Row++) {
        SetMem32(p, Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL), Color);
        p = p + this->Mode->Info->PixelsPerScanLine;
    }
    return EFI_SUCCESS;
}

/**
 * Print a line at the row specified. There is no line
 * wrapping, and \n and other special characters are not
 * supported.
 * 
 * @param Row               Row to display msg on
 * @param Column            Column to start display of message
 * @param ForegroundColor   Color of text in foreground
 * @param BackgroundColor   Color of background behind text
 * @param Msg               String to display
 * 
 * @retval EFI_SUCCESS      String was written to display
 */
EFI_STATUS
EFIAPI
PrintLn(
    IN  MS_EARLY_GRAPHICS_PROTOCOL      *this,
    IN  UINT32                          Row,
    IN  UINT32                          Column,
    IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ForegroundColor,
    IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   BackgroundColor,
    IN  CONST CHAR8                     *Msg)
{
    EFI_STATUS                      Status;
    OUT EFI_HII_GLYPH_INFO          *Cell;
    OUT UINT8                       *GlyphBlock;
    UINT16                          BaseLine;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BufferPtr;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *ImageBuffer;
  
    this->UpdateFrameBufferBase(this);
    ImageBuffer = AllocatePool (MS_EARLY_GRAPHICS_CELL_HEIGHT * MS_EARLY_GRAPHICS_CELL_ADVANCE * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    if (ImageBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }
  
    Status = EFI_SUCCESS;
    while ('\0' != *Msg) {
        Status = FindGlyph((UINT8)*Msg, &Cell, &GlyphBlock); // Poor man's CHAR8 to CHAR16 conversion
        if (!EFI_ERROR(Status)) {
            BaseLine = Cell->Height + Cell->OffsetY;
            BufferPtr = ImageBuffer + BaseLine * Cell->Width;
            Status = GlyphToBlt(GlyphBlock,
                                ForegroundColor,
                                BackgroundColor,
                                Cell->Width,
                                BaseLine,
                                Cell->Width,
                                Cell->Height,
                                Cell,
                                BufferPtr);
            if (!EFI_ERROR(Status)) {
                Status = SimpleBlt(this,
                                   ImageBuffer,
                                   Column * MS_EARLY_GRAPHICS_CELL_WIDTH,
                                   Row * MS_EARLY_GRAPHICS_CELL_HEIGHT,
                                   Cell->Width,
                                   Cell->Height);
            }
        }
    
        Msg++;
        Column++;
    }
  
    FreePool (ImageBuffer );
    return Status;
}

/**
 * GetCellHeight
 *
 *
 * @return UINT32
 */
UINT32
EFIAPI
GetCellHeight() {
    return MS_EARLY_GRAPHICS_CELL_HEIGHT;
}

/**
 * GetCellWidth
 *
 *
 * @return UINT32
 */
UINT32
EFIAPI
GetCellWidth() {
    return MS_EARLY_GRAPHICS_CELL_WIDTH;
}

