/**

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef UI_RECTANGLE_H
#define UI_RECTANGLE_H

typedef struct {
  UI_RECTANGLE Public;
  INTN FillDataSize;
  UINT8  FillData[0];
} PRIVATE_UI_RECTANGLE;

/***  PRIVATE METHODS ***/

/** 
Method checks to see if the StyleInfo is supported by
this implementation of UiRectangle

@retval TRUE:   Supported
@retval FALSE:  Not Supported
**/
BOOLEAN
IsStyleSupported(IN UI_STYLE_INFO* StyleInfo);


/** 
Method returns the private datasize in bytes needed to support this style info for this rectangle

**/
INTN
GetFillDataSize(
IN UINTN Width,
IN UINTN Height,
IN UI_STYLE_INFO* StyleInfo
);


VOID
PRIVATE_Init(
IN PRIVATE_UI_RECTANGLE* priv
);

VOID
DrawBorder(
IN PRIVATE_UI_RECTANGLE* priv
);

VOID
DrawIcon(
IN PRIVATE_UI_RECTANGLE* priv
);

#endif
