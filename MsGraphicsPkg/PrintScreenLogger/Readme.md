# PrintScreenLogger

## About

PrintScreenLogger is a DXE_DRIVER you can include in your platform to obtain
Screen Captures during the preboot environment by pressing the Ctrl-PrtScn key
combination. This action will creates a 24bbp (Bits Per Pixel) .BMP file of the
screen's contents and write it to a enabled USB drive.

## Supported Architectures

This package is not architecturally dependent. This package is dependent upon 
the Gop pixel format, and only supports these two pixel formats:
1. *PixelRedGreenBlueReserved8BitPerColor*
2. *PixelBlueGreenRedReserved8BitPerColor*
 
## PrintScreenLogger operation

During initialization, the Print Screen Loggger registers for notification of
the Ctrl-PrtScn key combination is pressed.

When a Print Screen callback occurs:

1. Looks for a mounted USB drive that contains a file in the root directory called
   **PrintScreenEnable.txt**.  This limits PrintScreenLogger to only write to
   enabled USB devices.
2. Looks for the next available filename in the form **PrtScreen####.bmp**,
   starting with 0000.
3. Creates the new **PrtScreen####.bmp** file.
4. Call GraphicsOutput->Blt to obtain the complete screen.  
5. Converts the BLT buffer to a 24bbp BMP structure.
6. Writes the BMP structure to the new **PrtScreen####.bmp** file.

# Including in your platform

## Sample DSC change
    [Components.<arch>]
    ...
    ...
    MsGraphicsPkg/PrintScreenFileLogger/PrintScreenFileLogger.inf
    

## Sample FDF change

    [FV.<a DXE firmware volume>]
    ...
    ...
    INF MsGraphicsPkg/PrintScreenFileLogger/PrintScreenFileLogger.inf

## Copyright

Copyright (c) 2018, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions are
met:
1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
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
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
