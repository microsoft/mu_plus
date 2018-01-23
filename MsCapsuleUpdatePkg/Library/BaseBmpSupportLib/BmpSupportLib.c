/**

Copyright (c) 2016, Microsoft Corporation<BR>
Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license.  Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.
**/


#include <PiDxe.h>
#include <Protocol/GraphicsOutput.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/IntSafeLib.h>
#include <IndustryStandard/Bmp.h>


/**
Translate a *.BMP graphics image to a GOP blt buffer. If a NULL Blt buffer
is passed in a GopBlt buffer will be allocated by this routine. It is the callers job to call FreePool()
to free the allocated memory.  If a GopBlt buffer is passed in it will be used if it is big enough.

@param  BmpImage      Pointer to BMP file
@param  BmpImageSize  Number of bytes in BmpImage
@param  GopBlt        Buffer containing GOP version of BmpImage.
@param  GopBltSize    Size of GopBlt in bytes.
@param  PixelHeight   Height of GopBlt/BmpImage in pixels
@param  PixelWidth    Width of GopBlt/BmpImage in pixels

@retval EFI_SUCCESS           GopBlt and GopBltSize are returned.
@retval EFI_UNSUPPORTED       BmpImage is not a valid *.BMP image
@retval EFI_BUFFER_TOO_SMALL  The passed in GopBlt buffer is not big enough.
GopBltSize will contain the required size.
@retval EFI_OUT_OF_RESOURCES  No enough buffer to allocate.

**/
RETURN_STATUS
EFIAPI
TranslateBmpToGopBlt(
IN     VOID      *BmpImage,
IN     UINTN     BmpImageSize,
IN OUT VOID      **GopBlt,
IN OUT UINTN     *GopBltSize,
OUT UINTN     *PixelHeight,
OUT UINTN     *PixelWidth
)
{

  UINT8                         *Image;
  UINT8                         *ImageHeader;
  BMP_IMAGE_HEADER              *BmpHeader;
  BMP_COLOR_MAP                 *BmpColorMap;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  UINT32                        BltBufferSize;
  UINTN                         Index;
  UINTN                         Height;
  UINTN                         Width;
  UINTN                         ImageIndex;
  UINT32                        DataSizePerLine;
  BOOLEAN                       IsAllocated;
  UINT32                        ColorMapNum;
  RETURN_STATUS                 Status;
  UINT32                        DataSize;
  UINT32                        Temp;

  if (sizeof(BMP_IMAGE_HEADER) > BmpImageSize) {
    return RETURN_INVALID_PARAMETER;
  }

  BmpHeader = (BMP_IMAGE_HEADER *)BmpImage;

  if (BmpHeader->CharB != 'B' || BmpHeader->CharM != 'M') {
    DEBUG((DEBUG_ERROR, "TranslateBmpToGopBlt: BmpHeader->Char fields incorrect\n"));
    return RETURN_UNSUPPORTED;
  }

  //
  // Doesn't support compress.
  //
  if (BmpHeader->CompressionType != 0) {
    DEBUG((DEBUG_ERROR, "TranslateBmpToGopBlt: Compression Type unsupported.\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // Only support BITMAPINFOHEADER format.
  // BITMAPFILEHEADER + BITMAPINFOHEADER = BMP_IMAGE_HEADER
  //
  if (BmpHeader->HeaderSize != sizeof(BMP_IMAGE_HEADER) - OFFSET_OF(BMP_IMAGE_HEADER, HeaderSize)) {
    DEBUG((DEBUG_ERROR, "TranslateBmpToGopBlt: BmpHeader->Headership is not as expected.  Headersize is 0x%x\n", BmpHeader->HeaderSize));
    return RETURN_UNSUPPORTED;
  }

  //
  // The data size in each line must be 4 byte alignment.
  //
  Status = SafeUInt32Mult(BmpHeader->PixelWidth,
                       BmpHeader->BitPerPixel,
                       &DataSizePerLine);

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR,
      "TranslateBmpToGopBlt: invalid BmpImage... PixelWidth:0x%x BitPerPixel:0x%x\n",
      BmpHeader->PixelWidth, BmpHeader->BitPerPixel));

    return RETURN_INVALID_PARAMETER;
  }

  Status = SafeUInt32Add(DataSizePerLine, 31, &DataSizePerLine);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR,
      "TranslateBmpToGopBlt: invalid BmpImage... DataSizePerLine:0x%x\n",
      DataSizePerLine));

    return RETURN_INVALID_PARAMETER;
  }

  DataSizePerLine = (DataSizePerLine >> 3) & (~0x3);
  Status = SafeUInt32Mult( DataSizePerLine,
                        BmpHeader->PixelHeight,
                        &BltBufferSize);

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR,
      "TranslateBmpToGopBlt: invalid BmpImage... DataSizePerLine:0x%x PixelHeight:0x%x\n",
      DataSizePerLine, BmpHeader->PixelHeight));

    return RETURN_INVALID_PARAMETER;
  }

  Status = SafeUInt32Mult( BmpHeader->PixelHeight,
                        DataSizePerLine,
                        &DataSize);

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR,
      "TranslateBmpToGopBlt: invalid BmpImage... PixelHeight:0x%x DataSizePerLine:0x%x\n",
      BmpHeader->PixelHeight, DataSizePerLine));

    return RETURN_INVALID_PARAMETER;
  }

  if ((BmpHeader->Size != BmpImageSize) ||
    (BmpHeader->Size < BmpHeader->ImageOffset) ||
    (BmpHeader->Size - BmpHeader->ImageOffset != DataSize)) {

    DEBUG((DEBUG_ERROR, "TranslateBmpToGopBlt: invalid BmpImage... \n"));
    DEBUG((DEBUG_ERROR, "   BmpHeader->Size: 0x%x\n", BmpHeader->Size));
    DEBUG((DEBUG_ERROR, "   BmpHeader->ImageOffset: 0x%x\n", BmpHeader->ImageOffset));
    DEBUG((DEBUG_ERROR, "   BmpImageSize: 0x%lx\n", (UINTN)BmpImageSize));
    DEBUG((DEBUG_ERROR, "   DataSize: 0x%lx\n", (UINTN)DataSize));

    return RETURN_INVALID_PARAMETER;
  }

  //
  // Calculate Color Map offset in the image.
  //
  Image = BmpImage;
  BmpColorMap = (BMP_COLOR_MAP *)(Image + sizeof(BMP_IMAGE_HEADER));
  if (BmpHeader->ImageOffset < sizeof(BMP_IMAGE_HEADER)) {
    return RETURN_INVALID_PARAMETER;
  }

  if (BmpHeader->ImageOffset > sizeof(BMP_IMAGE_HEADER)) {
    switch (BmpHeader->BitPerPixel) {
      case 1:
        ColorMapNum = 2;
        break;
      case 4:
        ColorMapNum = 16;
        break;
      case 8:
        ColorMapNum = 256;
        break;
      default:
        ColorMapNum = 0;
        break;
    }
    //
    // BMP file may has padding data between the bmp header section and the bmp data section.
    //
    if (BmpHeader->ImageOffset - sizeof(BMP_IMAGE_HEADER) < sizeof(BMP_COLOR_MAP) * ColorMapNum) {
      return RETURN_INVALID_PARAMETER;
    }
  }

  //
  // Calculate graphics image data address in the image
  //
  Image = ((UINT8 *)BmpImage) + BmpHeader->ImageOffset;
  ImageHeader = Image;

  //
  // Calculate the BltBuffer needed size.
  //
  Status = SafeUInt32Mult(BmpHeader->PixelWidth,
                       BmpHeader->PixelHeight,
                       &BltBufferSize);

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR,
      "TranslateBmpToGopBlt: invalid BltBuffer needed size... PixelWidth:0x%x PixelHeight:0x%x\n",
      BltBufferSize));

    return RETURN_INVALID_PARAMETER;
  }

  Temp = BltBufferSize;
  Status = SafeUInt32Mult( BltBufferSize,
                        sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL),
                        &BltBufferSize);

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR,
      "TranslateBmpToGopBlt: invalid BltBuffer needed size... BltBufferSize:0x%lx struct size:0x%x\n",
      Temp, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)));

    return RETURN_INVALID_PARAMETER;
  }

  IsAllocated = FALSE;
  if (*GopBlt == NULL) {
    //
    // GopBlt is not allocated by caller.
    //
    DEBUG((DEBUG_INFO, "Bmp Support: Allocating 0x%X bytes of memory\n", BltBufferSize));
    *GopBltSize = (UINTN)BltBufferSize;
    *GopBlt = AllocatePool(*GopBltSize);
    IsAllocated = TRUE;
    if (*GopBlt == NULL) {
      return RETURN_OUT_OF_RESOURCES;
    }
  }
  else
  {
    //
    // GopBlt has been allocated by caller.
    //
    if (*GopBltSize < (UINTN)BltBufferSize) {
      *GopBltSize = (UINTN)BltBufferSize;
      return RETURN_BUFFER_TOO_SMALL;
    }
  }

  *PixelWidth = BmpHeader->PixelWidth;
  *PixelHeight = BmpHeader->PixelHeight;
  DEBUG((DEBUG_INFO, "BmpHeader->ImageOffset 0x%X\n", BmpHeader->ImageOffset));
  DEBUG((DEBUG_INFO, "BmpHeader->PixelWidth 0x%X\n", BmpHeader->PixelWidth));
  DEBUG((DEBUG_INFO, "BmpHeader->PixelHeight 0x%X\n", BmpHeader->PixelHeight));
  DEBUG((DEBUG_INFO, "BmpHeader->BitPerPixel 0x%X\n", BmpHeader->BitPerPixel));
  DEBUG((DEBUG_INFO, "BmpHeader->ImageSize 0x%X\n", BmpHeader->ImageSize));
  DEBUG((DEBUG_INFO, "BmpHeader->HeaderSize 0x%X\n", BmpHeader->HeaderSize));
  DEBUG((DEBUG_INFO, "BmpHeader->Size 0x%X\n", BmpHeader->Size));

  //
  // Translate image from BMP to Blt buffer format
  //
  BltBuffer = *GopBlt;
  for (Height = 0; Height < BmpHeader->PixelHeight; Height++) {
    Blt = &BltBuffer[(BmpHeader->PixelHeight - Height - 1) * BmpHeader->PixelWidth];
    for (Width = 0; Width < BmpHeader->PixelWidth; Width++, Image++, Blt++) {
      switch (BmpHeader->BitPerPixel) {
        case 1:
          //
          // Translate 1-bit (2 colors) BMP to 24-bit color
          //
          for (Index = 0; Index < 8 && Width < BmpHeader->PixelWidth; Index++) {
            Blt->Red = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Red;
            Blt->Green = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Green;
            Blt->Blue = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Blue;
            Blt++;
            Width++;
          }

          Blt--;
          Width--;
          break;

        case 4:
          //
          // Translate 4-bit (16 colors) BMP Palette to 24-bit color
          //
          Index = (*Image) >> 4;
          Blt->Red = BmpColorMap[Index].Red;
          Blt->Green = BmpColorMap[Index].Green;
          Blt->Blue = BmpColorMap[Index].Blue;
          if (Width < (BmpHeader->PixelWidth - 1)) {
            Blt++;
            Width++;
            Index = (*Image) & 0x0f;
            Blt->Red = BmpColorMap[Index].Red;
            Blt->Green = BmpColorMap[Index].Green;
            Blt->Blue = BmpColorMap[Index].Blue;
          }
          break;

        case 8:
          //
          // Translate 8-bit (256 colors) BMP Palette to 24-bit color
          //
          Blt->Red = BmpColorMap[*Image].Red;
          Blt->Green = BmpColorMap[*Image].Green;
          Blt->Blue = BmpColorMap[*Image].Blue;
          break;

        case 24:
          //
          // It is 24-bit BMP.
          //
          Blt->Blue = *Image++;
          Blt->Green = *Image++;
          Blt->Red = *Image;
          break;

        case 32:
          //
          //Conver 32 bit to 24bit bmp - just ignore the final byte of each pixel
          Blt->Blue = *Image++;
          Blt->Green = *Image++;
          Blt->Red = *Image++;
          break;

        default:
          //
          // Other bit format BMP is not supported.
          //
          if (IsAllocated) {
            //FreePages(*GopBlt, *MemPages);
            FreePool(*GopBlt);
            *GopBlt = NULL;
          }
          DEBUG((DEBUG_ERROR, "Bmp Bit format not supported.  0x%X\n", BmpHeader->BitPerPixel));
          return RETURN_UNSUPPORTED;
          break;
      };

    }

    ImageIndex = (UINTN)(Image - ImageHeader);
    if ((ImageIndex % 4) != 0) {
      //
      // Bmp Image starts each row on a 32-bit boundary!
      //
      Image = Image + (4 - (ImageIndex % 4));
    }
  }

  return RETURN_SUCCESS;
}
