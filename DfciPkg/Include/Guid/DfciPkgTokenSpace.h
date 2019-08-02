/** @file
DfciPkgTokenSpace.h

GUID for DfciPkg PCD Token Space

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DFCIPKG_TOKEN_SPACE_GUID_H_
#define _DFCIPKG_TOKEN_SPACE_GUID_H_

///
/// The Global ID for the DFCI Package PCD Token Space.
///
// {3E8B05F6-DD48-413C-87B9-057E6A0C0F93}
#define DFCIPKG_TOKEN_SPACE_GUID \
  { \
    0x3e8b05f6, 0xdd48, 0x413c, { 0x87, 0xb9, 0x5, 0x7e, 0x6a, 0xc, 0xf, 0x93 } \
  }

extern EFI_GUID gDfciPkgTokenSpaceGuid;

#endif // _DFCIPKG_TOKEN_SPACE_GUID_H_
