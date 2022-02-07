/** @file
HwhMenu.h

Header file for HwhMenu

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __HWH_MENU__
#define __HWH_MENU__

#define MAX_DISPLAY_STRING_LENGTH  100

extern UINT8  HwhMenuVfrBin[];

extern unsigned char  HwhMenuStrings[];

/**
 *  Writes the input string to the input vfr string
 *
 *  @param[in]  Str                  EFI_STRING being written to
 *  @param[in]  Format               Format string
 *  @param[in]  ...                  Variables placed into format string
 *
 *  @retval     VOID
**/
UINTN
UnicodeDataToVFR (
  IN CONST EFI_STRING_ID  Str,
  IN CONST CHAR16         *Format,
  ...
  );

#endif
