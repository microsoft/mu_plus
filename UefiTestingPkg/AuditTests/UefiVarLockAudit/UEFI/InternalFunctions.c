/** @file
Internal helper functions for the App

Copyright (c) 2016, Microsoft Corporation

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


#include "LockTestXml.h"

//---------------------------------------------------------------
// THESE NEXT 3 FUNCTIONs WERE COPIED FROM 
// UEFISHELLDEBUG1COMMANDSLIB.  CAN NOT FIND THE FUNCTIONS
// IN A PUBLIC LIBRARY CLASS. 
//---------------------------------------------------------------

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



UINTN
EFIAPI
HexCharToUintn(
IN      CHAR16                    Char
)
{
	if (Char >= L'0' && Char <= L'9') {
		return Char - L'0';
	}

	return (UINTN)(10 + CharToUpper(Char) - L'A');
}


/**
Convert a string representation of a guid to a Guid value.

@param[in] StringGuid    The pointer to the string of a guid.
@param[in, out] Guid     The pointer to the GUID structure to populate.

@retval EFI_INVALID_PARAMETER   A parameter was invalid.
@retval EFI_SUCCESS             The conversion was successful.
**/
EFI_STATUS
EFIAPI
ConvertStringToGuid(
IN CONST CHAR16 *StringGuid,
IN OUT EFI_GUID *Guid
)
{
	CHAR16  *TempCopy;
	CHAR16  *TempSpot;
	CHAR16  *Walker;
	UINT64  TempVal;
	EFI_STATUS Status;

	if (StringGuid == NULL) {
		return (EFI_INVALID_PARAMETER);
	}
	else if (StrLen(StringGuid) != 36) {
		return (EFI_INVALID_PARAMETER);
	}
	TempCopy = NULL;
	TempCopy = StrnCatGrow(&TempCopy, NULL, StringGuid, 0);
	if (TempCopy == NULL) {
		return (EFI_OUT_OF_RESOURCES);
	}
	Walker = TempCopy;
	TempSpot = StrStr(Walker, L"-");
	if (TempSpot != NULL) {
		*TempSpot = CHAR_NULL;
	}
	Status = ShellConvertStringToUint64(Walker, &TempVal, TRUE, FALSE);
	if (EFI_ERROR(Status)) {
		FreePool(TempCopy);
		return (Status);
	}
	Guid->Data1 = (UINT32)TempVal;
	Walker += 9;
	TempSpot = StrStr(Walker, L"-");
	if (TempSpot != NULL) {
		*TempSpot = CHAR_NULL;
	}
	Status = ShellConvertStringToUint64(Walker, &TempVal, TRUE, FALSE);
	if (EFI_ERROR(Status)) {
		FreePool(TempCopy);
		return (Status);
	}
	Guid->Data2 = (UINT16)TempVal;
	Walker += 5;
	TempSpot = StrStr(Walker, L"-");
	if (TempSpot != NULL) {
		*TempSpot = CHAR_NULL;
	}
	Status = ShellConvertStringToUint64(Walker, &TempVal, TRUE, FALSE);
	if (EFI_ERROR(Status)) {
		FreePool(TempCopy);
		return (Status);
	}
	Guid->Data3 = (UINT16)TempVal;
	Walker += 5;
	Guid->Data4[0] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
	Guid->Data4[0] = (UINT8)(Guid->Data4[0] + (UINT8)HexCharToUintn(Walker[1]));
	Walker += 2;
	Guid->Data4[1] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
	Guid->Data4[1] = (UINT8)(Guid->Data4[1] + (UINT8)HexCharToUintn(Walker[1]));
	Walker += 3;
	Guid->Data4[2] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
	Guid->Data4[2] = (UINT8)(Guid->Data4[2] + (UINT8)HexCharToUintn(Walker[1]));
	Walker += 2;
	Guid->Data4[3] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
	Guid->Data4[3] = (UINT8)(Guid->Data4[3] + (UINT8)HexCharToUintn(Walker[1]));
	Walker += 2;
	Guid->Data4[4] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
	Guid->Data4[4] = (UINT8)(Guid->Data4[4] + (UINT8)HexCharToUintn(Walker[1]));
	Walker += 2;
	Guid->Data4[5] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
	Guid->Data4[5] = (UINT8)(Guid->Data4[5] + (UINT8)HexCharToUintn(Walker[1]));
	Walker += 2;
	Guid->Data4[6] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
	Guid->Data4[6] = (UINT8)(Guid->Data4[6] + (UINT8)HexCharToUintn(Walker[1]));
	Walker += 2;
	Guid->Data4[7] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
	Guid->Data4[7] = (UINT8)(Guid->Data4[7] + (UINT8)HexCharToUintn(Walker[1]));
	FreePool(TempCopy);
	return (EFI_SUCCESS);
}

/**
Convert an ascii string representation of a guid to a Guid value.

@param[in] StringGuid    The pointer to the string of a guid.
@param[in, out] Guid     The pointer to the GUID structure to populate.

@retval EFI_INVALID_PARAMETER   A parameter was invalid.
@retval EFI_SUCCESS             The conversion was successful.
**/
EFI_STATUS
EFIAPI
ConvertAsciiStringToGuid(
  IN CONST CHAR8 *StringGuid,
  IN OUT EFI_GUID *Guid
)
{ 
  CHAR16      StringGuid16[37];
  UINTN       Len = 0;

  if (StringGuid == NULL) 
  {
    return (EFI_INVALID_PARAMETER);
  }
  
  Len = AsciiStrnLenS(StringGuid, 40);
  if (Len != 36) 
  {
    return (EFI_INVALID_PARAMETER);
  }
  StringGuid16[0] = '\0';
  AsciiStrToUnicodeStrS(StringGuid, StringGuid16, 37);
  
  if (StrnLenS(StringGuid16, 40) != 36) 
  {
    return EFI_INVALID_PARAMETER;
  }

  return ConvertStringToGuid(StringGuid16, Guid);
}

