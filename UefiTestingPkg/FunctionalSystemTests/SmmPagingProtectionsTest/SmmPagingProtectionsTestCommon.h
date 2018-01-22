/** @file -- SmmPagingProtectionsTestCommon.h
Shared definitions between the DXE and SMM drivers.
Mostly used for SMM communication.

Copyright (c) 2017, Microsoft Corporation

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

// MS_CHANGE - Entire file created.

#ifndef _SMM_PAGING_PROTECTIONS_TEST_COMMON_H_
#define _SMM_PAGING_PROTECTIONS_TEST_COMMON_H_

#define SMM_PAGING_PROTECTIONS_SELF_TEST_CODE            1
#define SMM_PAGING_PROTECTIONS_SELF_TEST_DATA            2
#define SMM_PAGING_PROTECTIONS_TEST_INVALID_RANGE        3

#pragma pack(1)

typedef struct _SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER {
  UINT16                Function;
  EFI_STATUS            ReturnStatus;
  EFI_PHYSICAL_ADDRESS  TargetAddress;
  UINT64                TargetValue;
} SMM_PAGING_PROTECTIONS_TEST_COMM_BUFFER;

#pragma pack()

// {E78B35F9-2A7C-46D0-9EAC-94BF4E31C089}
#define SMM_PAGING_PROTECTIONS_TEST_SMI_HANDLER_GUID \
  { 0xe78b35f9, 0x2a7c, 0x46d0, { 0x9e, 0xac, 0x94, 0xbf, 0x4e, 0x31, 0xc0, 0x89 } };

EFI_GUID  gSmmPagingProtectionsTestSmiHandlerGuid = SMM_PAGING_PROTECTIONS_TEST_SMI_HANDLER_GUID;

#endif // _SMM_PAGING_PROTECTIONS_TEST_COMMON_H_
