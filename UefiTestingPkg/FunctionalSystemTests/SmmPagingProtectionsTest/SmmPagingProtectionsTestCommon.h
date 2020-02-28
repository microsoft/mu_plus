/** @file -- SmmPagingProtectionsTestCommon.h
Shared definitions between the DXE and SMM drivers.
Mostly used for SMM communication.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

// MS_CHANGE - Entire file created.

#ifndef _SMM_PAGING_PROTECTIONS_TEST_COMMON_H_
#define _SMM_PAGING_PROTECTIONS_TEST_COMMON_H_

#define SMM_PAGING_PROTECTIONS_SELF_TEST_CODE            1
#define SMM_PAGING_PROTECTIONS_SELF_TEST_DATA            2
#define SMM_PAGING_PROTECTIONS_TEST_INVALID_RANGE        3
#define SMM_PROTECTIONS_READ_UNAUTHORIZED_IO             4
#define SMM_PROTECTIONS_WRITE_UNAUTHORIZED_IO            5
#define SMM_PROTECTIONS_READ_UNAUTHORIZED_MSR            6
#define SMM_PROTECTIONS_WRITE_UNAUTHORIZED_MSR           7
#define SMM_PROTECTIONS_PRIVILEGED_INSTRUCTIONS          8
#define SMM_PROTECTIONS_ACCESS_ENTRY_POINT               9
#define SMM_PROTECTIONS_RUN_ARBITRARY_NON_SMM_CODE       10

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
