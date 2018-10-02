/** @file
ZeroTouchPkgTokenSpace.h

GUID for ZeroTouchPkg PCD Token Space

Copyright (c) 2018, Microsoft Corporation

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

extern EFI_GUID gZeroTouchPkgTokenSpaceGuid;

#endif // _ZEROTOUCHPKG_TOKEN_SPACE_GUID_H_
