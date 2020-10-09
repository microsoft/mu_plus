/**

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <UiPrimitiveSupport.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UiRectangleLib.h>
#include <Library/DebugLib.h>
#include <Protocol/GraphicsOutput.h>  //structure defs
#include <Library/MemoryAllocationLib.h>
#include "UiRectangle.h"

/*
Method to use create a new UI_RECTANGLE struct.
This structure is used by all the other functions to modify and draw the object

@param UpperLeft         - Upper left point of rectangle in framebuffer coordinates
@param FrameBufferBase   - pointer to framebuffer address of 0,0  (upper left)
@param PixelsPerScanLine - Number of pixels per scan line in framebuffer.
                           This is to support aligned framebuffers
@param PixelFormat       - An enum that tells use what format the pixel are in
@param PixelBitMap       - A pointer to the exact layout of the pixels
@param Width             - The width of the rectangle
@param Height            - The height of the rectangle
@param StyleInfo  - Style info for this (color, sizes, fill types, border, etc)

@ret   A new UI_RECTANGLE structure used for updating and drawing the rectangle

*/
UI_RECTANGLE*
EFIAPI
new_UI_RECTANGLE(
IN POINT *UpperLeft,
IN UINT8 *FrameBufferBase,
IN UINTN PixelsPerScanLine,
IN EFI_GRAPHICS_PIXEL_FORMAT PixelFormat,
IN EFI_PIXEL_BITMASK* PixelFormatBitMap,
IN UINT32 Width,
IN UINT32 Height,
IN UI_STYLE_INFO* StyleInfo
)
{
  INTN FillDataSize = 0;
  INT8 PixelSizeInBytes = 0;
  if (FrameBufferBase == NULL)
  {
    ASSERT(NULL != FrameBufferBase);
    return NULL;
  }

  if (UpperLeft == NULL)
  {
    ASSERT(NULL != UpperLeft);
    return NULL;
  }

  if (!IsStyleSupported(StyleInfo))
  {
    DEBUG((DEBUG_ERROR, "Style Info requested by caller is not supported!"));
    return NULL;
  }

  DEBUG((DEBUG_INFO, "MATTHEW CARLSON START\n"));

  // This is the default arrangement - 32 bit RGB
  if (PixelFormat == PixelRedGreenBlueReserved8BitPerColor) {
    PixelSizeInBytes = 4;
  }

  // This is the a reverse arrangement of the standard 32 bit BGR (RGB reversed)
  if (PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
    // TODO: Figure out how to re-organize the colors and create a color mapping
    DEBUG((DEBUG_ERROR, "%a:%d - BGR revered Framebuffer BOPS are not supported currently\n", __FILE__,__LINE__));
    ASSERT(PixelFormat != PixelBlueGreenRedReserved8BitPerColor);

  }
  // This is currently unsupported by this library
  else if (PixelFormat == PixelBltOnly) {
    DEBUG((DEBUG_ERROR, "%a:%d - BLT only GOP's are not supported currently\n", __FILE__,__LINE__));
    ASSERT(PixelFormat != PixelBltOnly);
  }
  // If this is bitmask, this mean to check the PixelBitMap and figure out how to handle it
  else if (PixelFormat == PixelBitMask) {
    DEBUG((DEBUG_INFO, "Figuring out how to map values from bitmap onto pixel size\n"));
    ASSERT(PixelFormatBitMap != NULL);
    UINT32 PixelBitFormat = PixelFormatBitMap->BlueMask | PixelFormatBitMap->GreenMask | PixelFormatBitMap->RedMask | PixelFormatBitMap->ReservedMask;
    ASSERT(PixelBitFormat != 0); // make sure we have a valid format
    DEBUG((DEBUG_INFO, "GOP Pixel Format: %x\n", PixelBitFormat));
    UINT8  BitsInPixel = 1;
    while( PixelBitFormat>>=1 ) BitsInPixel++;
    DEBUG((DEBUG_INFO, "GOP Bits in Pixel: %d\n", BitsInPixel));
    PixelSizeInBytes = BitsInPixel / 8;
  }
  ASSERT(PixelFormat != PixelFormatMax); // we should never have a pixel format that is at max
  
  DEBUG((DEBUG_INFO, "GOP Format: %x\n", PixelFormat));
  DEBUG((DEBUG_INFO, "GOP Bytes of a pixel: %x\n", PixelSizeInBytes));

  FillDataSize = GetFillDataSize(Width, Height, PixelSizeInBytes, StyleInfo);
  PRIVATE_UI_RECTANGLE *this = (PRIVATE_UI_RECTANGLE *)AllocateZeroPool(sizeof(PRIVATE_UI_RECTANGLE) + FillDataSize);
  ASSERT(NULL != this);
  ASSERT(PixelSizeInBytes != 0);

  if (this != NULL)
  {
    this->Public.UpperLeft = *UpperLeft;
    this->Public.FrameBufferBase = FrameBufferBase;
    this->Public.PixelsPerScanLine = PixelsPerScanLine;
    this->Public.PixelSizeInBytes = PixelSizeInBytes;
    this->Public.Width = Width;
    this->Public.Height = Height;
    this->Public.StyleInfo = *StyleInfo;
    this->FillDataSize = FillDataSize;

    this->Public.StyleInfo.IconInfo.PixelData = NULL;
    if ((this->Public.StyleInfo.IconInfo.Height > 0) && (this->Public.StyleInfo.IconInfo.Width > 0) && (StyleInfo->IconInfo.PixelData != NULL))
    {
      // By default pixel size is defined at 32 bit RGB with 8 bits reserved
      INTN PixelDataSize = this->Public.StyleInfo.IconInfo.Height * this->Public.StyleInfo.IconInfo.Width * sizeof(UINT32);
      this->Public.StyleInfo.IconInfo.PixelData = (UINT32*)AllocatePool(PixelDataSize);
      if (this->Public.StyleInfo.IconInfo.PixelData != NULL)
      {
        CopyMem(this->Public.StyleInfo.IconInfo.PixelData, StyleInfo->IconInfo.PixelData, PixelDataSize);
      }
    }

    PRIVATE_Init(this);
    return &this->Public;
  }

  return NULL;
}

/*
Method to free all allocated memory of the UI_RECTANGLE

@param this     - ProgressCircle object to draw

*/
VOID
EFIAPI
delete_UI_RECTANGLE(
IN UI_RECTANGLE *this
)
{
  if (this != NULL)
  {
    if (this->StyleInfo.IconInfo.PixelData != NULL)
    {
      FreePool(this->StyleInfo.IconInfo.PixelData);
    }
    FreePool(this);
  }
}


/*
Method to draw the rectangle to the framebuffer.

@param this     - UI_RECTANGLE object to draw

*/
VOID
EFIAPI
DrawRect(
IN  UI_RECTANGLE *this
)
{
  PRIVATE_UI_RECTANGLE* priv = (PRIVATE_UI_RECTANGLE*)this;
  UINT8* StartAddressInFrameBuffer = (UINT8*) (((UINT32*)this->FrameBufferBase) + ((this->UpperLeft.Y * this->PixelsPerScanLine) + this->UpperLeft.X));
  UINTN RowLengthInBytes = this->Width * this->PixelSizeInBytes;

  for (INTN y = 0; y < (INTN)this->Height; y++)   //each row
  {
    UINT32* temp = NULL;
    switch (this->StyleInfo.FillType){

      case FILL_SOLID:
      case FILL_VERTICAL_STRIPE:
        temp = (UINT32*)priv->FillData;
        break;

      case FILL_HORIZONTAL_STRIPE:
        temp = (UINT32*)priv->FillData;  //set to first row...else set to 2nd row
        if (((y / priv->Public.StyleInfo.FillTypeInfo.StripeFill.StripeSize) % 2) == 0)
        {
          //temp is uint32 pointer so move it forward by pixels
          temp += priv->Public.Width;
        }
        break;

      case FILL_FORWARD_STRIPE:
        temp = (UINT32*)priv->FillData;
        temp += (y % priv->Public.Height);  //Add 1 each time
        break;

      case FILL_BACKWARD_STRIPE:
        temp = (UINT32*)priv->FillData;
        temp += (priv->Public.Height - (y % priv->Public.Height));
        break;

      case FILL_CHECKERBOARD:
        temp = (UINT32*)priv->FillData;
        if (((y / priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.CheckboardWidth) % 2) == 0)
        {
          temp += priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.CheckboardWidth;
        }
        break;

      case FILL_POLKA_SQUARES:
        temp = (UINT32*)priv->FillData;  //set to first row


        if (((y + (priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.DistanceBetweenSquares / 2)) % (priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.DistanceBetweenSquares + priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.SquareWidth)) > priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.DistanceBetweenSquares)
        {
          //temp is uint32 pointer so move it forward by pixels
          temp += priv->Public.Width;
        }
        break;

      default:
        DEBUG((DEBUG_ERROR, "Unsupported Fill Type.  Cant draw Rectangle  0x%X\n", this->StyleInfo.FillType));
        return;
    }

    CopyMem(StartAddressInFrameBuffer, temp, RowLengthInBytes);
    StartAddressInFrameBuffer += (this->PixelsPerScanLine * this->PixelSizeInBytes);  //move to next row
  }

  if (this->StyleInfo.Border.BorderWidth > 0)
  {
    DrawBorder(priv);
  }

  if (this->StyleInfo.IconInfo.PixelData != NULL)
  {
    DrawIcon(priv);
  }
}



/***  PRIVATE METHODS ***/

/**
Method checks to see if the StyleInfo is supported by
this implementation of UiRectangle

@retval TRUE:   Supported
@retval FALSE:  Not Supported
**/
BOOLEAN
IsStyleSupported(IN UI_STYLE_INFO* StyleInfo)
{
  if ((StyleInfo->FillType > FILL_POLKA_SQUARES) || (StyleInfo->FillType < FILL_SOLID))
  {
    DEBUG((DEBUG_ERROR, "Unsupported FillType 0x%X\n", (UINTN) StyleInfo->FillType));
    return FALSE;
  }
  else
  {
    return TRUE;
  }
}

/**
Method returns the private datasize in bytes needed to support this style info for this rectangle

**/
INTN
GetFillDataSize(
IN UINTN Width,
IN UINTN Height,
IN UINT8 PixelSize,
IN UI_STYLE_INFO* StyleInfo
)
{
  INTN PixelBufferLength = 0;

  switch (StyleInfo->FillType)
  {
    case FILL_SOLID:
    case FILL_VERTICAL_STRIPE:
      PixelBufferLength = Width;
      break;

    case FILL_HORIZONTAL_STRIPE:
    case FILL_POLKA_SQUARES:
      PixelBufferLength = Width * 2;
      break;

    case FILL_FORWARD_STRIPE:
    case FILL_BACKWARD_STRIPE:
      PixelBufferLength = Width + Height;
      break;

    case FILL_CHECKERBOARD:
      PixelBufferLength = Width + StyleInfo->FillTypeInfo.CheckerboardFill.CheckboardWidth;
      break;
  }

  return PixelBufferLength * PixelSize;
}


VOID
PRIVATE_Init(
IN PRIVATE_UI_RECTANGLE* priv
)
{
  UINT8 PixelSizeInBytes = priv->Public.PixelSizeInBytes;
  //Init any private data needed for this rectangle
  INTN FillDataSizeInPixels = priv->FillDataSize / PixelSizeInBytes;
  DEBUG((DEBUG_INFO, "Private_init enter\n"));

  //Private fill data is used to hold row data needed for the fill
  switch (priv->Public.StyleInfo.FillType){
    case FILL_SOLID:
      DEBUG((DEBUG_INFO, "FILL_SOLID Private_init \n"));
      SetMemX(priv->FillData, priv->FillDataSize, (UINT8*)&(priv->Public.StyleInfo.FillTypeInfo.SolidFill.FillColor), PixelSizeInBytes);
      break;

    case FILL_HORIZONTAL_STRIPE:
      SetMem32(priv->FillData, (priv->FillDataSize / 2), priv->Public.StyleInfo.FillTypeInfo.StripeFill.Color1);
      SetMem32(priv->FillData + (priv->Public.Width * sizeof(UINT32)), (priv->FillDataSize / 2), priv->Public.StyleInfo.FillTypeInfo.StripeFill.Color2);
      break;

    case FILL_FORWARD_STRIPE:
    case FILL_BACKWARD_STRIPE:
    case FILL_VERTICAL_STRIPE:
      SetMem32(priv->FillData, priv->FillDataSize, priv->Public.StyleInfo.FillTypeInfo.StripeFill.Color1);  //set row to Color1

      //setup alternate color.  Color band is width stripe_width.
      //i is in pixels not bytes
      for (INTN i = priv->Public.StyleInfo.FillTypeInfo.StripeFill.StripeSize; i < FillDataSizeInPixels; i += (priv->Public.StyleInfo.FillTypeInfo.StripeFill.StripeSize * 2))
      {
        INTN Len = FillDataSizeInPixels - i;
        if (Len > priv->Public.StyleInfo.FillTypeInfo.StripeFill.StripeSize)
        {
          Len = (INTN) (priv->Public.StyleInfo.FillTypeInfo.StripeFill.StripeSize);
        }
        Len = Len * sizeof(UINT32);  //Convert Len from Pixels to bytes
        SetMem32(((UINT32*)priv->FillData) + i, Len, priv->Public.StyleInfo.FillTypeInfo.StripeFill.Color2);
      }
      break;

    case FILL_CHECKERBOARD:
      SetMem32(priv->FillData, priv->FillDataSize, priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.Color1);  //set row to Color1

      //setup alternate color.  Color band is width stripe_width.
      for (int i = priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.CheckboardWidth; i < FillDataSizeInPixels; i += (priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.CheckboardWidth * 2))
      {
        INTN Len = FillDataSizeInPixels - i;
        if (Len > priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.CheckboardWidth)
        {
          Len = (INTN)priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.CheckboardWidth;
        }
        Len = Len * sizeof(UINT32);  //convert len from Pixels to bytes
        SetMem32(((UINT32*)priv->FillData) + i, Len, priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.Color2);
      }
      break;

    case FILL_POLKA_SQUARES:
      SetMem32(priv->FillData, priv->FillDataSize, priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.Color1);  //set row to Color1

      //setup dot row. as row two of the filldata
      for (int i = priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.DistanceBetweenSquares / 2;
        i < (FillDataSizeInPixels / 2);
        i += (priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.SquareWidth + priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.DistanceBetweenSquares)
        )
      {
        INTN Len = (FillDataSizeInPixels / 2) - i;
        if (Len > priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.SquareWidth)
        {
          Len = (INTN)priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.SquareWidth;
        }
        Len = Len * sizeof(UINT32);  //convert len from Pixels to bytes
        SetMem32(priv->FillData + (priv->FillDataSize / 2) + (i * sizeof(UINT32)), Len, priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.Color2);  //color only the second row
      }
      break;

    default:
      DEBUG((DEBUG_ERROR, "Unsupported Fill Type.  0x%X\n", priv->Public.StyleInfo.FillType));
      break;
  }
}


/**
Method used to draw the rectangle border.  Border Width is included in rectangle width.
**/
VOID
DrawBorder(
IN PRIVATE_UI_RECTANGLE* priv
)
{
  UINT32* TempFB = (UINT32*)priv->Public.FrameBufferBase;  //frame buffer pointer
  TempFB += ((priv->Public.UpperLeft.Y * priv->Public.PixelsPerScanLine) + priv->Public.UpperLeft.X);  //move ptr to upper left of uirect

  if (priv->Public.StyleInfo.Border.BorderWidth <= 0)
  {
    return;
  }

  //Draw borders
  for (INTN Y = 0; Y < (INTN) priv->Public.Height; Y++)
  {
    if ((Y >= priv->Public.StyleInfo.Border.BorderWidth) && (Y < (INTN)(priv->Public.Height - priv->Public.StyleInfo.Border.BorderWidth)))
    {
      //left border
      SetMem32(TempFB, priv->Public.StyleInfo.Border.BorderWidth * sizeof(UINT32), priv->Public.StyleInfo.Border.BorderColor);
      //right border
      SetMem32(TempFB + (priv->Public.Width - priv->Public.StyleInfo.Border.BorderWidth), priv->Public.StyleInfo.Border.BorderWidth * sizeof(UINT32), priv->Public.StyleInfo.Border.BorderColor);
    }
    else
    {
      //top or bottom border
      SetMem32(TempFB, priv->Public.Width * sizeof(UINT32), priv->Public.StyleInfo.Border.BorderColor);
    }
    TempFB += priv->Public.PixelsPerScanLine;
  }
  return;
}


/**
Method used to draw an icon in the rectangle.
**/
VOID
DrawIcon(
IN PRIVATE_UI_RECTANGLE* priv
)
{
  INT32 OffsetX = 0;  //Left edge of icon in coordinate space of the rectangle
  INT32 OffsetY = 0;  //Upper edge of icon in coordinate space of the rectangle
  INT32 SizeX = (priv->Public.Width - (priv->Public.StyleInfo.Border.BorderWidth * 2));
  INT32 SizeY = (priv->Public.Height - (priv->Public.StyleInfo.Border.BorderWidth * 2));
  UINT8* TempFB = priv->Public.FrameBufferBase;  //frame buffer pointer
  UINT32* TempIcon = priv->Public.StyleInfo.IconInfo.PixelData;
  UINT8 PixelSizeInBytes = priv->Public.PixelSizeInBytes; // the number of bytes in a pixel

  if (priv->Public.StyleInfo.IconInfo.PixelData != NULL)
  {
    if ((priv->Public.StyleInfo.IconInfo.Width > SizeX ) || (priv->Public.StyleInfo.IconInfo.Height >  SizeY) )
    {
      DEBUG((DEBUG_ERROR, "Icon is larger than UI Rectangle.  Can't display icon.\n"));
      return;
    }

    //
    // Figure out where the logo is placed based on rect size, border, icon size, and icon placement
    // Offset X and Y will be in coordinate space of rectangle.
    //
    switch (priv->Public.StyleInfo.IconInfo.Placement)
    {
      case TOP_LEFT:
        OffsetX = priv->Public.StyleInfo.Border.BorderWidth;
        OffsetY = priv->Public.StyleInfo.Border.BorderWidth;
        break;

      case TOP_CENTER:
        OffsetX = (priv->Public.Width / 2) - (priv->Public.StyleInfo.IconInfo.Width / 2);
        OffsetY = priv->Public.StyleInfo.Border.BorderWidth;
        break;

      case TOP_RIGHT:
        OffsetX = priv->Public.Width - priv->Public.StyleInfo.Border.BorderWidth - priv->Public.StyleInfo.IconInfo.Width;
        OffsetY = priv->Public.StyleInfo.Border.BorderWidth;
        break;

      case MIDDLE_LEFT:
        OffsetX = priv->Public.StyleInfo.Border.BorderWidth;
        OffsetY = (priv->Public.Height / 2) - (priv->Public.StyleInfo.IconInfo.Height / 2);
        break;

      case MIDDLE_CENTER:
        OffsetX = (priv->Public.Width / 2) - (priv->Public.StyleInfo.IconInfo.Width / 2);
        OffsetY = (priv->Public.Height / 2) - (priv->Public.StyleInfo.IconInfo.Height / 2);
        break;

      case MIDDLE_RIGHT:
        OffsetX = priv->Public.Width - priv->Public.StyleInfo.Border.BorderWidth - priv->Public.StyleInfo.IconInfo.Width;
        OffsetY = (priv->Public.Height / 2) - (priv->Public.StyleInfo.IconInfo.Height / 2);
        break;

      case BOTTOM_LEFT:
        OffsetX = priv->Public.StyleInfo.Border.BorderWidth;
        OffsetY = priv->Public.Height - priv->Public.StyleInfo.Border.BorderWidth - priv->Public.StyleInfo.IconInfo.Height;
        break;

      case BOTTOM_CENTER:
        OffsetX = (priv->Public.Width / 2) - (priv->Public.StyleInfo.IconInfo.Width / 2);
        OffsetY = priv->Public.Height - priv->Public.StyleInfo.Border.BorderWidth - priv->Public.StyleInfo.IconInfo.Height;
        break;

      case BOTTOM_RIGHT:
        OffsetX = priv->Public.Width - priv->Public.StyleInfo.Border.BorderWidth - priv->Public.StyleInfo.IconInfo.Width;
        OffsetY = priv->Public.Height - priv->Public.StyleInfo.Border.BorderWidth - priv->Public.StyleInfo.IconInfo.Height;
        break;

      default:
        DEBUG((DEBUG_ERROR, "Unsupported Icon Placement value.\n"));
        return;
    } //close switch

    //now convert offsetx and y to framebuffer coordinates and draw it
    TempFB += ((priv->Public.UpperLeft.Y * priv->Public.PixelsPerScanLine) + priv->Public.UpperLeft.X) * PixelSizeInBytes;  //move ptr to upper left of uirect
    TempFB += ((OffsetY * priv->Public.PixelsPerScanLine) + OffsetX) * PixelSizeInBytes;  //move ptr to upper left of icon
    for (INTN Y = 0; Y < priv->Public.StyleInfo.IconInfo.Height; Y++)
    {
      //draw icon
      // TODO figure out how to convert icon to proper version
      CopyMem(TempFB, TempIcon, priv->Public.StyleInfo.IconInfo.Width * priv->Public.PixelSizeInBytes);
      TempFB += priv->Public.PixelsPerScanLine * PixelSizeInBytes;
      TempIcon += priv->Public.StyleInfo.IconInfo.Width;
    }

  } //close if pixel data not null
}

VOID *
EFIAPI
SetMemX (
  OUT VOID   *Buffer,
  IN UINTN   BufferLength,
  IN UINT8   *Value,
  IN UINTN   ValueLength
) 
{
  volatile UINT8 *Pointer8;
  UINTN           ValueIter;
  // Check if it's a common size we already support
  if (ValueLength == 1) return   SetMem (Buffer, BufferLength, (UINT8)  *Value);
  if (ValueLength == 2) return SetMem16 (Buffer, BufferLength, (UINT16) *Value);
  if (ValueLength == 4) return SetMem32 (Buffer, BufferLength, (UINT32) *Value);
  if (ValueLength == 8) return SetMem64 (Buffer, BufferLength, (UINT64) *Value);
  Pointer8 = Buffer;
  DEBUG((DEBUG_INFO, "SetMemX: Writing to %x of length %x\n", Buffer, BufferLength));
  DEBUG((DEBUG_INFO, "SetMemX: Writing value of length %x:", ValueLength));
  for(ValueIter = 0; ValueIter <= ValueLength; ValueIter += 1) {
    DEBUG((DEBUG_INFO, "%x", *(Value+ValueIter)));
  }
  DEBUG((DEBUG_INFO, "\n"));
  ValueIter = 0;
  while (BufferLength-- > 0) {
    *(Pointer8++) = *(Value + ValueIter);
    ValueIter = (ValueIter + 1) % ValueLength;
  }
  return Buffer;
}