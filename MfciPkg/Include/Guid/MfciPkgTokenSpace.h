/** @file
MfciPkgTokenSpace.h

GUID for MfciPkg PCD Token Space

Copyright (c) Microsoft Corporation
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MFCIPKG_TOKEN_SPACE_GUID_H_
#define _MFCIPKG_TOKEN_SPACE_GUID_H_

///
/// The Global ID for the MFCI Package PCD Token Space.
///
// {9dea6cff-1d8d-449f-b1f7-5a8b8f9ca268}
#define MFCIPKG_TOKEN_SPACE_GUID \
  { \
    0x9dea6cff, 0x1d8d, 0x449f, { 0xb1, 0xf7, 0x5a, 0x8b, 0x8f, 0x9c, 0xa2, 0x68 } \
  }

extern EFI_GUID gMfciPkgTokenSpaceGuid;

#endif // _MFCIPKG_TOKEN_SPACE_GUID_H_
