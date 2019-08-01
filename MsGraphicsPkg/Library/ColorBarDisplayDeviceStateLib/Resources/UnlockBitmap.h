/** @file
    Unlock graphic used by DisplayDeviceStateLib

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

typedef struct BITMAPDATA {
    CONST INT32 Height;
    CONST INT32 Width;
    CONST UINT32* BlitData;
    CONST UINT32  BuffSize;
} BITMAPDATA;


#ifdef COLOR_BAR_UNLOCK_ICON_BITMASK

    #if COLOR_BAR_UNLOCK_ICON_BITMASK & 0x1
        #include "UnlockBitmap32.h"
    #endif
    #if COLOR_BAR_UNLOCK_ICON_BITMASK & 0x2
        #include "UnlockBitmap64.h"
    #endif
    #if COLOR_BAR_UNLOCK_ICON_BITMASK & 0x4
        #include "UnlockBitmap112.h"
    #endif
    #if COLOR_BAR_UNLOCK_ICON_BITMASK & 0x8
        #include "UnlockBitmap128.h"
    #endif
    #if COLOR_BAR_UNLOCK_ICON_BITMASK & 0x10
        #include "UnlockBitmap256.h"
    #endif

    //Pre-Defined Unlock Bitmap Sizes
    //  Pos 0: 166 x 256 x 32bpp
    //  Pos 1: 82  x 128 x 32bpp
    //  Pos 2: 72  x 112 x 32bpp - Legacy Size
    //  Pos 3: 42  x 64  x 32bpp
    //  Pos 4: 21  x 32  x 32bpp
    BITMAPDATA* mBlitArray[] = {
        #if COLOR_BAR_UNLOCK_ICON_BITMASK & 0x10
            &unlock256,
        #endif

        #if COLOR_BAR_UNLOCK_ICON_BITMASK & 0x8
            &unlock128,
        #endif

        #if COLOR_BAR_UNLOCK_ICON_BITMASK & 0x4
            &unlock112,
        #endif

        #if COLOR_BAR_UNLOCK_ICON_BITMASK & 0x2
            &unlock64,
        #endif

        #if COLOR_BAR_UNLOCK_ICON_BITMASK & 0x1
            &unlock32
        #endif
    };

#else

    #include "UnlockBitmap32.h"
    #include "UnlockBitmap64.h"
    #include "UnlockBitmap112.h"
    #include "UnlockBitmap128.h"
    #include "UnlockBitmap256.h"

    //Pre-Defined Unlock Bitmap Sizes
    //  Pos 0: 166 x 256 x 32bpp
    //  Pos 1: 82  x 128 x 32bpp
    //  Pos 2: 72  x 112 x 32bpp - Legacy Size
    //  Pos 3: 42  x 64  x 32bpp
    //  Pos 4: 21  x 32  x 32bpp
    BITMAPDATA* mBlitArray[5] = {&unlock256,&unlock128,&unlock112,&unlock64,&unlock32};
#endif