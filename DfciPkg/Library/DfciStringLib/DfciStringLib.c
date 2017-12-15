/** @file
Ascii string manipulation
**/

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
Converts a Ascii encoded Hex array into a byte array

@param  Value      Pointer to a ascii char buffer that holds as hex value as a ascii char.
@param  ByteArray  Pointer to a UINT8 buffer to return the hex output.
@param  Size       Size of char array.
@retval Status     Returns Invalid parameter if the input char array has characters other than 0-9 or A-F

**/
RETURN_STATUS AsciitoHexByteArray(CONST CHAR8 *Value, UINT8* ByteArray, UINTN Size)
{
    UINT8 i;

    for (i = 0; i < Size; i++){
        if (!(IsHexaDecimal(*(Value + (2 * i))) && IsHexaDecimal(*(Value + (2 * i) + 1)))){
            return RETURN_INVALID_PARAMETER;
        }
        *(ByteArray + i) = (HexLookUp(*(Value + (2 * i))) << 4) | (HexLookUp(*(Value + (2 * i) + 1)));
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