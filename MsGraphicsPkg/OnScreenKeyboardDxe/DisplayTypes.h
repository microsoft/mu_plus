/** @file
  Implements common structures and constants used by a simple on-screen virtual keyboard.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
