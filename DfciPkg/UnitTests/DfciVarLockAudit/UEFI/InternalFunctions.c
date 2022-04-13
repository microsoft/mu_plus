/** @file
Internal helper functions for the App

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DfciLockTestXml.h"

// ---------------------------------------------------------------
// THESE NEXT 2 FUNCTIONs WERE COPIED FROM
// UefiShellLevel1CommandsLib.  CAN NOT FIND THE FUNCTIONS
// IN A PUBLIC LIBRARY CLASS.
// ---------------------------------------------------------------

UINTN
EFIAPI
HexCharToUintn (
  IN      CHAR16  Char
  )
{
  if ((Char >= L'0') && (Char <= L'9')) {
    return Char - L'0';
  }

  return (UINTN)(10 + CharToUpper (Char) - L'A');
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
ConvertStringToGuid (
  IN CONST CHAR16  *StringGuid,
  IN OUT EFI_GUID  *Guid
  )
{
  CHAR16      *TempCopy;
  CHAR16      *TempSpot;
  CHAR16      *Walker;
  UINT64      TempVal;
  EFI_STATUS  Status;

  if (StringGuid == NULL) {
    return (EFI_INVALID_PARAMETER);
  } else if (StrLen (StringGuid) != 36) {
    return (EFI_INVALID_PARAMETER);
  }

  TempCopy = NULL;
  TempCopy = StrnCatGrow (&TempCopy, NULL, StringGuid, 0);
  if (TempCopy == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  Walker   = TempCopy;
  TempSpot = StrStr (Walker, L"-");
  if (TempSpot != NULL) {
    *TempSpot = CHAR_NULL;
  }

  Status = ShellConvertStringToUint64 (Walker, &TempVal, TRUE, FALSE);
  if (EFI_ERROR (Status)) {
    FreePool (TempCopy);
    return (Status);
  }

  Guid->Data1 = (UINT32)TempVal;
  Walker     += 9;
  TempSpot    = StrStr (Walker, L"-");
  if (TempSpot != NULL) {
    *TempSpot = CHAR_NULL;
  }

  Status = ShellConvertStringToUint64 (Walker, &TempVal, TRUE, FALSE);
  if (EFI_ERROR (Status)) {
    FreePool (TempCopy);
    return (Status);
  }

  Guid->Data2 = (UINT16)TempVal;
  Walker     += 5;
  TempSpot    = StrStr (Walker, L"-");
  if (TempSpot != NULL) {
    *TempSpot = CHAR_NULL;
  }

  Status = ShellConvertStringToUint64 (Walker, &TempVal, TRUE, FALSE);
  if (EFI_ERROR (Status)) {
    FreePool (TempCopy);
    return (Status);
  }

  Guid->Data3    = (UINT16)TempVal;
  Walker        += 5;
  Guid->Data4[0] = (UINT8)(HexCharToUintn (Walker[0]) * 16);
  Guid->Data4[0] = (UINT8)(Guid->Data4[0] + (UINT8)HexCharToUintn (Walker[1]));
  Walker        += 2;
  Guid->Data4[1] = (UINT8)(HexCharToUintn (Walker[0]) * 16);
  Guid->Data4[1] = (UINT8)(Guid->Data4[1] + (UINT8)HexCharToUintn (Walker[1]));
  Walker        += 3;
  Guid->Data4[2] = (UINT8)(HexCharToUintn (Walker[0]) * 16);
  Guid->Data4[2] = (UINT8)(Guid->Data4[2] + (UINT8)HexCharToUintn (Walker[1]));
  Walker        += 2;
  Guid->Data4[3] = (UINT8)(HexCharToUintn (Walker[0]) * 16);
  Guid->Data4[3] = (UINT8)(Guid->Data4[3] + (UINT8)HexCharToUintn (Walker[1]));
  Walker        += 2;
  Guid->Data4[4] = (UINT8)(HexCharToUintn (Walker[0]) * 16);
  Guid->Data4[4] = (UINT8)(Guid->Data4[4] + (UINT8)HexCharToUintn (Walker[1]));
  Walker        += 2;
  Guid->Data4[5] = (UINT8)(HexCharToUintn (Walker[0]) * 16);
  Guid->Data4[5] = (UINT8)(Guid->Data4[5] + (UINT8)HexCharToUintn (Walker[1]));
  Walker        += 2;
  Guid->Data4[6] = (UINT8)(HexCharToUintn (Walker[0]) * 16);
  Guid->Data4[6] = (UINT8)(Guid->Data4[6] + (UINT8)HexCharToUintn (Walker[1]));
  Walker        += 2;
  Guid->Data4[7] = (UINT8)(HexCharToUintn (Walker[0]) * 16);
  Guid->Data4[7] = (UINT8)(Guid->Data4[7] + (UINT8)HexCharToUintn (Walker[1]));
  FreePool (TempCopy);
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
ConvertAsciiStringToGuid (
  IN CONST CHAR8   *StringGuid,
  IN OUT EFI_GUID  *Guid
  )
{
  CHAR16  StringGuid16[37];
  UINTN   Len = 0;

  if (StringGuid == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  Len = AsciiStrnLenS (StringGuid, 40);
  if (Len != 36) {
    return (EFI_INVALID_PARAMETER);
  }

  StringGuid16[0] = '\0';
  AsciiStrToUnicodeStrS (StringGuid, StringGuid16, 37);

  if (StrnLenS (StringGuid16, 40) != 36) {
    return EFI_INVALID_PARAMETER;
  }

  return ConvertStringToGuid (StringGuid16, Guid);
}
