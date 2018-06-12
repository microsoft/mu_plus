/** @file
Provides the helper routines needed for working with test capsules

Copyright (c) 2016, Microsoft Corporation

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

#ifndef __TEST_CAPSULE_HELPER_LIB_H__
#define __TEST_CAPSULE_HELPER_LIB_H__

/**
Function to get a capsule from the system table.
Since there can be more than a single capsule with the same
guid.  Use the Index parameter to iterate thru the
capsules.

@param  Index - capsule index to return (0 based)
@param  Head (out) - Capsule header pointer returned on successful

@return EFI_SUCCESS if capsule found
EFI_NOT_FOUND - Index parameter out of bounds
EFI_VOLUME_CORRUPTED - Capsule doesn't have valid signature
EFI_INCOMPATIBLE_VERSION - Capsule version not expected
**/
EFI_STATUS
EFIAPI
GetTestCapsuleFromSystemTable(
	IN  UINTN Index,
	OUT EFI_CAPSULE_HEADER **Head
	);

UINTN
EFIAPI
GetTestCapsuleCountFromSystemTable();

EFI_STATUS
EFIAPI
BuildTestCapsule(
	IN      UINT32                         CapsuleFlags,
	IN OUT  EFI_CAPSULE_BLOCK_DESCRIPTOR** SgList,
	IN      INTN                          Count,
	IN      UINTN                          Sizes[]
);

VOID
EFIAPI
FreeSgList(
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR* List OPTIONAL
);

UINTN
EFIAPI
GetLayoutTotalSize(
  IN      INTN                          Count,
  IN      UINTN                          Sizes[]
);


#endif // 