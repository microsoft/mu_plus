/** @file
DfciPkgTokenSpace.h

GUID for DfciPkg PCD Token Space

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
