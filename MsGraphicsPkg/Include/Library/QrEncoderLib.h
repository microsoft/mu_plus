/**@file
QrEncoderLib.h

QrEncoderLib is used to generate a QR code from caller data.

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

#ifndef __QRENCODER_LIB__
#define __QRENCODER_LIB__

#include <Protocol/GraphicsOutput.h>

typedef enum {
    QrECLevel_L = 1,
    QrECLevel_M = 2,
    QrECLevel_Q = 3,
    QrECLevel_H = 4,
} QRLEVEL;

#define QrAutoVersion 0
#define QrMinVersion  1
#define QrMaxVersion 40

typedef enum {
    QrAutoMode         = 0,
    QrNumericMode      = 1,
    QrAlphaNumericMode = 2,
    QrByteMode         = 3,
    QrECIMode          = 4,  // NOT SUPPORTED AT THIS TIME
    QrStAppendMode     = 5,  // NOT SUPPORTED AT THIS TIME
    QrFNC1Mode         = 6   // NOT SUPPORTED AT THIS TIME
} QRENCODING;

// Module Colors
#define  QrRsvd      0x03
#define  QrWhite     0x02
#define  QrBlack     0x01
#define  QrGray      0x00

#define  QrExclude   0x80   // Indicates a module that does not participate in XOR data masking

#define  QrRsvd_E   (QrRsvd  | QrExclude )
#define  QrWhite_E  (QrWhite | QrExclude )
#define  QrBlack_E  (QrBlack | QrExclude )
#define  QrGray_E   (QrGray  | QrExclude )

#define QR_FLAGS_MASK_SEL         0x00000008     // Bit indicate USE low 3 bits for the MASK code
#define QR_FLAGS_MASK_0           0x00000008
#define QR_FLAGS_MASK_1           0x00000009
#define QR_FLAGS_MASK_2           0x0000000A
#define QR_FLAGS_MASK_3           0x0000000B
#define QR_FLAGS_MASK_4           0x0000000C
#define QR_FLAGS_MASK_5           0x0000000D
#define QR_FLAGS_MASK_6           0x0000000E
#define QR_FLAGS_MASK_7           0x0000000F
#define QR_FLAGS_NO_MASK          0x00000010    // No Mask
#define QR_FLAGS_DEBUG_BIT_STREAM 0x00000020
#define QR_FLAGS_DEBUG_CODE_WORDS 0x00000040
#define QR_FLAGS_DEBUG_POLYDIVIDE 0x00000080
#define QR_FLAGS_DEBUG_ENCODING   0x00000100
#define QR_FLAGS_DEBUG_MASKING    0x00000200   // Write 0 to last data word to validate Masking
#define QR_FLAGS_DEBUG_MASK_ONLY  0x00000400   // Only write the mask to validate mask formulae

//*----------------------------------------------------------------------------*
//*   QrEncodeData                                                             *
//*                                                                            *
//*   Creates the QR Bitmap using the Version and Mode from the initialize     *
//*   setp                                                                     *
//*                                                                            *
//*   QR Version and Encoding mode can be set to Auto, and will be determined  *
//*   by the data.                                                             *
//*                                                                            *
//*   Input:                                                                   *
//*      Version      Version Requested (1-40, 0=Auto)                         *
//*      Level        Error Correction Level                                   *
//*      Mode         Character Encoding mode                                  *
//*      Flags        Debug flags - Used only by QrTest. Enabled additional dbg*
//*      Data         Character string for the QR Code                         *
//*      DataLen      Length of the data                                       *
//*      RegionSize   Area available for the QR Code                           *
//*      Bitmap       Where to store the pointer to a Gop->Blt ready           *
//*                   bitmap that will fit into RegionSize.                    *
//*                                                                            *
//*   Returns:                                                                 *
//*      EFI_INVALID_PARAMETER = Version, Level, or Mode out of range          *
//*                              Data == NULL, or Datalen == 0                 *
//*                              RegionSize too small                          *
//*                              Bitmap == NULL, BitmapLen == NULL,            *
//*----------------------------------------------------------------------------*
EFI_STATUS
EFIAPI
QrEncodeData (
              IN  UINT8                            Version,      // Version requested
              IN  QRLEVEL                          Level,        // EC Correction level
              IN  QRENCODING                       Mode,         // Alpha encoding??
              IN  UINT32                           Flags,        // Debug fla
              IN  UINT8                           *Data,         // Input Ascii Character data (BINARY IS NOT SUPPORTED)
              IN  UINT16                           DataLen,      // Length of the data
              IN  INTN                             RegionSize,   // Width and height of the square display area
              OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL  **Bitmap);      // Place to store the created bitmap pointer

#endif
