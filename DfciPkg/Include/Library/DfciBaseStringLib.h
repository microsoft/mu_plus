/** @file
Ascii String manipulation
**/

/**
Convert Ascii encoded Hex to Binary value

@param  Char         Char ascii input to be converted to 4 bit Hex. (0-F)
@retval HexValue     Hex value of the ascii char

**/
UINT8 HexLookUp(CHAR8 Char);

/**
Checks if the ASCII Char is a hexadecimal number 0-F

@param  Char      Char ascii input. (0-F)
@retval BOOLEAN   Returns true if the char is 0-9 or A-F

**/
BOOLEAN IsHexaDecimal (CHAR8 Char);

/**
Converts a Ascii encoded Hex array into a byte array

@param  Value      Pointer to a ascii char buffer that holds as hex value as a ascii char.
@param  ByteArray  Pointer to a UINT8 buffer to return the hex output.
@param  Size       Size of Byte array -which is half the size of the ascii char array. every two characters will be stored as one byte.'F', 'F', -->0xFF
@retval Status     Returns Invalid parameter if the input char array has characters other than 0-9 or A-F

**/
RETURN_STATUS 
AsciitoHexByteArray (CONST CHAR8 *Value,
                     UINT8* ByteArray,
                     UINTN Size);

/**
Converts a byte array into an Ascii encoded Hex array

@param  ByteArray  Pointer to a UINT8 buffer to return the hex output.
@param  Size       Size of ByteArray.
@param  Value      Pointer to an ascii char buffer to hold the Ascii string
                   -- must be ((2 * size of ByteArray) + 1) for a terminating NULL.
@retval Status     Returns Invalid Parameter if the size is odd or zero

**/
RETURN_STATUS
HexByteArraytoAscii(IN  CONST UINT8 *ByteArray,
                    IN  UINTN        Size,
                    OUT CHAR8       *Value);

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
);

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
              IN        UINTN  *asciiSize);

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
Base64_Decode(IN  CONST UINT8  *dataPtr,
              IN        UINTN   dataLen,
     OPTIONAL OUT       UINT8  *binPtr,
              IN        UINTN  *binSize);

