/** @file -- PageTableDumpCommon.h
Shared definitions between the DXE and SMM drivers.
Mostly used for SMM communication.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_PAGING_AUDIT_COMMON_H_
#define _SMM_PAGING_AUDIT_COMMON_H_

#pragma pack(1)

#define MAX_IMAGE_NAME_SIZE   100
#define MAX_SMI_CALL_COUNT    1000

typedef struct {
  UINT64              ImageBase;
  UINT64              ImageSize;
  CHAR8               ImageName[MAX_IMAGE_NAME_SIZE];
} IMAGE_STRUCT;

#define BUFFER_COUNT_1G       300
#define BUFFER_COUNT_2M       500
#define BUFFER_COUNT_4K       1000
#define BUFFER_COUNT_PDE      20
#define BUFFER_COUNT_IMAGES   25

#define SMM_PAGE_AUDIT_TABLE_REQUEST      0x01
#define SMM_PAGE_AUDIT_PDE_REQUEST        0x02
#define SMM_PAGE_AUDIT_MISC_DATA_REQUEST  0x03
#define SMM_PAGE_AUDIT_CLEAR_DATA_REQUEST 0x04

//
// Structures for page table entries and miscellaneous memory
// information from SMM. 
//
typedef struct _SMM_PAGE_AUDIT_COMM_HEADER {
  UINTN                   RequestType;
  UINTN                   RequestIndex;
} SMM_PAGE_AUDIT_COMM_HEADER;

typedef struct _PAGE_TABLE_ENTRY_COMM_BUFFER {
  PAGE_TABLE_1G_ENTRY     Pte1G[BUFFER_COUNT_1G];
  PAGE_TABLE_ENTRY        Pte2M[BUFFER_COUNT_2M];
  PAGE_TABLE_4K_ENTRY     Pte4K[BUFFER_COUNT_4K];
  UINTN                   Pte1GCount;
  UINTN                   Pte2MCount;
  UINTN                   Pte4KCount;
  BOOLEAN                 HasMore;
} SMM_PAGE_AUDIT_TABLE_ENTRY_COMM_BUFFER;

typedef struct _SMM_PAGE_AUDIT_PDE_ENTRY_COMM_BUFFER {
  UINT64                  Pde[BUFFER_COUNT_PDE];
  UINTN                   PdeCount;
  BOOLEAN                 HasMore;
} SMM_PAGE_AUDIT_PDE_ENTRY_COMM_BUFFER;

typedef struct _SMM_PAGE_AUDIT_MISC_DATA_COMM_BUFFER {
  IA32_DESCRIPTOR         Gdtr;
  IA32_DESCRIPTOR         Idtr;
  IMAGE_STRUCT            SmmImage[BUFFER_COUNT_IMAGES];
  UINTN                   SmmImageCount;
  BOOLEAN                 HasMore;
} SMM_PAGE_AUDIT_MISC_DATA_COMM_BUFFER;

typedef struct _SMM_PAGE_AUDIT_UNIFIED_COMM_BUFFER
{
  SMM_PAGE_AUDIT_COMM_HEADER  Header;
  union {
    SMM_PAGE_AUDIT_TABLE_ENTRY_COMM_BUFFER    TableEntry;
    SMM_PAGE_AUDIT_PDE_ENTRY_COMM_BUFFER      PdeEntry;
    SMM_PAGE_AUDIT_MISC_DATA_COMM_BUFFER      MiscData;
  } Data;
} SMM_PAGE_AUDIT_UNIFIED_COMM_BUFFER;

#pragma pack()

// {81B8D274-114B-4627-97A1-F5A41647AC12}
#define SMM_PAGING_AUDIT_SMI_HANDLER_GUID \
{ 0x81b8d274, 0x114b, 0x4627, { 0x97, 0xa1, 0xf5, 0xa4, 0x16, 0x47, 0xac, 0x12 } };


EFI_GUID  gSmmPagingAuditSmiHandlerGuid = SMM_PAGING_AUDIT_SMI_HANDLER_GUID;

#endif // _SMM_PAGING_AUDIT_COMMON_H_
