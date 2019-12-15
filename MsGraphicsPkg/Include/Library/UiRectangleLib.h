/** @file

Ui Rectangle Lib
This supports making drawable rectangle primitives with different fill and border options

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UI_RECTANGLE_LIB_H_
#define __UI_RECTANGLE_LIB_H_

//
// Rectangle context
//
typedef struct
{
  POINT                        UpperLeft;
  UINT32                       Width;
  UINT32                       Height;
  UINT8*                       FrameBufferBase;
  UINTN                        PixelsPerScanLine;  //in framebuffer
  UI_STYLE_INFO                StyleInfo;
} UI_RECTANGLE;


/*
Method to use create a new UI_RECTANGLE struct.
This structure is used by all the other functions to modify and draw the object

@param UpperLeft         - Upper left point of rectangle in framebuffer coordinates
@param FrameBufferBase   - pointer to framebuffer address of 0,0  (upper left)
@param PixelsPerScanLine - Number of pixels per scan line in framebuffer.
                           This is to support aligned framebuffers
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
  IN UINT32 Width,
  IN UINT32 Height,
  IN UI_STYLE_INFO* StyleInfo
  );


/*
Method to free all allocated memory of the UI_RECTANGLE

@param this     - ProgressCircle object to draw

*/
VOID
EFIAPI
delete_UI_RECTANGLE(
IN UI_RECTANGLE *this
);



/*
Method to draw the rectangle to the framebuffer.

@param this     - UI_RECTANGLE object to draw

*/
VOID
EFIAPI
DrawRect(
IN  UI_RECTANGLE *this
);


#endif
