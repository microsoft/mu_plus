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

During initialization, the Print Screen Logger registers for notification of
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

## Including in your platform

## Sample DSC change

```inf
    [Components.<arch>]
    ...
    ...
    MsGraphicsPkg/PrintScreenLogger/PrintScreenLogger.inf
```

## Sample FDF change

```inf
    [FV.<a DXE firmware volume>]
    ...
    ...
    INF MsGraphicsPkg/PrintScreenLogger/PrintScreenLogger.inf
```

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
