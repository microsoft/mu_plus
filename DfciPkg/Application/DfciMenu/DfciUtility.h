/** @file
DfciUtility.h

DfciUtility contains useful functions

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

#ifndef __DFCI_UTILITY_H__
#define __DFCI_UTILITY_H__

#include <Uefi.h>

#include <DfciSystemSettingTypes.h>

#include <Library/HiiLib.h>

#define DFCI_MAX_STRING_LEN (1024)

typedef struct {
    CHAR8      *SerialNumber;
    UINTN       SerialNumberSize;
    CHAR8      *Manufacturer;
    UINTN       ManufacturerSize;
    CHAR8      *ProductName;
    UINTN       ProductNameSize;
} DFCI_SYSTEM_INFORMATION;

/**
 * ConvertToCHAR16 - Converts a CHAR8 string to a CHAR16 string.
 *
 * @param[in]   Text           Input CHAR8 String
 * @param[out]  Text16         Where to store pointer to new CHAR8 string
 * @param[out]  Text16Size     CHAR16 string size (includes NULL)
 *
 * @return     String as CHAR16.  Caller is responsible for freeing returned
 *             string
 */
EFI_STATUS
DfciConvertToCHAR16 (
    IN CHAR8    *Text8,
    IN UINTN     Text8Len,
    OUT CHAR16 **Text16,
    OUT UINTN   *Text16Size  OPTIONAL
  );

/**
 * ConvertToCHAR8 - Converts a CHAR16 string to a CHAR8 string.
 *
 * @param[in]   Text16           Input CHAR16 string
 * @param[in]   Text16Len        Input CHAR16 number of characters
 * @param[out]  Text8            Where to store pointer to new CHAR8 string
 * @param[out]  Text8Size        CHAR8 string size (includes NULL)
 *
 * @return     String as CHAR8.  Caller is responsible for freeing returned
 *             string
 */
EFI_STATUS
DfciConvertToCHAR8 (
    IN CHAR16    *Text16,
    IN UINTN      Test16Len,
    OUT CHAR8   **Text8,
    OUT UINTN    *Text8Len  OPTIONAL
  );

/**
 * SetStringEntry16 - sets the HiiString, and verify that it was accepted.
 *
 * @param[in]  IdName
 * @param[in]  StringValue
 *
 * @return EFI_STATUS
 */
EFI_STATUS
DfciSetString16Entry (
    IN  EFI_HII_HANDLE HiiHandle,
    IN  EFI_STRING_ID  IdName,
    IN  CHAR16        *StringValue
  );

/**
 * SetStringEntry - Converts the string to CHAR16, and calls SetString16Entry
 *
 * @param[in]  IdName
 * @param[in]  StringValue
 *
 * @return EFI_STATUS
 */
EFI_STATUS
DfciSetStringEntry (
    IN  EFI_HII_HANDLE HiiHandle,
    IN  EFI_STRING_ID  IdName,
    IN  CHAR8         *StringValue
  );

/**
 * Get A Setting
 *
 * @param[in]   IdName
 * @param[in]   ValuePtr
 * @param[out]  ValueSize
 *
 * @return EFI_STATUS
 */
EFI_STATUS
DfciGetASetting (
    IN  DFCI_SETTING_ID_STRING  IdName,
    IN  DFCI_SETTING_TYPE       Type,
    IN  VOID                  **ValuePtr,
    OUT UINTN                  *ValueSize
  );

/**
 * DfciGetSystemInfo
 *
 * Get the system identifier elements
 *
 * @param[in] DfciInfo
 *
 **/
EFI_STATUS
DfciGetSystemInfo (
    IN DFCI_SYSTEM_INFORMATION *DfciInfo
  );

/**
 * DfciFreeInfo
 *
 * Free the system identifier elements
 **/
VOID
DfciFreeSystemInfo (
    IN DFCI_SYSTEM_INFORMATION *DfciInfo
  );

#endif