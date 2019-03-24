/**@file
QrEncoderTables.h

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
#ifndef __QRENCODER_TABLES_LIB__
#define __QRENCODER_TABLES_LIB__

/*--------------------------------------------------------------------------------*/
/*  Format of table A.1 entries                                                   */
/*--------------------------------------------------------------------------------*/
typedef struct {
    UINT16 totalWords;             // Number of Data Words
    UINT16 ECWordsPerBlock;        // Number of EC words to generate from each block of data words
    UINT16 group1BlockCount;       // Number of group 1 blocks
    UINT16 group1Words;            // Number of data words in a group 1 block
    UINT16 group2BlockCount;       // Number of group 2 blocks
    UINT16 group2Words;            // Number of data words in a group 2 block
    UINT16 requiredRemainder;      // Number of leftover bits to be set to 0
    UINT16 maxNumeric;             // Maximum number of Numeric encoded characters in this QR code
    UINT16 maxAlphanumeric;        // Maximum number of AlphaNumeric encoded characters in this QR code
    UINT16 maxBytes;               // Maximum number of Byte encoded data in this QR code
    UINT16 maxKanji;               // Maximum number of Kanji encoded characters in this QR code
} QrTableEntry;

/*--------------------------------------------------------------------------------*/
/*  Magic numbers from the ISO 18004:2015 spec                                    */
/*--------------------------------------------------------------------------------*/
#define QR_CODES  40               // Support for QR codes 1-40 (no support for Micro QR Codes)
#define QR_EC_LEVELS 4             // 4 EC levels (L,M,Q,H)
#define QR_TABLE_ENTRIES (QR_CODES * QR_EC_LEVELS) // 40 QrCodes * 4 EC versions per QrCode
#define QR_ALPHA_TABLE_SIZE 45     // 45 Alphanumeric codes
#define QR_MAX_LOCATIONS 7         // Number of Alignment locations
#define QR_MAX_GEN_POLYS 69        // Number of generator polynomials
#define QR_LENGTH_ENTRIES 3        // There are 3 scales of length bits
#define QR_ENCODER_ENTRIES 5       // Encoder entries
#define QR_MASK_PATTERNS 8         // Number of mask patterns
#define QR_QUIET_ZONE 4            // Reguired white modules on each side

extern CONST QrTableEntry gQrTable[QR_TABLE_ENTRIES];      // ISO 18004:2015 Combination of data from Table 7 and Table 9

extern CONST UINT8 gAlphaNumerics[QR_ALPHA_TABLE_SIZE];    // ISO 18004:2015 Table 5 Encoding for Alphanumeric mode

extern CONST UINT8 gLengthBits[QR_LENGTH_ENTRIES][QR_ENCODER_ENTRIES];  // ISO 18004:2015 Table 3 Number of Bits in the length indicator

extern CONST UINT16 gFormatInfo[QR_EC_LEVELS][QR_MASK_PATTERNS];  // Table C.1 ISO 18004:2015

extern CONST UINT32 gVersionInfo[QR_CODES-6];                // Table D.1 ISO 18004:2015

extern CONST UINT8 gAlignmentLocations[QR_CODES - 1][QR_MAX_LOCATIONS]; // ISO 18004:2015 Table E.1

extern CONST UINT8 *gGeneratorPolynomials[QR_MAX_GEN_POLYS]; // ISO 18004:2015 Table A.1 - ISO has a sparse list up to 68.
                                                             // However, only those code up to 30 are used
// Galios Field GF(256) log tables
#define GF256_SIZE 256
extern UINT8 logTable[GF256_SIZE];
extern UINT8 alogTable[GF256_SIZE];


VOID InitializeLogTables(VOID);    // Initialize Galois field Log Tables

#endif
