/**

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
