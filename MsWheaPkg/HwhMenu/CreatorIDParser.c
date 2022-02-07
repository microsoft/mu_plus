/** @file
CreatorIDParser.c

A function for parsing the creator ID which for now just prints the guid.
Created to more easily implement friendly strings in the future

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Guid/Cper.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

#include "CreatorIDParser.h"
#include "HwhMenu.h"

/**
 *  Parses the Creator ID which, for now, just prints the GUID
 *
 *  @param[in] EFI_GUID*   Guid being parsed
 *
 *  @retval     VOID
**/
VOID
ParseCreatorID (
  IN CONST EFI_GUID  *CreatorID
  )
{
  // For now, just print the guid. Leaving this separate file
  // to make extending guid parsing in the future a bit simpler
  UnicodeDataToVFR (
    (EFI_STRING_ID)STR_HWH_LOG_CREATORID_VALUE,
    L"%g",
    CreatorID
    );
}
