/** @file
CreatorIDParser.h

A function for parsing the creator ID which for now just prints the guid.
Created to more easily implement friendly strings in the future

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef CREATOR_ID_PARSER_H_
#define CREATOR_ID_PARSER_H_

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
  );

#endif
