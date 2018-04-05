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



