/**@file
QrEncoderLib.c

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

#include <Uefi.h>

#include <Protocol/GraphicsOutput.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>            // AllocatePool
#include <Library/QrEncoderLib.h>
#include <Library/UefiBootServicesTableLib.h>       // gBS

#include "QrEncoderTables.h"


//*----------------------------------------------------------------------------*
//*   Global Variables                                prefix g for Global      *
//*----------------------------------------------------------------------------*
UINT8                           gQrVersion;
INTN                            gQrSize;
QRLEVEL                         gQrLevel;
QRENCODING                      gQrMode;
INTN                            gQrMask;
UINT8                          *gQrBitmap      = NULL;
INTN                            gQrBitmapLen;
UINT8                          *gCodeWords     = NULL;
UINTN                           gCodeWordCount;
UINT8                          *gECWords       = NULL;
UINTN                           gECWordCount;
UINT8                          *gBitStream     = NULL;
UINTN                           gBitStreamCount;
CONST QrTableEntry             *gQrT;
UINT32                          gFlags;
EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *gBltBuffer     = NULL;

// Module Colors
#define  QrRsvd      0x03
#define  QrWhite     0x02
#define  QrBlack     0x01
#define  QrGray      0x00
#define  QrExclude   0x80      // Color mask to assist with XOR pattern

// Module Drawing state
static BOOLEAN   up;
static BOOLEAN   right;
static INTN      row;
static INTN      col;

#define BITS_PER_BYTE 8
//*----------------------------------------------------------------------------*
//*   Check Encoding Type.                                                     *
//*   Checks the data stream and returns the lowest encoding type for the      *
//*   data                                                                     *
//*----------------------------------------------------------------------------*
static
QRENCODING
CheckEncodingType (UINT8 * Data, UINTN Datalen) {
    UINTN       i;
    UINTN       j;
    BOOLEAN     numeric = TRUE;
    BOOLEAN     alphanum = TRUE;
    QRENCODING  suggestedType;

    // check for numbers only
    for (i = 0; i < Datalen; i++) {
        if ((Data[i] < '0') || (Data[i] > '9')) {
            numeric = FALSE;
        }
        for (j = 0; j < sizeof(gAlphaNumerics); j++) {
            if (Data[i] == gAlphaNumerics[j]) {
                break;
            }
        }
        if (j == sizeof(gAlphaNumerics)) {
            alphanum = FALSE;
            break;
        }
    }

    if (numeric == TRUE) {
        suggestedType = QrNumericMode;
    } else if (alphanum == TRUE) {
        suggestedType = QrAlphaNumericMode;
    } else {
        suggestedType = QrByteMode;
    }
    return suggestedType;
}

//*----------------------------------------------------------------------------*
//*   Check Qr Version.                                                        *
//*   Checks the data stream and returns the lowest encoding type for the      *
//*   data                                                                     *
//*----------------------------------------------------------------------------*
UINT8
CheckQrVersion(UINT8 *Data, UINT16 DataLen) {
    UINTN               i;
    UINT8               Version = 0;
    UINT16              qLen;
    CONST QrTableEntry *QrT;

    // Start with Version 1 table entry - using qQrLevel to select which of the 4 entries per version to look at
    for (i = gQrLevel - 1; i < QR_TABLE_ENTRIES; i += 4) {  // Spec has versions 1-40, and 4 EC levels.
        QrT = &gQrTable[i];
        DEBUG((DEBUG_INFO, "Checking version table entry %d\n",i));

        switch (gQrMode) {
        case QrNumericMode:
            qLen = QrT->maxNumeric;
            break;
        case QrAlphaNumericMode:
            qLen = QrT->maxAlphanumeric;
            break;
        case QrByteMode:
            qLen = QrT->maxBytes;
            break;
        default:
            qLen = 0;
            DEBUG((DEBUG_ERROR,__FUNCTION__ " Internal error - QrMode invalid %d\n",gQrMode));
            ASSERT(FALSE);
        }
        if (qLen >= DataLen) {
            Version = (UINT8) (i/4) + 1;
            break;
        }
    }
    if (Version == 0) {
        DEBUG((DEBUG_ERROR,"Unable to find a proper QrCode version\n"));
    } else {
        DEBUG((DEBUG_INFO,"suggesting Version %d\n",Version));
    }
    return Version;
}

//*----------------------------------------------------------------------------*
//*   Private Variables for AddCodeWordBits    prefix pv for Private Variables *
//*----------------------------------------------------------------------------*
static UINTN                  pvCwIndex;
static UINTN                  pvCwUsed;
static UINTN                  pvCwTarget;

/*--------------------------------------------------------------------------------*/
/*  Init Code Word Bits                                                           */
/*       Bits           - The bits to be added                                    */
/*       Count          - Number of bits.  Intended to work with number of bits in*/
/*                        Bits - 1                                                */
/*       NOTE:  It is the callers responsibility to free the gCodeWords array     */
/*--------------------------------------------------------------------------------*/
static
VOID
InitCodeWords (UINTN NumberOfCodeWords) {

    gCodeWords = AllocateZeroPool (NumberOfCodeWords);  // CodeWords needs to be zeros.
    gCodeWordCount = NumberOfCodeWords;
    pvCwIndex = 0;
    pvCwUsed = 0;
    pvCwTarget = NumberOfCodeWords;
}

/*--------------------------------------------------------------------------------*/
/*  AddCodeWordBits     Adds bits to the CodeWord array                           */
/*       Bits           - The bits to be added                                    */
/*       Count          - Number of bits.  Intended to work with number of bits in*/
/*                        Bits - 1                                                */
/*--------------------------------------------------------------------------------*/
static
VOID
AddCodeWordBits (UINTN Bits, UINTN Count) { // allows bits up to 31/63
    UINTN temp;
    UINTN mask;

    if (gFlags & QR_FLAGS_DEBUG_ENCODING) {
        DEBUG((DEBUG_INFO,"Adding %d bits %x\n", Count, Bits));
    }
    ASSERT (gCodeWords != NULL);

    while (Count > 0) {
        mask = (-1 << Count);
        temp = Bits & ~mask;

        if (Count <= (BITS_PER_BYTE - pvCwUsed)) { // All remaining bits fit into this CodeWord
            temp <<= (BITS_PER_BYTE - pvCwUsed) - Count;
            pvCwUsed += Count;
            Count = 0;
        } else {                       // Take up to 8 bits from the remaining bits
            temp >>= Count - (BITS_PER_BYTE - pvCwUsed);
            Count -= (BITS_PER_BYTE - pvCwUsed);
            pvCwUsed += BITS_PER_BYTE - pvCwUsed;
        }
        if (pvCwIndex < pvCwTarget) {
            gCodeWords[pvCwIndex] |= (UINT8) temp;
        } else {
            DEBUG((DEBUG_ERROR,"Unable to store bits %d\n",temp) );
        }
        gCodeWords[pvCwIndex] |= (UINT8) temp;
        if (pvCwUsed == BITS_PER_BYTE) {
            pvCwIndex++;
            pvCwUsed = 0;
        }
    }
}

#define maxBitsSupported 31
/*--------------------------------------------------------------------------------*/
/*  PrintBinary       Print bits in binary                                        */
/*       data           - The binary data                                         */
/*       width          - how many bits to print (max number of bits in UINT32 - 1*/
/*       fill           - leading characters for width                            */
/*--------------------------------------------------------------------------------*/
static
VOID
PrintBinary (UINTN data, UINTN width, CHAR8 fill ) {
    UINTN  Mask;
    CHAR8  binString[maxBitsSupported + 1];  // Bits as printable chars + a terminating NULL
    UINTN  Index = 0;

    if (width > maxBitsSupported) {
        ASSERT (width <= maxBitsSupported);
        return;
    }

    Mask = 0x01i64 << (width - 1);
    for (; Mask != 0; Mask >>= 1) {
        if (data & Mask) {
            binString[Index] = '1';
            fill = '0';
        } else {
            binString[Index] = fill;
        }
        Index++;
    }
    binString[Index] = '\0';
    DEBUG((DEBUG_INFO,"%a",binString));
}

/*--------------------------------------------------------------------------------*/
/*  AddCodeWordBits     Adds bits to the CodeWord array                           */
/*       Bits           - The bits to be added                                    */
/*       Count          - Number of bits.  Intended to work with number of bits in*/
/*                        Bits - 1                                                */
/*--------------------------------------------------------------------------------*/
static
VOID
AddCodeWordPadBytes ( VOID ) {
BOOLEAN PadSelect;
UINT8   PadCharacter;
UINTN   i;

#define PAD1 0xEC  // Pad code word values from ISO 18004-2015 7.4.10
#define PAD2 0x11

    if (pvCwUsed > 0) {
        pvCwIndex++;
    }
    PadSelect = FALSE;

    while (pvCwIndex < pvCwTarget) {
        if (PadSelect) {
            PadSelect = FALSE;
            PadCharacter = PAD2;
        } else {
            PadCharacter = PAD1;
            PadSelect = TRUE;
        }
        gCodeWords[pvCwIndex++] = PadCharacter;
    }

    if (gFlags & QR_FLAGS_DEBUG_CODE_WORDS) {
        for (i = 0; i < pvCwTarget; i++)
        {
            DEBUG((DEBUG_INFO," CodeWord %4d is %4d - ", i, gCodeWords[i]));
            PrintBinary (gCodeWords[i], 8, '0');
            DEBUG((DEBUG_INFO,"\n"));
        }
    }
}

/*--------------------------------------------------------------------------------*/
/*  Polynomial divide                                                             */
/*       DividendCount  - number of data code words                               */
/*       Dividend       - CodeWords                                               */
/*       RemainderCount - number of EC code words to generate                     */
/*       Remainder      - Where to store the EC code words                        */
/*                                                                                */
/*  Divisor is a polynomial from table A.1 based on the number of ECWords         */
/*--------------------------------------------------------------------------------*/
static
VOID
PolynomialDivision (UINT16 DividendCount, UINT8 *Dividend, UINT16 RemainderCount, UINT8 *Remainder) {

    UINT8       *TempRemainder;
    CONST UINT8 *Divisor;
    UINT8        temp;
    UINTN        sizeofnumbers;
    UINTN        i;
    UINTN        j;
    UINT8        stepMultiplier;

    Divisor = gGeneratorPolynomials [RemainderCount];
    if (Divisor == NULL) {
        DEBUG((DEBUG_ERROR,"Unable to locate generator polynomial for word count %d\n",RemainderCount));
        ASSERT(FALSE );
        return;
    }
    if (gFlags & QR_FLAGS_DEBUG_POLYDIVIDE) {
        DEBUG((DEBUG_INFO, "Divisor %3d  ", RemainderCount));
        for (j = 0; j < (UINTN)(RemainderCount+1); j++) {
            DEBUG((DEBUG_INFO," %3d,", Divisor[j]));
        }
        DEBUG((DEBUG_INFO,"\n"));
    }
    sizeofnumbers = DividendCount + RemainderCount;
    TempRemainder = AllocateZeroPool (sizeofnumbers );

    for (i = 0; i < DividendCount; i++) {
        TempRemainder[i] = Dividend[i];
    }

    if (gFlags & QR_FLAGS_DEBUG_POLYDIVIDE) {
        DEBUG((DEBUG_INFO, "MsgPly %3d   ", i));
        for (j = 0; j < sizeofnumbers; j++) {
            DEBUG((DEBUG_INFO," %3d,", TempRemainder[j]) );
        }
        DEBUG((DEBUG_INFO,"\n"));
    }

    for (i = 0; i < DividendCount; i++) {
        stepMultiplier = logTable[TempRemainder[i]]; // Lead term of message / previous result
        if (TempRemainder[i] == 0) {
            continue;
        }
        for (j = i; j < i + RemainderCount + 1; j++) {
            temp = alogTable[(stepMultiplier + Divisor[j - i]) % (GF256_SIZE - 1)];
            TempRemainder[j] ^= temp;  // XOR is Galois field Add
            if (gFlags & QR_FLAGS_DEBUG_POLYDIVIDE) {
                DEBUG((DEBUG_INFO,"--- s=%3d d=%3d t=%3d r=%3d\n",stepMultiplier,Divisor[j-i],temp,TempRemainder[j]));
            }
        }
        if (gFlags & QR_FLAGS_DEBUG_POLYDIVIDE) {
            DEBUG((DEBUG_INFO, "Result %3d   ", i));
            for (j = 0; j < sizeofnumbers; j++) {
                DEBUG((DEBUG_INFO," %3d,", TempRemainder[j]) );
            }
            DEBUG((DEBUG_INFO,"\n"));
        }
    }
    if (gFlags & QR_FLAGS_DEBUG_POLYDIVIDE) {
        DEBUG((DEBUG_INFO, "Result - "));
    }
    for (j = 0; j < RemainderCount; j++) {
        Remainder[j] = TempRemainder[j+DividendCount]; //Copy remainder
        if (gFlags & QR_FLAGS_DEBUG_POLYDIVIDE) {
            DEBUG((DEBUG_INFO," %3d,", Remainder[j]));
        }
    }
    if (gFlags & QR_FLAGS_DEBUG_POLYDIVIDE) {
        DEBUG((DEBUG_INFO,"\n"));
    }
    FreePool(TempRemainder);
}

/*--------------------------------------------------------------------------------*/
/* Encode bytes                                                                   */
/*--------------------------------------------------------------------------------*/
static
VOID
EncodeBytes (UINT8 *Data, UINTN DataLen) {
    UINTN   i;

    // The spec shows Table 6 ISO / IEC 8859-1 character set meaning for the data
    // but does not indicate any filtering of the data.  The assumption here is that
    // all binary data is allowed.

    for (i=0; i<DataLen; i++) {
        AddCodeWordBits (Data[i], 8 );
        if (gFlags & QR_FLAGS_DEBUG_ENCODING) {
            DEBUG((DEBUG_INFO," Binary %2d:%2d is %4d - ", i - 1, i, Data[i]));
            PrintBinary (Data[i], 8, '0');
            DEBUG((DEBUG_INFO,"\n"));
        }
    }
}

/*--------------------------------------------------------------------------------*/
/* Encode numeric data                                                            */
/*--------------------------------------------------------------------------------*/
static
VOID
EncodeNumeric (UINT8 *Data, UINTN DataLen) {
    UINT16 Triplet = 0;
    UINTN  TripIndx = 0;
    UINTN  i;

    for (i=0; i<DataLen; i++) {
        ASSERT ((Data[i] >= '0') && (Data[i] <= '9'));  // Checked earlier
        Triplet *= 10;
        Triplet += Data[i] - '0';
        TripIndx++;
        if (TripIndx == 3) {
            AddCodeWordBits (Triplet, 10);   // Three digits pack into 10 bits
            if (gFlags & QR_FLAGS_DEBUG_ENCODING) {
                DEBUG((DEBUG_INFO," Triplet %2d:%2d is %4d - ", i - 1, i, Triplet));
                PrintBinary (Triplet, 10, '0');
                DEBUG((DEBUG_INFO,"\n"));
            }
            Triplet = 0;
            TripIndx = 0;
        }
    }

    // Handle left over digits....
    if (TripIndx == 1) {
        AddCodeWordBits (Triplet, 4);        // One left over digits pack into 4 bits
        if (gFlags & QR_FLAGS_DEBUG_ENCODING) {
            DEBUG((DEBUG_INFO," Triplet %2d:%2d is %4d -       ", i - 1, i, Triplet));
            PrintBinary (Triplet, 4, '0');
            DEBUG((DEBUG_INFO,"\n"));
        }
    } else if (TripIndx == 2) {
        AddCodeWordBits (Triplet, 7);        // Two left over digits pack into 7 bits;
        if (gFlags & QR_FLAGS_DEBUG_ENCODING) {
            DEBUG((DEBUG_INFO," Triplet %2d:%2d is %4d -    ", i - 1, i, Triplet));
            PrintBinary (Triplet, 7, '0');
            DEBUG((DEBUG_INFO,"\n"));
        }
    }

}

/*--------------------------------------------------------------------------------*/
/* Encode Alphanumeric                                                            */
/*--------------------------------------------------------------------------------*/
static
VOID
EncodeAlphanumeric (UINT8 *Data, UINTN DataLen) {
    UINTN i;
    UINT8 letter;
    UINT16 Pair = 0;
    UINT16 index;

    for (i = 0; i < DataLen; i++)
    {
        letter = Data[i];
        for (index = 0; index < sizeof(gAlphaNumerics); index++) {
            if (letter == gAlphaNumerics[index]) {
                break;
            }
        }
        if (index == sizeof(gAlphaNumerics)) {
            DEBUG((DEBUG_ERROR,"Invalid charcter - was checked for valid earlier\n"));
            ASSERT (FALSE);
            return;
        }

        if (0 == (i % 2)) {
            Pair = index;
        } else {
            Pair = index + (UINT16) (Pair * 45);
            AddCodeWordBits(Pair, 11);
            if (gFlags & QR_FLAGS_DEBUG_ENCODING) {
                DEBUG((DEBUG_INFO," Pair %2d:%2d is %4d - ", i - 1, i, Pair));
                PrintBinary (Pair, 11, '0');
                DEBUG((DEBUG_INFO,"\n"));
            }
        }
    }

    if (1 == (i % 2)) {
        AddCodeWordBits(Pair, 6);
        if (gFlags & QR_FLAGS_DEBUG_ENCODING) {
            DEBUG((DEBUG_INFO," Pair  :%2d is%4d -      ", i, Pair));
            PrintBinary (Pair, 6, '0');
            DEBUG((DEBUG_INFO,"\n"));
        }
    }
}

/*--------------------------------------------------------------------------------*/
/* setBitmap - this draws a module at the next module location ISO 18004 7.7.3    */
/*--------------------------------------------------------------------------------*/
static
VOID
setBitmap (UINT8 *bitmap, INTN rowSize, UINT8 Color) {
    BOOLEAN done = FALSE;

    while (!done) {
        if (bitmap[row * rowSize + col] == QrGray) {
            bitmap[row * rowSize + col] = Color;
            done = TRUE;
        }

        if (up) {
            if (right) {
                col--;
                right = FALSE;
            } else {
                if (row > 0) {
                    col++;
                    row--;
                } else {
                    up = FALSE;
                    col--;
                    if (col == 6) {  // Column 7 is reserved
                        col = 5;
                    }
                }
                right = TRUE;
            }
        } else {
            if (right) {
                col--;
                right = FALSE;
            } else {
                if (row < (rowSize - 1)) {
                    col++;
                    row++;
                } else {
                    up = TRUE;
                    col--;
                    if (col == 6) { // Column 7 is reserved
                        col = 5;
                    }
                }
                right = TRUE;
            }
        }
    }
}

/*--------------------------------------------------------------------------------*/
/* drawBits - draws all of the data modules                                       */
/*--------------------------------------------------------------------------------*/
static
VOID
drawBits (UINT8 *bitmap, INTN rowSize) {
    UINTN   i;
    UINTN   data;
    UINT8   bit;
    UINTN   mask;

    up = TRUE;
    right = TRUE;
    row = rowSize - 1;
    col = rowSize - 1;

    for (i = 0; i< gBitStreamCount; i++ ) {

        data = gBitStream[i];
        mask = 0x80;

        for (mask = 0x80; mask != 0; mask >>= 1)
        {
            bit = (UINT8) (data & mask);
            setBitmap(bitmap, rowSize, (bit == 0) ? QrWhite : QrBlack);
        }
    }
    for (i = 0; i < gQrT->requiredRemainder; i++) {
        setBitmap(bitmap, rowSize, QrWhite);
    }
}

/*--------------------------------------------------------------------------------*/
/* drawHLine - draw a horizontal line left to right from x:y to x+tx-1:y          */
/*--------------------------------------------------------------------------------*/
static
VOID
drawHLine (UINT8 *bitmap, INTN s, INTN x, INTN y, INTN tx, UINT8 Color) {

    INTN i;
    INTN g;
    INTN b;

    g = y * s;       // Index of ROW to set bits into
    b = g + x;       // Index of starting point for the line
    for (i = x; i <= tx; i++) {
        if (b > (gQrBitmapLen)) {
            DEBUG((DEBUG_ERROR,__FUNCTION__ " Attempt to write module out of bitmap bounds\n"));
            ASSERT(FALSE);
        } else {
            bitmap[g + i] = Color;
        }
    }
}

/*--------------------------------------------------------------------------------*/
/* drawVLine - draw a vertical line top to bottom from x:y to x:y+ty-1            */
/*--------------------------------------------------------------------------------*/
static
VOID
drawVLine (UINT8 *bitmap, INTN s, INTN x, INTN y, INTN ty, UINT8 Color)
{
    INTN i;
    for (i = y; i <= ty; i++) {
        if ((x + (i * s)) > (gQrBitmapLen)) {
            DEBUG((DEBUG_ERROR,__FUNCTION__ " Attempt to write module out of bitmap bounds\n"));
            ASSERT(FALSE);
        } else {
            bitmap[x +( i * s)] = Color;
        }
    }
}

/*--------------------------------------------------------------------------------*/
/* drawReserved - mark all regions for final bits for module placement to skip    */
/*--------------------------------------------------------------------------------*/
static
VOID
drawReserved (UINT8 *bitmap, INTN rowSize) {

    drawVLine(bitmap, rowSize, 8, 0, 8, QrRsvd_E);
    drawHLine(bitmap, rowSize, 0, 8, 7, QrRsvd_E);
    drawVLine(bitmap, rowSize, 8, rowSize - 7, rowSize - 1, QrRsvd_E);
    drawHLine(bitmap, rowSize, rowSize-8, 8, rowSize - 1, QrRsvd_E);

    if (gQrVersion >= 7) { // Reserve the Version locations
        drawHLine(bitmap, rowSize, 0, rowSize - 11, 6, QrRsvd_E);
        drawHLine(bitmap, rowSize, 0, rowSize - 10, 6, QrRsvd_E);
        drawHLine(bitmap, rowSize, 0, rowSize -  9, 6, QrRsvd_E);
        drawVLine(bitmap, rowSize, rowSize - 11, 0, 6, QrRsvd_E);
        drawVLine(bitmap, rowSize, rowSize - 10, 0, 6, QrRsvd_E);
        drawVLine(bitmap, rowSize, rowSize -  9, 0, 6, QrRsvd_E);
    }
}

/*--------------------------------------------------------------------------------*/
/* drawFinder         - draw a finder pattern at x:y                              */
/* Finder Patters are a:                                                          */
/*               3x3 block box inside a                                           */
/*               5x5 white box inside a                                           */
/*               7x7 black box                                                    */
/*               with 2 sides of white border                                     */
/*--------------------------------------------------------------------------------*/
static
VOID
drawFinder (UINT8 *bitmap, INTN rowSize, INTN x, INTN y) {

    drawHLine(bitmap, rowSize, x,     y,     x + 6, QrBlack_E);
    drawHLine(bitmap, rowSize, x,     y + 6, x + 6, QrBlack_E);
    drawVLine(bitmap, rowSize, x,     y + 1, y + 6, QrBlack_E);
    drawVLine(bitmap, rowSize, x + 6, y + 1, y + 6, QrBlack_E);

    drawHLine(bitmap, rowSize, x + 1, y + 1, x + 5, QrWhite_E);
    drawHLine(bitmap, rowSize, x + 1, y + 5, x + 5, QrWhite_E);
    drawVLine(bitmap, rowSize, x + 1, y + 2, y + 5, QrWhite_E);
    drawVLine(bitmap, rowSize, x + 5, y + 2, y + 5, QrWhite_E);

    drawHLine(bitmap, rowSize, x + 2, y + 2, x + 4, QrBlack_E);
    drawHLine(bitmap, rowSize, x + 2, y + 3, x + 4, QrBlack_E);
    drawHLine(bitmap, rowSize, x + 2, y + 4, x + 4, QrBlack_E);


    if (y != 0) {
         drawHLine(bitmap, rowSize, x    , y - 1, x + 7, QrWhite_E);
         drawVLine(bitmap, rowSize, x + 7, y    , y + 6, QrWhite_E);
    } else {
        if (x == 0) {
            drawVLine(bitmap, rowSize, x + 7, y    , y + 7, QrWhite_E);
            drawHLine(bitmap, rowSize, x    , y + 7, x + 6, QrWhite_E);
        } else {
            drawVLine(bitmap, rowSize, x - 1, y,     y + 7, QrWhite_E);
            drawHLine(bitmap, rowSize, x    , y + 7, x + 6, QrWhite_E);
        }
    }
}

/*--------------------------------------------------------------------------------*/
/* drawAlignment - draw an alignment patter at x:y if the area is free            */
/*     Alignment Patters are                                                      */
/*               single black module inside a                                     */
/*               3x3 white box inside a                                           */
/*               5x5 black box                                                    */
/*--------------------------------------------------------------------------------*/
static
VOID
drawAlignment (UINT8 *bitmap, INTN rowSize, UINT8 x, UINT8 y) {
    INTN    i;
    INTN    j;

    x -= 2;  // Adjust drawing location so x:y marks the center of the mark
    y -= 2;
    // Check to see if the area for the alignment patter is free
    for (i = y; i < y + 5; i++) {
        for (j = x; j < x + 5; j++) {
            if (bitmap[(i * rowSize) + j] != QrGray) {
                return;  // No Alignment pattern here
            }
        }
    }

    drawHLine(bitmap, rowSize, x,     y    , x + 4,QrBlack_E);
    drawHLine(bitmap, rowSize, x,     y + 4, x + 4,QrBlack_E);
    drawVLine(bitmap, rowSize, x,     y + 1, y + 4,QrBlack_E);
    drawVLine(bitmap, rowSize, x + 4, y + 1, y + 4,QrBlack_E);
    drawHLine(bitmap, rowSize, x + 1, y + 1, x + 3, QrWhite_E);
    drawHLine(bitmap, rowSize, x + 1, y + 2, x + 3, QrWhite_E);
    drawHLine(bitmap, rowSize, x + 1, y + 3, x + 3, QrWhite_E);
    bitmap[x + 2 + ((y + 2)* rowSize)] = QrBlack_E;
}

/*--------------------------------------------------------------------------------*/
/* drawHTiming - draw the horizontal timing pattern from x:y to x+tx-1:y          */
/*--------------------------------------------------------------------------------*/
static
VOID
drawHTiming (UINT8 *bitmap, INTN rowSize, INTN x, INTN y, INTN tx) {
    INTN    i;
    INTN    b;
    BOOLEAN IsWhite;

    b = (y - 1) * rowSize;
    IsWhite = FALSE;

    for (i = x; i < tx; i ++) {
        bitmap[b + i] = (IsWhite) ? QrWhite_E : QrBlack_E;
        IsWhite = !IsWhite;
    }
}

/*--------------------------------------------------------------------------------*/
/* drawVTiming - draw the vertical timing pattern from x:y to x:y+ty-1            */
/*--------------------------------------------------------------------------------*/
static
VOID
drawVTiming (UINT8 *bitmap, INTN rowSize, INTN x, INTN y, INTN ty) {
    INTN     i;
    BOOLEAN  IsWhite;

    IsWhite = FALSE;

    for (i = y; i < ty; i ++) {
        bitmap[(i - 1) * rowSize + x] = (IsWhite) ? QrWhite_E : QrBlack_E;
        IsWhite = !IsWhite;
    }
}

/*--------------------------------------------------------------------------------*/
/* Evaluate 1                                                                     */
/*         Compute a penalty based on runs of horizontal or vertical cells        */
/*         of the same color.                                                     */
/*                                                                                */
/*         5 points for 5 in a row.                                               */
/*         1 additional point for each additional module of the same color        */
/*--------------------------------------------------------------------------------*/
static
INTN
Evaluate1 (UINT8 *Bitmap, INTN RowSize) {
    INTN  Penalty;
    INTN  x,y;
    INTN  AdjacentCount;
    INTN  RowOffset;
    UINT8 Cell1;
    UINT8 Cell2;

    Penalty = 0;

    //
    // Check each row for adjacent cells the same color.
    //
    for (y = 0; y < RowSize; y++) {           // Check every Row
        AdjacentCount = 0;
        RowOffset = y * RowSize;
        Cell1 = Bitmap[RowOffset] & ~QrExclude;
        if (Cell1 == QrRsvd) {
            Cell1 = QrWhite;
        }

        for (x = 1; x < RowSize; x++) {  // Check every cell in the row
            Cell2 = Bitmap[RowOffset + x ] & ~QrExclude;
            if (Cell2 == QrRsvd) {
                Cell2 = QrWhite;
            }
            if (Cell1 == Cell2) {
                AdjacentCount++;
                if (AdjacentCount == 4) {   // 4th adjacent module is 5 in a row
                    Penalty += 3;
                } else if (AdjacentCount > 4) {
                    Penalty++;
                }
            } else {
                AdjacentCount = 0;
            }
            Cell1 = Cell2;
        }
    }

    //
    // Check each column for adjacent cells the same color.
    //
    for (x = 0; x < RowSize; x++) {            // Check every column
        AdjacentCount = 0;
        Cell1 = Bitmap[x] & ~QrExclude;
        if (Cell1 == QrRsvd) {
            Cell1 = QrWhite;
        }
        for (y = 1; y < RowSize; y++) {  // Check every cell in the column
            Cell2 = Bitmap[(y * RowSize) + x] & ~QrExclude;
            if (Cell2 == QrRsvd) {
                Cell2 = QrWhite;
            }
            if (Cell1 == Cell2) {
                AdjacentCount++;
                if (AdjacentCount == 4) {   // 4th adjacent module is 5 in a row
                    Penalty += 3;
                } else if (AdjacentCount > 4) {
                    Penalty++;
                }
            } else {
                AdjacentCount = 0;
            }
            Cell1 = Cell2;
        }
    }

    DEBUG((DEBUG_INFO,"Evaluate 1 penalty is %d\n",Penalty));
    return Penalty;
}

/*--------------------------------------------------------------------------------*/
/* Evaluate 2                                                                     */
/*         Scan rows and colums looking for 2x2 collections of modules the        */
/*         of the same color.                                                     */
/*                                                                                */
/*         3 points for each occurrence.                                          */
/*         FYI - a 3x3 collection contains 4 2x2 collections - worth 12 points    */
/*--------------------------------------------------------------------------------*/
static
INTN
Evaluate2 (UINT8 *Bitmap, INTN RowSize) {
    INTN  Penalty;
    INTN  x,y;
    UINT8 Cell1;
    UINT8 Cell2;
    UINT8 Cell3;
    UINT8 Cell4;

    Penalty = 0;

    //
    // Check each row for 2x2 collections of the same color.
    //
    for (y = 0; y < (RowSize - 1); y++) {      // Check two cells vertially and two cells horizontal
        for (x = 0; x < (RowSize - 1); x++) {  // Checking two cells - so stop one short
            Cell1 = Bitmap[(y * RowSize) + x] & ~QrExclude;
            if (Cell1 == QrRsvd) {
                Cell1 = QrWhite;
            }
            Cell2 = Bitmap[(y * RowSize) + x + 1] & ~QrExclude;
            if (Cell2 == QrRsvd) {
                Cell2 = QrWhite;
            }
            if (Cell1 == Cell2) {
                Cell3 = Bitmap[((y + 1) * RowSize) + x] & ~QrExclude;
                if (Cell3 == QrRsvd) {
                    Cell3 = QrWhite;
                }
                if (Cell1 == Cell3) {
                    Cell4 = Bitmap[((y + 1) * RowSize) + x + 1] & ~QrExclude;
                    if (Cell4 == QrRsvd) {
                        Cell4 = QrWhite;
                    }
                    if (Cell1 == Cell4) {
                            Penalty += 3;
                    }
                }
            }
        }
    }

    DEBUG((DEBUG_INFO,"Evaluate 2 penalty is %d\n",Penalty));
    return Penalty;
}

/*--------------------------------------------------------------------------------*/
/* Evaluate 3                                                                     */
/*         Look for the specific sequence of                                      */
/*                                                                                */
/*         1 0 1 1 1 0 1 0 0 0 0          or   B W B B B Q B W W W W              */
/*         0 0 0 0 1 0 1 1 1 0 1               W W W W B W B B B W B              */
/*                                                                                */
/*         Either horizontally or vertically                                      */
/*         Add a penalty of 40 points for each occurrence.                        */
/*--------------------------------------------------------------------------------*/
static
INTN
Evaluate3 (UINT8 *Bitmap, INTN RowSize) {
    INTN  Penalty;
    INTN  x,y;
    UINT8 Cell;
    INTN  Target1Index;
    INTN  Target2Index;
    UINT8 Target1[]  = {QrBlack,QrWhite,QrBlack,QrBlack,QrBlack,QrWhite,QrBlack,QrWhite,QrWhite,QrWhite,QrWhite};
    UINT8 Target2[]  = {QrWhite,QrWhite,QrWhite,QrWhite,QrBlack,QrWhite,QrBlack,QrBlack,QrBlack,QrWhite,QrBlack};

    Penalty = 0;

    //
    // Check each row for the special patters.
    //
    for (y = 0; y < RowSize; y++) {             // Check every Row
        Target1Index = 0;
        Target2Index = 0;
        for (x = 0; x < (RowSize - 11); x++) {  // Checking mutiple cells

            Cell = Bitmap[(y * RowSize) + x] & ~QrExclude;
            if (Cell == QrRsvd) {
                Cell = QrWhite;
            }
            if (Cell == Target1[Target1Index]) {
                Target1Index++;
                if (Target1Index == 10) {
                    Penalty += 40;
                    Target1Index = 0;
                    DEBUG((DEBUG_INFO,"Found horizontal pattern 1 at %d:%d\n",y,x));
                }

            } else {
                Target1Index = 0;
            }
            if (Cell == Target2[Target2Index]) {
                Target2Index++;
                if (Target2Index == 10) {
                    Penalty += 40;
                    Target2Index = 0;
                    DEBUG((DEBUG_INFO,"Found horizontal pattern 2 at %d:%d\n",y,x));
                }
            } else {
                Target2Index = 0;
            }
        }
    }
    //
    // Check each column for the special patterns
    //
    for (y = 0; y < RowSize; y++) {             // Check every Row
        Target1Index = 0;
        Target2Index = 0;
        for (x = 0; x < (RowSize - 11); x++) {  // Checking mutiple cells

            Cell = Bitmap[(y * RowSize) + x] & ~QrExclude;
            if (Cell == QrRsvd) {
                Cell = QrWhite;
            }
            if (Cell == Target1[Target1Index]) {
                Target1Index++;
                if (Target1Index == 10) {
                    Penalty += 40;
                    Target1Index = 0;
                    DEBUG((DEBUG_INFO,"Found vertical pattern 1 at %d:%d\n",y,x));
                }

            } else {
                Target1Index = 0;
            }
            if (Cell == Target2[Target2Index]) {
                Target2Index++;
                if (Target2Index == 10) {
                    Penalty += 40;
                    Target2Index = 0;
                    DEBUG((DEBUG_INFO,"Found vertical pattern 2 at %d:%d\n",y,x));
                }
            } else {
                Target2Index = 0;
            }
        }
    }

    DEBUG((DEBUG_INFO,"Evaluate 3 penalty is %d\n",Penalty));
    return Penalty;
}

/*--------------------------------------------------------------------------------*/
/* Evaluate 4                                                                     */
/*         Check for ratio of black to white modules                              */
/*                                                                                */
/*         For every 5% deviation, add 10 points                                  */
/*         eg. 45% to 55% == 0 points                                             */
/*         eg. 40% to 60% == 10 points                                            */
/*         FYI - a 3x3 collection contains 4 2x2 collections - worth 12 points    */
/*--------------------------------------------------------------------------------*/
static
INTN
Evaluate4 (UINT8 *Bitmap, INTN RowSize) {
    INTN  Penalty;
    INTN  x,y;
    UINT8 Cell;
    INTN  CountOfBlack;
    INTN  TotalCount;
    INTN  Ratio;

    TotalCount = RowSize * RowSize;
    CountOfBlack = 0;
    for (y = 0; y < RowSize; y++) {
        for (x = 0; x < RowSize; x++) {
            Cell = Bitmap[(y * RowSize) + x] & ~QrExclude;
            // gQrRsvd is treated as White
            if (Cell == QrBlack) {
                CountOfBlack++;
            }
        }
    }
    Ratio = ((CountOfBlack * 100) / TotalCount) - 50;
    if (Ratio < 0) {
        Ratio = - Ratio;
    }
    Penalty = 10 * (Ratio / 5);

    DEBUG((DEBUG_INFO,"Evaluate 4 penalty is %d, based on TC=%d, CB=%d, R=%d\n",Penalty,TotalCount,CountOfBlack,Ratio));

    return Penalty;
}


/*--------------------------------------------------------------------------------*/
/* Step 1. Data Analysis                                                          */
/*         Analyze input data to identify the characteristics of the data         */
/*--------------------------------------------------------------------------------*/
static
EFI_STATUS
Step1_Process (UINT8 *Data, UINT16 DataLen, INTN RegionSize) {
    QRENCODING  suggestedMode;
    UINT8       suggestedQrVersion;


    suggestedMode = CheckEncodingType(Data,DataLen);

    if (gQrMode  < suggestedMode) {
        if (gQrMode == QrAutoMode) {
            gQrMode = suggestedMode;
        } else {
            DEBUG((DEBUG_ERROR,"Suggested mode %d is larger than requested mode %d\n", suggestedMode, gQrMode));
            return EFI_INVALID_PARAMETER;
        }
    }

    suggestedQrVersion = CheckQrVersion(Data,DataLen);

    if (gQrVersion < suggestedQrVersion) {
        if (gQrVersion == QrAutoVersion) {  // Automatic selection
            gQrVersion = suggestedQrVersion;
        } else {
            DEBUG((DEBUG_INFO,"Suggested version %d is larger than requested version %d\n", suggestedQrVersion, gQrVersion));
            return EFI_INVALID_PARAMETER;
        }
    }

    // Validate that Version and Mode are currect after applying suggested values;
    if ((gQrVersion < QrMinVersion) || (gQrVersion > QrMaxVersion)) {  // ISO 18004:2015 Qr Versions supported
        DEBUG((DEBUG_INFO,"Suggested version %d is not 1<=QrVersion<=40\n", gQrVersion));
        return EFI_INVALID_PARAMETER;
    }
    if ((gQrMode <= QrAutoMode) || (gQrMode > QrByteMode)) {  // Only support Num/Alpha/Byte for now
        DEBUG((DEBUG_INFO,"Suggested QrMode %d is not supported\n", gQrMode));
        return EFI_INVALID_PARAMETER;
    }

    DEBUG((DEBUG_INFO,"QrVersion is %d\n", gQrVersion));
    gQrSize = gQrVersion * 4 + 17;  // Rule from ISO 18004.
    if (RegionSize < (gQrSize + (2 * QR_QUIET_ZONE))) {
        DEBUG((DEBUG_ERROR,"Region size %d for QR code size %d is too small\n",RegionSize,gQrSize));
        return EFI_INVALID_PARAMETER;
    }

    gQrBitmapLen = gQrSize * gQrSize;
    gQrBitmap = AllocateZeroPool (gQrBitmapLen);  // Initialize to "gray"

    gQrT = &gQrTable[(gQrVersion - 1) * 4 + gQrLevel - 1]; // QrT points to the table entry to use;

    DEBUG((DEBUG_INFO,"Using QrCode=%d (%dx%d), Mode=%d, ECLevel=%d\n", gQrVersion, gQrSize, gQrSize, gQrMode, gQrLevel));

    DEBUG((DEBUG_INFO,"entry   %d %d %d %d %d %d %d %d %d %d %d\n",
         gQrT->totalWords,
         gQrT->ECWordsPerBlock,
         gQrT->group1BlockCount,
         gQrT->group1Words,
         gQrT->group2BlockCount,
         gQrT->group2Words,
         gQrT->requiredRemainder,
         gQrT->maxNumeric,
         gQrT->maxAlphanumeric,
         gQrT->maxBytes,
         gQrT->maxKanji
        ));

    return EFI_SUCCESS;
}

#define ISO_NUMERIC_CODE      0x01  // 0001  ISO Spec 18004:2015 Table 2 defines encoding values
#define ISO_ALPHANUMERIC_CODE 0x02  // 0010
#define ISO_BYTE_CODE         0x04  // 0100

/*--------------------------------------------------------------------------------*/
/* Step 2. Data Encoding                                                          */
/*         Convert the characters to a bit stream in accordance with the QR Code, */
/*         the encoding mode, and the EC lavel.                                   */
/*--------------------------------------------------------------------------------*/
static
EFI_STATUS
Step2_Process (UINT8 *Data, UINTN DataLen ) {
    UINTN  lengthBits = 0;

    if (gQrVersion <= gLengthBits[0][0]) {
        lengthBits = gLengthBits[0][gQrMode];
    } else if (gQrVersion <= gLengthBits[1][0]) {
        lengthBits = gLengthBits[1][gQrMode];
    } else {
        lengthBits = gLengthBits[2][gQrMode];
    }

    InitCodeWords (gQrT->totalWords);

    switch (gQrMode)
    {
        case QrNumericMode:
            AddCodeWordBits(ISO_NUMERIC_CODE, 4);
            AddCodeWordBits(DataLen, lengthBits);
            EncodeNumeric(Data,DataLen);
            break;

        case QrAlphaNumericMode:
            AddCodeWordBits(ISO_ALPHANUMERIC_CODE, 4);
            AddCodeWordBits(DataLen, lengthBits);
            EncodeAlphanumeric(Data,DataLen);
            break;

        case QrByteMode:
            AddCodeWordBits(ISO_BYTE_CODE, 4);
            AddCodeWordBits(DataLen, lengthBits);
            EncodeBytes(Data,DataLen);
            break;

        default:
            DEBUG((DEBUG_ERROR,"Unsupported mode %d\n", gQrMode));
            ASSERT(FALSE);
            break;
    }

    DEBUG((DEBUG_INFO,"Adding terminator bits. They are allowed not to fit, so ignore AddWords error on this call\n"));

    AddCodeWordBits(0, 4);   // Terminating 0000's as required - if they fit
    AddCodeWordPadBytes ();

    return EFI_SUCCESS;
}

/*--------------------------------------------------------------------------------*/
/* Step 3. Error Correction Encoding                                              */
/*         Divide the codeword sequence into blocks as per table 9, and generate  */
/*         the error correction codewords for each block.                         */
/*         the encoding mode, and the EC lavel.                                   */
/*--------------------------------------------------------------------------------*/
static
EFI_STATUS
Step3_Process ( VOID ) {
    UINTN   i;
    UINTN   ECWordIndex;
    UINTN   CodeWordIndex;
    UINTN   ECWords;
    UINT16  ECWordsPerBlock;
    UINTN   Blocks;

    Blocks = gQrT->group1BlockCount + gQrT->group2BlockCount;
    ECWordsPerBlock = gQrT->ECWordsPerBlock;
    ECWords = ECWordsPerBlock * Blocks;
    CodeWordIndex = 0;
    ECWordIndex = 0;

    gECWords = AllocatePool (ECWords);
    gECWordCount = ECWords;
    ASSERT (NULL != gECWords);
    if (NULL == gECWords) {
        return EFI_OUT_OF_RESOURCES;
    }

    //
    // Compute EC Words for every block
    //
    for (i = 0; i < gQrT->group1BlockCount; i++) {   // Process Group 1 blocks
         PolynomialDivision ( gQrT->group1Words, &gCodeWords[CodeWordIndex], ECWordsPerBlock, &gECWords[ECWordIndex] );
         CodeWordIndex += gQrT->group1Words;
         ECWordIndex += ECWordsPerBlock;
    }
    for (i = 0; i < gQrT->group2BlockCount; i++) {   // Process Group 2 blocks
         PolynomialDivision ( gQrT->group2Words, &gCodeWords[CodeWordIndex], ECWordsPerBlock, &gECWords[ECWordIndex] );
         CodeWordIndex += gQrT->group2Words;
         ECWordIndex += ECWordsPerBlock;
    }

    /*

     From Making hint at Thonky.com QRCode

     CodeWord    0 is   32 - 00100000      v
     CodeWord    1 is   91 - 01011011      v
     CodeWord    2 is   11 - 00001011      v
     CodeWord    3 is  120 - 01111000      v
     CodeWord    4 is  209 - 11010001      v
     CodeWord    5 is  114 - 01110010      v
     CodeWord    6 is  220 - 11011100      v
     CodeWord    7 is   77 - 01001101      v
     CodeWord    8 is   67 - 01000011      v
     CodeWord    9 is   64 - 01000000      v
     CodeWord   10 is  236 - 11101100      v
     CodeWord   11 is   17 - 00010001      v
     CodeWord   12 is  236 - 11101100           EC is computed correctly, but this word set to 0
                                                before code words printed
     CodeWord[12] set to 0 to match web page masking sample
     EC Word     0 is  168 - 10101000
     EC Word     1 is   72 - 01001000
     EC Word     2 is   22 - 00010110
     EC Word     3 is   82 - 01010010
     EC Word     4 is  217 - 11011001
     EC Word     5 is   54 - 00110110
     EC Word     6 is  156 - 10011100
     EC Word     7 is    0 - 00000000
     EC Word     8 is   46 - 00101110
     EC Word     9 is   15 - 00001111
     EC Word    10 is  180 - 10110100
     EC Word    11 is  122 - 01111010
     EC Word    12 is   16 - 00010000

    */


    if ((gQrVersion == 1) &&                      // Specific data to match we site samples
        (gQrMode    == QrAlphaNumericMode) &&     // of masking.  The underlying data at that
        (gQrLevel   == QrECLevel_Q) &&            // site is invalid (IMHO) due to incorrect padding
        (gFlags & QR_FLAGS_DEBUG_MASKING)) {
        gCodeWords[12] = 0;                       // Codeword is incorrect on Web page
        DEBUG((DEBUG_INFO,"CodeWord[12] set to 0 to match web page masking sample\n"));
    }

    if (gFlags & QR_FLAGS_DEBUG_CODE_WORDS) {
        DEBUG((DEBUG_INFO,"ECWords=%d, ECWordIndex=%d\n",ECWords,ECWordIndex));
        for (i = 0; i < ECWords; i++)
        {
            DEBUG((DEBUG_INFO," EC Word %4d is %4d - ", i, gECWords[i]));
            PrintBinary (gECWords[i], 8, '0');
            DEBUG((DEBUG_INFO,"\n"));
        }
    }
    return EFI_SUCCESS;
}

/*--------------------------------------------------------------------------------*/
/* Step 4. Structure the final message                                            */
/*         Interleave the data and codewords from each block and add remainder    */
/*         bits if necessary                                                      */
/*--------------------------------------------------------------------------------*/
static
EFI_STATUS
Step4_Process ( VOID ) {
    UINTN   StreamWordIndex;
    UINTN   i;
    UINTN   Indx;
    UINTN   MaxIndx;
    UINTN   Blocks;
    UINTN   G2Base;

    gBitStreamCount = gCodeWordCount + gECWordCount;
    gBitStream = AllocatePool (gBitStreamCount);
    StreamWordIndex = 0;

    Blocks = gQrT->group1BlockCount + gQrT->group2BlockCount;

    MaxIndx = gQrT->group1Words;
    if (MaxIndx < gQrT->group2Words) {
        MaxIndx = gQrT->group2Words;
    }
    G2Base =  gQrT->group1BlockCount * gQrT->group1Words;
    //
    //  Interleave the bitstream followed by interleaved EC words
    //
    for (Indx = 0; Indx < MaxIndx; Indx++) {
        for (i = 0; i < Blocks; i++) {
            if (i < gQrT->group1BlockCount) {
                if (Indx < gQrT->group1Words) {
                    gBitStream[StreamWordIndex++] = gCodeWords[(i * gQrT->group1Words) + Indx];
                }
            } else {
                if (Indx < gQrT->group2Words) {
                    gBitStream[StreamWordIndex++] = gCodeWords[G2Base + ((i - gQrT->group1BlockCount) * gQrT->group2Words) + Indx];
                }
            }
        }
    }
    for (Indx = 0; Indx < gQrT->ECWordsPerBlock; Indx++) {
        for (i = 0; i < Blocks; i++) {
            gBitStream[StreamWordIndex++] = gECWords[(i * gQrT->ECWordsPerBlock) + Indx];
        }
    }

    if (gFlags & QR_FLAGS_DEBUG_BIT_STREAM) {
        for (i = 0; i < gBitStreamCount; i++)
        {
            DEBUG((DEBUG_INFO," BitStream %4d is %4d - ", i, gBitStream[i]));
            PrintBinary (gBitStream[i], 8, '0');
            DEBUG((DEBUG_INFO,"\n"));
        }
    }
    //
    //  Delay remainder bits to a later step
    //

    return EFI_SUCCESS;
}

/*--------------------------------------------------------------------------------*/
/* Step 5. Module placement in matrix                                             */
/*         Place the finder pattern                                               */
/*         Place the separators                                                   */
/*         Place the timing patterns                                              */
/*         Place the alignment patterns (if required)                             */
/*         Place the codewords into the Matrix                                    */
/*--------------------------------------------------------------------------------*/
static
EFI_STATUS
Step5_Process ( VOID ) {
    INTN   i;
    INTN   j;

    // All QR Codes get the same size finder in the upper left, upper right, and lower left cornet
    // along with a single black module next to the lower left Finder.
    drawFinder(gQrBitmap, gQrSize, 0, 0);
    drawFinder(gQrBitmap, gQrSize, gQrSize - 7, 0);
    drawFinder(gQrBitmap, gQrSize, 0, gQrSize - 7);

    gQrBitmap[ ((4 * gQrVersion) + 9)  * gQrSize + 8 ] = QrBlack_E;

    // All QR Code > version 1 get alignment patterns
    if (gQrVersion > 1) {
        for (i = 0; i < QR_MAX_LOCATIONS; i++)
        {
            if (gAlignmentLocations[gQrVersion-2][i] == 0)
            {
                break;
            }
            for (j = 0; j < QR_MAX_LOCATIONS; j++)
            {
                if (gAlignmentLocations[gQrVersion-2][j] == 0)
                {
                    break;
                }
                drawAlignment(gQrBitmap, gQrSize, gAlignmentLocations[gQrVersion-2][i], gAlignmentLocations[gQrVersion-2][j]);
                drawAlignment(gQrBitmap, gQrSize, gAlignmentLocations[gQrVersion-2][j], gAlignmentLocations[gQrVersion-2][i]);
            }
        }
    }

    drawReserved (gQrBitmap, gQrSize);

    drawVTiming(gQrBitmap, gQrSize, 6, 9, gQrSize - 7);
    drawHTiming(gQrBitmap, gQrSize, 8, 7, gQrSize - 8);

    drawBits(gQrBitmap, gQrSize);

    return EFI_SUCCESS;
}
/*--------------------------------------------------------------------------------*/
/* Step 6. Data Masking                                                           */
/*         Apply the 8 data masking patterns and evaluate each pattern for quality*/
/*         Choose the pattern with the best quality                               */
/*--------------------------------------------------------------------------------*/
static
EFI_STATUS
Step6_Process ( VOID ) {

    UINT8    *TestBitmaps[QR_MASK_PATTERNS];
    INTN      TestPenalty[QR_MASK_PATTERNS];
    BOOLEAN   flip = FALSE;
    UINT8    *Temp;
    INTN      MinPenalty = MAX_INTN;
    INTN      MinPenaltyIndex = QR_MASK_PATTERNS;
    INTN      k,Row,Column;
    UINT8     Cell;

    //Get 8 copies of the bitmap to apply mask patterns to.
    for (k = 0; k < QR_MASK_PATTERNS; k++) {
        DEBUG((DEBUG_INFO,"Processing pattern %d\n",k));
        TestBitmaps[k] = AllocateCopyPool (gQrBitmapLen, gQrBitmap);

        for (Row = 0; Row < gQrSize; Row++) {              // i is Row
            for (Column = 0; Column < gQrSize; Column++) {          // j is Column
                switch (k) {
                case 0:     /* Data Mask Reference 000 */
                    flip =  0 == ((Row + Column) % 2);      // (i + j) mod 2 = 0
                    break;
                case 1:     /* Data Mask Reference 001 */
                    flip =  0 == (Row % 2);            // i mod 2 = 0
                    break;
                case 2:     /* Data Mask Reference 010 */
                    flip =  0 == (Column % 3);            // j mod 3 = 0
                    break;
                case 3:     /* Data Mask Reference 011 */
                    flip =  0 == ((Row + Column) % 3);      // (i + j) mod 3 = 0
                    break;
                case 4:     /* Data Mask Reference 100 */
                    flip =  0 == ((Row / 2) + (Column / 3)) % 2; // ((i div 2) + ( j div 3)) mod 2 = 0
                    break;
                case 5:     /* Data Mask Reference 101 */
                    flip =  0 == ((Row * Column) % 2) + ((Row * Column) % 3) ; // (i j) mod 2 + (i j) mod 3 = 0
                    break;
                case 6:     /* Data Mask Reference 110 */
                    flip =  0 == (((Row * Column) % 2) + ((Row * Column) % 3)) % 2;  // ((i j) mod 2 + (i j) mod 3) mod 2 = 0
                    break;
                case 7:     /* Data Mask Reference 111 */
                    flip =  0 == (((Row + Column) % 2) + ((Row * Column) % 3)) % 2;    // ((i+j) mod 2 + (i j) mod 3) mod 2 = 0
                    break;
                }

                if (gFlags & QR_FLAGS_DEBUG_MASK_ONLY) {   // Draw the Masking patters from ISO Spec Fig 21
                    Cell = TestBitmaps[k][Row * gQrSize + Column];
                    if ( 0 != (Cell & QrExclude)) {        // If a module is "excluded", draw as gray
                        if (flip) {
                            Cell = QrGray;
                        } else {
                            Cell = QrWhite;
                        }
                    } else {
                        if (flip) {
                            Cell = QrBlack;
                        } else {
                            Cell = QrWhite;
                        }
                    }
                    TestBitmaps[k][Row * gQrSize + Column] = Cell;
                } else {
                    if (flip) {
                        Cell = TestBitmaps[k][Row * gQrSize + Column];
                        if ( 0 == (Cell & QrExclude)) {        // If a module is "excluded", don't flip the bit.
                            if (Cell == QrWhite) {
                                Cell = QrBlack;
                            } else {
                                Cell = QrWhite;
                            }
                            TestBitmaps[k][Row * gQrSize + Column] = Cell;
                        }
                    }
                }
            }
        }

        TestPenalty[k]  = Evaluate1 (TestBitmaps[k],gQrSize);
        TestPenalty[k] += Evaluate2 (TestBitmaps[k],gQrSize);
        TestPenalty[k] += Evaluate3 (TestBitmaps[k],gQrSize);
        TestPenalty[k] += Evaluate4 (TestBitmaps[k],gQrSize);
        if (MinPenalty > TestPenalty[k]) {
            MinPenalty = TestPenalty[k];
            MinPenaltyIndex = k;
        }
    }

    DEBUG((DEBUG_INFO, "Minimum penalty is %d from index %d\n",MinPenalty, MinPenaltyIndex));
    ASSERT (MinPenaltyIndex < QR_MASK_PATTERNS);
    if (gFlags & QR_FLAGS_NO_MASK) {
        DEBUG((DEBUG_INFO, "Not using mask\n"));
    } else {
        if (gFlags & QR_FLAGS_MASK_SEL) {
            MinPenaltyIndex = gFlags & 0x07;
        }
        // Swap the "best" masked bitmap with gQrBitmap, and free the 8 unused bitmaps.
        Temp = TestBitmaps[MinPenaltyIndex];
        TestBitmaps[MinPenaltyIndex] = gQrBitmap;
        gQrBitmap = Temp;
        gQrMask = MinPenaltyIndex;
        DEBUG((DEBUG_INFO, "Using mask %d\n",MinPenaltyIndex));
    }

    for (k = 0; k < QR_MASK_PATTERNS; k++) {
        FreePool (TestBitmaps[k]);
    }

    return EFI_SUCCESS;
}

/*--------------------------------------------------------------------------------*/
/* Step 7. Format and version information                                         */
/*         Generate the format information and version information and complete   */
/*         the symbol.                                                            */
/*--------------------------------------------------------------------------------*/
static
EFI_STATUS
Step7_Process ( VOID ) {

    UINT16  FormatInfo;
    UINT32  VersionInfo;
    INTN    t;
    INTN    i;
    INTN    j;
    UINT16  Mask;
    UINT32  VMask;

    FormatInfo = gFormatInfo[gQrLevel-1][gQrMask];

    DEBUG((DEBUG_INFO," FormatInfo %x - ", FormatInfo));
    PrintBinary (FormatInfo, 15, '0');
    DEBUG((DEBUG_INFO,"\n"));
    if (gFlags & QR_FLAGS_DEBUG_MASK_ONLY) {     // Don't draw Format or version info
        return EFI_SUCCESS;
    }

    Mask = 0x0001;
    t = 0;
    for (i = 0; i < 8; i++) {
        if (i == 6) { // Skip the timing pattern
            t = 1;
        }
        gQrBitmap[ (i + t) * gQrSize + 8] = (Mask & FormatInfo) ? QrBlack : QrWhite;
        gQrBitmap[ 9 * gQrSize - 1 - i ]  = (Mask & FormatInfo) ? QrBlack : QrWhite;
        Mask <<= 1;
    }
    t = 1;
    for (i = 0; i < 7; i++) {
        if (i == 1) { // Skip the timing pattern
            t = 0;
        }
        gQrBitmap[8 * gQrSize + 6 - i + t]         = (Mask & FormatInfo) ? QrBlack : QrWhite;
        gQrBitmap[(gQrSize - 7 + i) * gQrSize + 8] = (Mask & FormatInfo) ? QrBlack : QrWhite;
        Mask <<= 1;
    }

    if (gQrVersion > 6) {
        VersionInfo = gVersionInfo[gQrVersion-7];  // Table starts at version 7
        VMask = 0x00001;
        DEBUG((DEBUG_INFO," VersionInfo  %x - ", VersionInfo));
        PrintBinary (VersionInfo, 18, '0');
        DEBUG((DEBUG_INFO,"\n"));

        // Write the VersionInfo data to the lower block
        for (j=0;j<6;j++) {
           for (i=0; i<3; i++) {    // Enter data column by column
                // Lower Left block
                gQrBitmap[(gQrSize - 11 + i) * gQrSize + j] = (VMask & VersionInfo) ? QrBlack : QrWhite;
                // Upper Right block
                gQrBitmap[(gQrSize * j) + gQrSize - 11 + i] = (VMask & VersionInfo) ? QrBlack : QrWhite;
                VMask <<= 1;
            }
        }
    }
    return EFI_SUCCESS;
}

/*--------------------------------------------------------------------------------*/
/* Step 8. Build the Gop->Blt ready bitmap                                        */
/*         Generate the format information and version information and complete   */
/*         the symbol.                                                            */
/*--------------------------------------------------------------------------------*/
static
EFI_STATUS
Step8_Process ( INTN RegionSize ) {
    INTN                            Factor;
    UINTN                           QrOffset;
    UINTN                           BltBufferSize;
    UINTN                           Offset;
    INTN                            y, x, fy, fx;
    INTN                            yy, xx;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   Color;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ColorRsvd  =  { 232, 162,   0, 255 };    // Visibly different error pixel
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ColorWhite =  { 255, 255, 255, 255 };
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ColorBlack =  {   0,   0,   0, 255 };
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ColorGray  =  { 135, 135, 135, 255 };    // Visibly different missing pixel
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ColorBad   =  {   0, 255,   0, 255 };    // Visibly different data corruption pixel
    BOOLEAN                         OutOfBounds = FALSE;

    Factor = RegionSize / (gQrSize + 2 * QR_QUIET_ZONE);    // There must be 4 modules of white around bitmap from spec.
    QrOffset = ((RegionSize - (Factor * gQrSize)) / Factor) / 2;
    DEBUG((DEBUG_INFO,"RegionSize data R=%d, Computed R%d\n",RegionSize, Factor * gQrSize));
    BltBufferSize = RegionSize * RegionSize;
    gBltBuffer = AllocatePool (BltBufferSize * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    if (NULL == gBltBuffer) {
        DEBUG((DEBUG_ERROR,"Error allocating gBltBuffer\n"));
        return EFI_OUT_OF_RESOURCES;
    }
    SetMem32(gBltBuffer, BltBufferSize * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL), (UINT32)0xffffffff);

    for (y = 0; y < gQrSize ; y++) {
        for (x = 0; x < gQrSize; x++) {
            switch (gQrBitmap[y * gQrSize + x]) {
            case QrRsvd:
            case QrRsvd_E:
                Color =  ColorRsvd;
                break;
            case QrWhite:
            case QrWhite_E:
                Color =  ColorWhite;
                break;
            case QrBlack:
            case QrBlack_E:
                Color =  ColorBlack;
                break;
            case QrGray:
            case QrGray_E:
                Color =  ColorGray;
                break;
            default:
                Color =  ColorBad;
                break;
            }

            //
            // The BltBuffer larger than the Bitmap buffer - Factor * each side.
            fy = (y + QrOffset) * Factor;
            fx = (x + QrOffset) * Factor;

            //
            //  copy Bitmap[x:y] to BltBuffer[fx:fy], but fill in BltBuffer with Factor * Factor pixels
            //  for each single pixel in Bitmap.
            //                                fxo = fx + QrOffset
            //                                fyo = fy + QrOffset
            // QrCode is centered within the Region provided, and entire region is
            // set to white before the QrCode is written
            for (yy = 0; (yy < Factor) && (!OutOfBounds); yy++) {
                for (xx = 0; (xx < Factor) && (!OutOfBounds); xx++) {
                    Offset = ((fy + yy) * RegionSize) + fx + xx;
                    if (Offset >= BltBufferSize) {
                        DEBUG((DEBUG_ERROR,"Out of bounds for PIXEL Array. y=%d, x=%d, fy=%d, fx=%d, yy=%d, xx=%d, Factor=%d, Of=%d, BltSize=%d,Reg=%d\n",y,x,fy,fx,yy,xx,Factor,Offset,BltBufferSize,RegionSize));
                        OutOfBounds = TRUE;
                    } else {
                        gBltBuffer[Offset] = Color;
                    }
                }
            }
            if (OutOfBounds) {
                return EFI_NO_MEDIA;
            }

#if 0
            //
            // The BltBuffer larger than the Bitmap buffer - Factor * each side.

            fy = (y * Factor) * (BitmapLen * Factor);
            fx = x * Factor;
            //
            //  copy Bitmap[x:y] to BltBuffer[fx:fy], but fill in BltBuffer with Factor * Factor pixels
            //
            for (yy = 0; yy < Factor; yy++) {
                for (xx = 0; xx < Factor; xx++) {
                    Offset = fy + (yy * BitmapLen * Factor) + fx + xx;
                    if (Offset > BltBufferElements) {
                        DEBUG((DEBUG_ERROR,"Out of bounds for PIXEL Array\n"));
                        ASSERT(FALSE );
                    }
                    BltBuffer[Offset] = Color;
                }
            }
#endif
        }
    }
    return EFI_SUCCESS;
}

//*----------------------------------------------------------------------------*
//*   QrInitialize                                                             *
//*                                                                            *
//*   Initializes the encoder for a fixed choice of QR code, QR                *
//*   Correction level, and character encoding mode.                           *
//*                                                                            *
//*   QR Version and Encoding mode can be set to Auto, and will be determined  *
//*   by the data.                                                             *
//*                                                                            *
//*   Input:                                                                   *
//*       Version              = Version Requested (1-40, QrAutoVersion=Auto)  *
//*       Level                = Error Correction Level                        *
//*       Mode                 = Character Encoding mode                       *
//*       Flags                = Debug flags.  Used only by QrTest. Should     *
//*                              be 0 for normal use.                          *
//*   Returns:                                                                 *
//*      EFI_SUCCESS           = Parameters accepted                           *
//*                                                                            *
//*      EFI_UNSUPPORTED       = QrCode requested does not support the data    *
//*                              supplied.                                     *
//*      EFI_INVALID_PARAMETER = Version, Level, or Mode out of range          *
//*                                                                            *
//*----------------------------------------------------------------------------*
static
EFI_STATUS
QrInitialize (IN UINT8      Version,
              IN QRLEVEL    Level,
              IN QRENCODING Mode,
              IN UINT32     Flags) {

    if (Version > QrMaxVersion)  // Version within spec?
    {
        DEBUG((DEBUG_ERROR, __FUNCTION__ " - Invalid Version %d proposed\n",Version));
        return EFI_INVALID_PARAMETER;
    }
    // ECLevel MUST be specified.
    if ((Level < QrECLevel_L) || (Level > QrECLevel_H))
    {
        DEBUG((DEBUG_ERROR, __FUNCTION__ " - Invalid ECLevel %d proposed\n",Level));
        return EFI_INVALID_PARAMETER;
    }
    if ((Mode < QrAutoMode) || (Mode > QrByteMode))  // Only support Num/Alpha/Byte for now
    {
        DEBUG((DEBUG_ERROR, __FUNCTION__ " - Invalid Mode %d proposed\n",Mode));
        return EFI_INVALID_PARAMETER;
    }

    InitializeLogTables ();

    gQrVersion = Version;
    gQrSize = 0;
    gQrLevel = Level;
    gQrMode = Mode;
    gQrMask = 0;
    gFlags = Flags;

    return EFI_SUCCESS;
}

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
//*      Version      Version Requested (1-40, QrAutoVersion=Auto)             *
//*      Level        Error Correction Level                                   *
//*      Mode         Character Encoding mode                                  *
//*      Flags        Debug flags - Used only by QrTest. Enables additional dbg*
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
              OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL  **Bitmap) {     // Place to store the created bitmap pointer
    EFI_STATUS Status;

    if ((Data == NULL) || DataLen == 0 || (Bitmap == NULL)) {
        DEBUG((DEBUG_ERROR, __FUNCTION__ " - Data == NULL, DataLen == 0, or Bitmap == NULL\n"));
        return EFI_INVALID_PARAMETER;
    }

    Status = QrInitialize (Version, Level, Mode, Flags);

    if (EFI_ERROR(Status)) {
        return Status;
    }

    /*--------------------------------------------------------------------------------*/
    /* Step 1. Data Analysis                                                          */
    /*         Analyze input data to identify the characteristics of the data         */
    /*--------------------------------------------------------------------------------*/
    Status = Step1_Process (Data, DataLen, RegionSize);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Step 1 Process error.  Code=%r\n",Status));
        goto error_exit;
    }
    DEBUG((DEBUG_INFO,"Step 1 Complete\n"));
    /*--------------------------------------------------------------------------------*/
    /* Step 2. Data Encoding                                                          */
    /*         Convert the characters to a bit stream in accordance with the QR Code, */
    /*         the encoding mode, and the EC lavel.                                   */
    /*--------------------------------------------------------------------------------*/
    Status = Step2_Process (Data, DataLen);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Step 2 Process error.  Code=%r\n",Status));
        goto error_exit;
    }
    DEBUG((DEBUG_INFO,"Step 2 Complete\n"));

    /*--------------------------------------------------------------------------------*/
    /* Step 3. Error Correction Encoding                                              */
    /*         Divide the codeword sequence into blocks as per table 9, and generate  */
    /*         the error correction codewords for each block.                         */
    /*         the encoding mode, and the EC lavel.                                   */
    /*--------------------------------------------------------------------------------*/
    Status = Step3_Process ();
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Step 3 Process error.  Code=%r\n",Status));
        goto error_exit;
    }
    DEBUG((DEBUG_INFO,"Step 3 Complete\n"));

    /*--------------------------------------------------------------------------------*/
    /* Step 4. Structure the final message                                            */
    /*         Interleave the data and codewords from each block and add remainder    */
    /*         bits if necessary                                                      */
    /*--------------------------------------------------------------------------------*/
    Status = Step4_Process ();
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Step 4 Process error.  Code=%r\n",Status));
        goto error_exit;
    }
    DEBUG((DEBUG_INFO,"Step 4 Complete\n"));

    /*--------------------------------------------------------------------------------*/
    /* Step 5. Module placement in matrix                                             */
    /*         Place the finder pattern                                               */
    /*         Place the separators                                                   */
    /*         Place the timing pattern                                               */
    /*         Place the timing pattern                                               */
    /*         Place the alignment patterns (if required)                             */
    /*         Place the codewords into the Matrix                                    */
    /*--------------------------------------------------------------------------------*/
    Status = Step5_Process ();
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Step 5 Process error.  Code=%r\n",Status));
        goto error_exit;
    }
    DEBUG((DEBUG_INFO,"Step 5 Complete\n"));

    /*--------------------------------------------------------------------------------*/
    /* Step 6. Data Masking                                                           */
    /*         Apply the 8 data masking patterns and evaluate each pattern for quality*/
    /*         Choose the pattern with the best quality                               */
    /*--------------------------------------------------------------------------------*/
    Status = Step6_Process ();
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Step 6 Process error.  Code=%r\n",Status));
        goto error_exit;
    }
    DEBUG((DEBUG_INFO,"Step 6 Complete\n"));

    /*--------------------------------------------------------------------------------*/
    /* Step 7. Format and version information                                         */
    /*         Generate the format information and version information and complete   */
    /*         the symbol.                                                            */
    /*--------------------------------------------------------------------------------*/
    Status = Step7_Process ();
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Step 7 Process error.  Code=%r\n",Status));
        goto error_exit;
    }
    DEBUG((DEBUG_INFO,"Step 7 Complete\n"));

    /*--------------------------------------------------------------------------------*/
    /* Step 8. Build the Gop->Blt ready bitmap                                        */
    /*--------------------------------------------------------------------------------*/
    Status = Step8_Process (RegionSize);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Step 8 Process error.  Code=%r\n",Status));
        goto error_exit;
    }
    DEBUG((DEBUG_INFO,"Step 8 Complete\n"));

error_exit:

    // Free all the memory allocated - EXCEPT the returned bitmap

    if (NULL != gCodeWords ) {
        FreePool (gCodeWords);
        gCodeWords = NULL;
    }
    if (NULL != gECWords ) {
        FreePool (gECWords);
        gECWords = NULL;
    }
    if (NULL != gBitStream ) {
        FreePool (gBitStream);
        gBitStream = NULL;
    }
    if (gQrBitmap != NULL) {
        FreePool (gQrBitmap);
        gQrBitmap = NULL;
    }
    if (EFI_ERROR(Status)) {
        *Bitmap = NULL;
        if (NULL != gBltBuffer) {
            FreePool(gBltBuffer);
            gBltBuffer = NULL;
        }
    } else {
        *Bitmap = gBltBuffer;
        gBltBuffer = NULL;
    }

    DEBUG((DEBUG_INFO,"QrEncode complete. Code = %r\n",Status));

    return Status;
}

