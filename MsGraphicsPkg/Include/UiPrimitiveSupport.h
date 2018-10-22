/** @file

This include file is to support common elements used for the Ui Primitive libraries

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

#ifndef __UI_PRIMITIVE_SUPPORT_H__
#define __UI_PRIMITIVE_SUPPORT_H__

//
// Struct used to represent a point in 2D space
//
typedef struct {
  INTN X;
  INTN Y;
} POINT;

//
// Defined fill types. 
//
typedef enum _UI_FILL_TYPE {
  FILL_SOLID,
  FILL_FORWARD_STRIPE,
  FILL_BACKWARD_STRIPE,
  FILL_VERTICAL_STRIPE,
  FILL_HORIZONTAL_STRIPE,
  FILL_CHECKERBOARD,
  FILL_POLKA_SQUARES
} UI_FILL_TYPE;

typedef union {
  struct {
    UINT32 FillColor;
  } SolidFill;

  struct {
    UINT32 Color1;
    UINT32 Color2;
    INT32 StripeSize;  //Width or Height depending on Stripe type
  } StripeFill;

  struct {
    UINT32 Color1;
    UINT32 Color2;
    INT32 CheckboardWidth;
  } CheckerboardFill;

  struct {
    UINT32 Color1;
    UINT32 Color2;
    INT32 DistanceBetweenSquares;
    INT32 SquareWidth;
  } PolkaSquareFill;
} UI_FILL_TYPE_STYLE_UNION;

typedef struct {
  UINT32 BorderColor;
  INT32  BorderWidth;
} UI_BORDER_STYLE;

typedef enum {
  INVALID_PLACEMENT,
  TOP_LEFT,
  TOP_CENTER,
  TOP_RIGHT,
  MIDDLE_LEFT,
  MIDDLE_CENTER,
  MIDDLE_RIGHT,
  BOTTOM_LEFT,
  BOTTOM_CENTER,
  BOTTOM_RIGHT
} UI_PLACEMENT;

typedef struct {
  INT32   Width;
  INT32   Height;
  UI_PLACEMENT Placement;
  UINT32* PixelData;
} UI_ICON_INFO;

typedef struct {
  UI_BORDER_STYLE Border;
  UI_FILL_TYPE    FillType;
  UI_FILL_TYPE_STYLE_UNION FillTypeInfo;
  UI_ICON_INFO IconInfo;
} UI_STYLE_INFO;

#endif  // __UI_PRIMITIVE_SUPPORT_H__
