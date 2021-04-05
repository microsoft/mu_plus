/** @file 
PlatformIDParser.c

A function for parsing the Platform ID which for now just prints the guid. 
Created to more easily implement friendly strings in the future

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/Cper.h> 
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

#include "PlatformIDParser.h"
#include "HwhMenu.h"

/**
 *  Parses the Platform/Source ID
 *
 * @param[in] EFI_GUID *SourceID   Guid being parsed
 * 
 *  @retval     VOID
**/
VOID
ParseSourceID(
  IN CONST EFI_GUID *SourceID
  )
{

    // For now, just print the guid. Leaving this separate file
    // to make extending guid parsing in the future a bit simpler
    UnicodeDataToVFR((EFI_STRING_ID)STR_HWH_LOG_SOURCEID_VALUE,
                     L"%g", 
                     *SourceID);
    
}