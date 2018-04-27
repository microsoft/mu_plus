/** @file
Ascii string manipulation
**/

#include <Uefi.h>
#include <Library/DebugLib.h>

#define BAD_V  99

static CHAR8 encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz"
                                "0123456789+/";

static UINT8 decoding_table[] = {
       /* Valid characters ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/                                                  */
       /*                  Also, set '=' as a zero                                                                                           */
       /*        0    ,  1,      2,      3,      4,      5,      6,      7,      8,      9,      a,      b,      c,      d,      e,      f   */
             BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //   0
             BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  10
             BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,     62,  BAD_V,  BAD_V,  BAD_V,     63,   //  20
                52,     53,     54,     55,     56,     57,     58,     59,     60,     61,  BAD_V,  BAD_V,  BAD_V,      0,  BAD_V,  BAD_V,   //  30
             BAD_V,      0,      1,      2,      3,      4,      5,      6,      7,      8,      9,     10,     11,     12,     13,     14,   //  40
                15,     16,     17,     18,     19,     20,     21,     22,     23,     24,     25,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  50
             BAD_V,     26,     27,     28,     29,     30,     31,     32,     33,     34,     35,     36,     37,     38,     39,     40,   //  60
                41,     42,     43,     44,     45,     46,     47,     48,     49,     50,     51,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  70
             BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  80
             BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  90
             BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  a0
             BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  b0
             BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  c0
             BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  d0
             BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  d0
             BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V    //  f0
};

/**
 * Get the next character that represents 6 bits of data
 *
 * @param ptr
 * @param value
 */
static
VOID
GetNext6bits (IN     CONST CHAR8  **ptr,
              IN OUT       UINT32  *Value,
                 OUT       BOOLEAN *Error) {

    UINT8   chr;
    CONST CHAR8 *x;

    x = *ptr;
    *ptr = *ptr +1;
    chr = decoding_table[*x];
    if (chr == BAD_V) {
        DEBUG((DEBUG_ERROR,"Invalid character %c found\n",chr));
        chr = 0;  // treat as zero and continue, but tell caller
        *Error = TRUE;
    }
    *Value <<= 6;
    *Value |= chr;
    return;
}

/**
 * Convert binary blob to a b64 encoded ascii string
 *
 *
 * @param dataPtr     Input UINT8 data
 * @param dataLen     Number of UINT8 bytes of data
 * @param asciiPtr    Pointer to output string buffer.
 *                    Caller is responsible for passing in buffer of asciiSize
 * @param asciiSize   Size of ascii buffer.  Set to 0 to get the size needed
 *
 * @return EFI_STATUS EFI_SUCCESS             ascii buffer filled in.
 *                    EFI_INVALID_PARAMETER   dataPtr NULL, asciiSize NULL, dataLen 0.
 *                    EFI_INVALID_PARAMETER   asciiPtr NULL and asciiSize != 0
 *                    EFI_BUFFER_TOO_SMALL    asciiSize too small - asciiize set to required size
 */
EFI_STATUS
Base64_Encode(IN  CONST UINT8  *dataPtr,
              IN        UINTN   dataLen,
     OPTIONAL OUT       CHAR8  *asciiPtr,
              IN        UINTN  *asciiSize) {

    UINTN          requiredSize;
    UINTN          left;
    CONST UINT8   *inptr;
    CHAR8         *outptr;

    // Check pointers, and datalen is valid
    if ((dataPtr == NULL) || (asciiSize == NULL) || (dataLen == 0)) {
        DEBUG((DEBUG_ERROR,"dataPtr=%p, binSize=%p, dateLem=%d\n", dataPtr, asciiSize, dataLen));
        return EFI_INVALID_PARAMETER;
    }

    requiredSize = ((dataLen + 2) / 3) * 4 + 1; // 4 ascii per 3 bytes + NULL
    if ((asciiPtr == NULL) || *asciiSize < requiredSize) {
        *asciiSize = requiredSize;
        return EFI_BUFFER_TOO_SMALL;
    }

    left = dataLen;
    outptr = asciiPtr;
    inptr  = dataPtr;

    // Encoding 24 bits (three bytes) into 4 ascii characters
    while (left >= 3) {
       *outptr++ = encoding_table[( inptr[0] & 0xfc) >> 2 ];  // Get first 6 bits
       *outptr++ = encoding_table[((inptr[0] & 0x03) << 4) +
                                  ((inptr[1] & 0xf0) >> 4)];  // Get second 6 bits
       *outptr++ = encoding_table[((inptr[1] & 0x0f) << 2) +
                                  ((inptr[2] & 0xc0) >> 6)];  // Get third 6 bits
       *outptr++ = encoding_table[( inptr[2] & 0x3f)];       // Get fourth 6 bits
       left -= 3;
       inptr += 3;
    }

    // Handle the remainder.
    switch (left) {
    case 0:
        // Done
        break;
    case 1:
        *outptr++ = encoding_table[( inptr[0] & 0xfc) >> 2];  // Get first 6 bits
        *outptr++ = encoding_table[((inptr[0] & 0x03) << 4)]; // Get last 2 bits
        *outptr++ = '=';                                      // 2 pad characters
        *outptr++ = '=';
        break;
    case 2:
        *outptr++ = encoding_table[( inptr[0] & 0xfc) >> 2];  // Get first 6 bits
        *outptr++ = encoding_table[((inptr[0] & 0x03) << 4) +
                                   ((inptr[1] & 0xf0) >> 4)];  // Get second 6 bits
        *outptr++ = encoding_table[((inptr[1] & 0x0f) << 2)]; // get last 4 bits
        *outptr++ = '=';                                      // 1 pad character
        break;
    }
    *outptr++;
    return EFI_SUCCESS;
}

/**
 * Convert b64 ascii string to binary blob
 *
 *
 * @param dataPtr     Input ASCII characters
 * @param dataLen     Number of ASCII characters
 * @param binPtr      Pointer to output buffer.
 *                    Caller is responsible for passing in buffer of binSize
 * @param binSize     0 to get the size needed
 *
 * @return EFI_STATUS EFI_SUCCESS             binary buffer filled in.
 *                    EFI_INVALID_PARAMETER   dataPtr NULL, binSize NULL, dataLen 0.
 *                    EFI_INVALID_PARAMETER   binPtr NULL and binSize != 0
 *                    EFI_INVALID_PARAMETER   Invalid character in input stream
 *                    EFI_BUFFER_TOO_SMALL    Buffer length too small - binSize set to required size;
 */
EFI_STATUS
Base64_Decode(IN  CONST CHAR8  *dataPtr,
              IN        UINTN   dataLen,
     OPTIONAL OUT       UINT8  *binPtr,
              IN        UINTN  *binSize) {

    UINT8   *binData;
    UINT32   Value;
    UINTN    indx;
    UINTN    ondx;
    UINTN    bufferSize;
    BOOLEAN  Error = FALSE;


    // Check pointers, and datalen is valid
    if ((dataPtr == NULL) || (binSize == NULL) || (dataLen == 0) || (dataLen % 4 != 0)) {
        DEBUG((DEBUG_ERROR,"dataPtr=%p, binSize=%p, dateLem=%d\n",dataPtr,binSize,dataLen));
        return EFI_INVALID_PARAMETER;
    }

    bufferSize = dataLen / 4 * 3;

    // Adjust for not multiple of 3 lengths
    if (dataPtr[dataLen - 1] == '=') {
        (bufferSize)--;
    }
    if (dataPtr[dataLen - 2] == '=') {
        (bufferSize)--;
    }

    if ((binPtr == NULL) || (*binSize < bufferSize)) {
        *binSize = bufferSize;
         return EFI_BUFFER_TOO_SMALL;
    }

    binData = binPtr;

    // Input data is verified to be a multiple of 4.  Process 4
    // characters at a time
    for (indx = 0, ondx = 0; indx < dataLen; indx += 4) {
        Value = 0;
        // Get 24 bits of data, each character representing 6 bits
        GetNext6bits(&dataPtr, &Value, &Error);
        GetNext6bits(&dataPtr, &Value, &Error);
        GetNext6bits(&dataPtr, &Value, &Error);
        GetNext6bits(&dataPtr, &Value, &Error);

        // Store 3 bytes of binary data (24 bits)
        *binData++ = (UINT8) (Value >> 16);

        // Due to the '=' special cases for two bytes at the end,
        // we have to check the length and not store the padding data
        if (ondx++ < *binSize) {
            *binData++ = (UINT8) (Value >>  8);
        }
        if (ondx++ < *binSize) {
            *binData++ = (UINT8) Value;
        }
    }

    if (Error) {
        return EFI_NO_MAPPING;
    }
    return EFI_SUCCESS;
}

/**
Convert Ascii encoded Hex to Binary value

@param  Char      Char ascii input to be converted to 4 bit Hex. (0-F)
@retval HexValue  Hex value of the ascii char

**/
UINT8 HexLookUp(CHAR8 Char){

    UINT8 HexValue;

    if (Char >= 'a' && Char <= 'f')
    {
        HexValue = (Char - 'a') + 10;
    }
    else if (Char >= 'A' && Char <= 'F')
    {
        HexValue = (Char - 'A') + 10;
    }
    else {
        HexValue = (Char - '0');
    }
    return (HexValue & 0x0f);
}

/**
Checks if the ASCII Char is a hexadecimal string 0-F

@param  Char      Char ascii input. (0-F)
@retval BOOLEAN   Returns true if the char is 0-9 or A-F

**/
BOOLEAN IsHexaDecimal(CHAR8 Char){

    if ((Char >= 'a' && Char <= 'f') || (Char >= 'A' && Char <= 'F') || (Char >= '0' && Char <= '9')){
        return TRUE;
    }
    else{
        return FALSE;
    }
}

/**
Converts an Ascii encoded Hex array into a byte array

@param  Value      Pointer to an ascii char buffer that holds a hex value as a ascii char.
@param  ByteArray  Pointer to a UINT8 buffer to return the hex output.
@param  Size       Size of char array.
                   ByteArray size must be at least 1/2 the StrLen(AsciiString)
@retval Status     Returns Invalid parameter if the input char array has characters other than 0-9 or A-F

**/
RETURN_STATUS AsciitoHexByteArray(CONST CHAR8 *Value, UINT8* ByteArray, UINTN Size)
{
    UINTN i;

    for (i = 0; i < Size; i++){
        if (!(IsHexaDecimal(*(Value + (2 * i))) && IsHexaDecimal(*(Value + (2 * i) + 1)))){
            return RETURN_INVALID_PARAMETER;
        }
        *(ByteArray + i) = (HexLookUp(*(Value + (2 * i))) << 4) | (HexLookUp(*(Value + (2 * i) + 1)));
    }

    return RETURN_SUCCESS;
}

/**
Converts a byte array into an Ascii encoded Hex array

@param  ByteArray  Pointer to a UINT8 buffer to return the hex output.
@param  Size       Size of ByteArray.
@param  Value      Pointer to an ascii char buffer to hold the Ascii string
                   -- must be ((2 * size of ByteArray) + 1) for a terminating NULL.
@retval Status     Returns Invalid Parameter if the size is odd or zero

**/
RETURN_STATUS HexByteArraytoAscii(IN  CONST UINT8 *ByteArray,
                                  IN  UINTN        Size,
                                  OUT CHAR8       *Value)
{
        UINTN  i;
static  CHAR8  HexChar[] = "0123456789abcdef";

    if ((Size < 2) || (Size & 0x01)) {
        return RETURN_INVALID_PARAMETER;
    }
    for (i = 0; i < Size; i++){
        *(Value + (2 * i    )) = HexChar[(*(ByteArray + i) & 0xf0) >> 4];
        *(Value + (2 * i + 1)) = HexChar[(*(ByteArray + i) & 0x0f)];
    }

    return RETURN_SUCCESS;
}

/**
Convert a Unicode character to upper case only if
it maps to a valid small-case ASCII character.

This function only deal with Unicode character
which maps to a valid small-case ASCII character, i.e.
L'a' to L'z'. For other Unicode character, the input character
is returned directly.

@param  Char  The character to convert.

@retval LowerCharacter   If the Char is with range L'a' to L'z'.
@retval Unchanged        Otherwise.

**/
CHAR16
EFIAPI
CharToUpper(
IN      CHAR16                    Char
)
{
    if (Char >= L'a' && Char <= L'z') {
        return (CHAR16)(Char - (L'a' - L'A'));
    }

    return Char;
}

