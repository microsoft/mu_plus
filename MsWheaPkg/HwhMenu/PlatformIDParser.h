/** @file
PlatformIDParser.h

A function for parsing the Platform ID which for now just prints the guid.
Created to more easily implement friendly strings in the future

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_ID_PARSER_H_
#define PLATFORM_ID_PARSER_H_

/**
 *  Parses the Platform/Source ID
 *
 * @param[in] EFI_GUID *SourceID   Guid being parsed
 *
 *  @retval     VOID
**/
VOID
ParseSourceID (
  IN CONST EFI_GUID  *SourceID
  );

#endif
