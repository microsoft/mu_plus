/** @file 
GenericSectionParser.c

This library implements a parser for the MS Generic WHEA section type.
It must be linked against the ParserRegistryLib.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Pi/PiStatusCode.h>

#include <Guid/Cper.h> 
#include <Guid/MuTelemetryCperSection.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ParserRegistryLib.h>

#define MAX_STRING 100

/**
 *  Parses the data located at Err + SectionHead->SectionOffset
 *
 *  @param[in]     Strings          Pointer to unallocated array of strings which will be populated by the parser
 *  @param[in]     Err              Pointer to the HwErrRec bing parsed
 *  @param[in]     SectionHeader    Pointer to Section Header of data being parsed
 * 
 *  @retval        UINTN            Number of CHAR16* allocated in Strings
**/
UINTN 
ParseGenericSection(
  IN OUT        CHAR16***                         Strings, 
  IN     CONST  EFI_COMMON_ERROR_RECORD_HEADER*   Err, 
  IN     CONST  EFI_ERROR_SECTION_DESCRIPTOR*     SectionHead
  )
{ 
  // Pointer to section data of section we're looking at
  CONST MU_TELEMETRY_CPER_SECTION_DATA  *SectionData = (MU_TELEMETRY_CPER_SECTION_DATA*) ((UINT8*)Err + SectionHead->SectionOffset);
  CONST UINT8                           NumStrings = 4; // Number of CHAR16* which will be allocated within Strings
        UINT8                           *p;              // For capturing each byte of AdditionalInfo Fields
        CHAR8                           str[9];         // For converting AdditionalInfo1 & 2 into ascii

  str[8] = '\0';

  // Allocate pointers to strings  
  *Strings = AllocatePool(NumStrings * sizeof(CHAR16*));

  // Allocate actual strings
  for(UINT8 i = 0; i < NumStrings; i++){
    (*Strings)[i] = AllocatePool((MAX_STRING + 1) * sizeof(CHAR16));
  }

  //Output ComponentID and SubcomponentID
  UnicodeSPrint((*Strings)[0], 
                (MAX_STRING + 1) * sizeof(CHAR16), 
                L"Component ID:\n%g", 
                SectionData->ComponentID);

  UnicodeSPrint((*Strings)[1], 
                (MAX_STRING + 1) * sizeof(CHAR16), 
                L"SubComponent ID:\n%g", 
                SectionData->SubComponentID);
  
  // Set p to the start of AdditionalInfo1 to print bytes and ascii
  p = (UINT8*)&(SectionData->AdditionalInfo1);

  // Capture ascii version of AdditionalInfo1
  for(UINT8 i = 0; i < 8; i++)
  {
    str[i] = ((*(p + i) >= 0x20) && (*(p + i) <= 0x7e)) ? (*(p + i)) : '.'; 
  }

  UnicodeSPrint((*Strings)[2], 
                (MAX_STRING + 1) * sizeof(CHAR16), 
                L"AdditionalInfo1:\n%02X %02X %02X %02X %02X %02X %02X %02X\n%a",
                *(p), *(p + 1), *(p + 2), *(p + 3), *(p + 4), *(p + 5), *(p + 6), *(p + 7), 
                str);

  // Set p to the start of AdditionalInfo2 to print bytes and ascii
  p = (UINT8*)&(SectionData->AdditionalInfo2);

  // Capture ascii version of AdditionalInfo2
  for(UINT8 i = 0; i < 8; i++)
  {
    str[i] = ((*(p + i) >= 0x20) && (*(p + i) <= 0x7e)) ? (CHAR8)(*(p + i)) : '.';  
  }

  UnicodeSPrint((*Strings)[3], 
                (MAX_STRING + 1) * sizeof(CHAR16), 
                L"AdditionalInfo2:\n%02X %02X %02X %02X %02X %02X %02X %02X\n%a",
                *(p), *(p + 1), *(p + 2), *(p + 3), *(p + 4), *(p + 5), *(p + 6), *(p + 7), 
                str);

  return NumStrings;
} 

/**
  The driver's Constructor

  Simply registers the generic section parser
**/
EFI_STATUS
EFIAPI
GenericSectionParserLibConstructor(
    IN      EFI_HANDLE                ImageHandle,
    IN      EFI_SYSTEM_TABLE          *SystemTable
)
{

  // Register the generic section parser
  DEBUG((DEBUG_ERROR,"%a Adding to section parser to registry: %r\n", __FUNCTION__,
    ParserLibRegisterSectionParser((SECTIONFUNCTIONPTR) ParseGenericSection, &gMuTelemetrySectionTypeGuid)));

  return RETURN_SUCCESS;

}
