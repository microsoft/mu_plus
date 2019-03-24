/** @file
    Unlock graphic used by DisplayDeviceStateLib

    Copyright (c) 2018, Microsoft Corporation

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