/** @file
ZeroTouchPkgTokenSpace.h

GUID for ZeroTouchPkg PCD Token Space

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ZEROTOUCHPKG_TOKEN_SPACE_GUID_H_
#define _ZEROTOUCHPKG_TOKEN_SPACE_GUID_H_

///
/// The Global ID for the ZeroTouch Package PCD Token Space.
///
// { 353455c8-b2ec-44f3-91cf-0f7633c2de6b }
#define ZEROTOUCHPKG_TOKEN_SPACE_GUID \
  { \
    0x353455c8, 0xb2ec,0x44f3, { 0x91, 0xcf, 0x0f, 0x76, 0x33, 0xc2, 0xde, 0x6b } \
  }

extern EFI_GUID  gZeroTouchPkgTokenSpaceGuid;

#endif // _ZEROTOUCHPKG_TOKEN_SPACE_GUID_H_
