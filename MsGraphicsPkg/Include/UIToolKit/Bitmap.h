/** @file

  Implements a simple control for managing & displaying a bitmap image.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UIT_BITMAP_H_
#define _UIT_BITMAP_H_


//////////////////////////////////////////////////////////////////////////////
// Bitmap Class Definition
//
typedef struct _Bitmap
{
    // *** Base Class ***
    //
    ControlBase                         Base;

    // *** Member variables ***
    //
    SWM_RECT                            m_BitmapBounds;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *m_Bitmap;

    // *** Functions ***
    //
    VOID 
    (*Ctor)(IN struct _Bitmap                  *this,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *Bitmap,
            IN SWM_RECT                         BitmapBounds);

} Bitmap;


//////////////////////////////////////////////////////////////////////////////
// Public
//
Bitmap *new_Bitmap(IN UINT32                         OrigX,
                   IN UINT32                         OrigY,
                   IN UINT32                         BitmapWidth,
                   IN UINT32                         BitmapHeight,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Bitmap);

VOID delete_Bitmap(IN Bitmap *this);


#endif  // _UIT_BITMAP_H_.
