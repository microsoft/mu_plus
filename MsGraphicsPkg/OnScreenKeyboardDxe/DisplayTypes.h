/** @file
  Implements common structures and constants used by a simple on-screen virtual keyboard.

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

#ifndef _DISPLAY_TYPES_H_
#define _DISPLAY_TYPES_H_

// Preprocessor constants
//
#define PI                  (float)3.14159265
#define HALF_PI             (float)1.570796325
#define RADIANS_PER_DEGREE  (float)0.017453293
#define DEGREES_PER_RADIAN  (float)57.295779579


// Point definition in 3D-space
//
typedef union
{
    struct
    {
        float x;
        float y;
        float z;
        float rsvd;		// Always 1.
    } pt;
    float mtx[4];
} POINT3D;

// Rectangle definition in 3D-space
//
typedef struct _RECT_tag
{
    POINT3D topL;
    POINT3D topR;
    POINT3D botL;
    POINT3D botR;
} RECT3D;

// Font package definition.
//
#pragma pack (push, 1)
typedef struct _OSK_FONT_PACKAGE_tag_
{
    EFI_HII_FONT_PACKAGE_HDR FontHeader;
    CHAR16 FontFamilyNameContd[25];
} OSK_FONT_PACKAGE_HEADER;
#pragma pack (pop)

#endif  // _DISPLAY_TYPES_H_
