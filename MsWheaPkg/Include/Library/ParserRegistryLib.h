/** @file
ParserRegistryLib.h

Header file for parser registry

ParserRegistryLib.c holds a table which associates a guid with a function. When
ParserLibRegisterSectionParser is called, a guid and function are put into the table.
And when ParserLibFindSectionParser is called, the input guid is used to return an
associated function (if one exists)

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PARSER_REG_LIB_
#define _PARSER_REG_LIB_

typedef UINTN (*SECTIONFUNCTIONPTR)(
  IN OUT CHAR16 ***,
  IN CONST EFI_COMMON_ERROR_RECORD_HEADER *,
  IN CONST EFI_ERROR_SECTION_DESCRIPTOR *
  );

#pragma pack(1)

// Struct contained within the table in parserregistrylib.c
typedef struct SectionMapType {
  EFI_GUID              Guid;                  // Section type
  SECTIONFUNCTIONPTR    Parser;                // Function which parses the section data
} SectionMapType;

#pragma pack()

/**
 *  Inserts a guid and function pointer into the internal table. The function pointer can later be retrieved
 *  by calling ParserLibFindSectionParser with the guid used to register the function. Note that we do not allow one
 *  guid to register more than one function.
 *
 *  @param[in]     Ptr                        Function pointer being registered
 *  @param[in]     Guid                       Guid being registered
 *
 *  @retval        EFI_SUCCESS                The guid and pointer were successfully registered
 *                 EFI_ABORTED                The guid has already been registered
 *                 EFI_OUT_OF_RESOURCES       Couldn't allocate the space required to store the guid and pointer
**/
EFI_STATUS
EFIAPI
ParserLibRegisterSectionParser (
  IN CONST SECTIONFUNCTIONPTR  Ptr,
  IN CONST GUID                *Guid
  );

/**
 *  Retrieves a function pointer from internal table using the input guid (if it exists)
 *
 *  @param[in]     Guid                       Guid used to find associated function pointer
 *
 *  @retval        NULL                       The guid was not registered with an associated function
 *                 Anything else              Function pointer returned
**/
SECTIONFUNCTIONPTR
EFIAPI
ParserLibFindSectionParser (
  IN CONST GUID  *Guid
  );

#endif
