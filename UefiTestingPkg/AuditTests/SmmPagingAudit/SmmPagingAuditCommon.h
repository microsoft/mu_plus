/** @file -- PageTableDumpCommon.h
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

#ifndef _SMM_PAGING_AUDIT_COMMON_H_
#define _SMM_PAGING_AUDIT_COMMON_H_

#define MAX_STRING_SIZE 0x1000
#define ADDRESS_BITS 0x0000007FFFFFF000ull

#pragma pack(1)

//
// Page-Map Level-4 Offset (PML4) and
// Page-Directory-Pointer Offset (PDPE) entries 4K & 2MB
//
typedef union {
  struct {
    UINT64  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    UINT64  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    UINT64  UserSupervisor:1;         // 0 = Supervisor, 1=User
    UINT64  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    UINT64  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    UINT64  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64  Reserved:1;               // Reserved
    UINT64  MustBeZero:2;             // Must Be Zero
    UINT64  Available:3;              // Available for use by system software
    UINT64  PageTableBaseAddress:40;  // Page Table Base Address
    UINT64  AvailableHigh:11;        // Available for use by system software
    UINT64  Nx:1;                     // No Execute bit
  } Bits;
  UINT64    Uint64;
} PAGE_MAP_AND_DIRECTORY_POINTER;

//
// Page Table Entry 4KB
//
typedef union {
  struct {
    UINT64  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    UINT64  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    UINT64  UserSupervisor:1;         // 0 = Supervisor, 1=User
    UINT64  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    UINT64  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    UINT64  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64  Dirty:1;                  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64  PAT:1;                    //
    UINT64  Global:1;                 // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64  Available:3;              // Available for use by system software
    UINT64  PageTableBaseAddress:40;  // Page Table Base Address
    UINT64  AvailableHigh:11;        // Available for use by system software
    UINT64  Nx:1;                     // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_4K_ENTRY;

//
// Page Table Entry 2MB
//
typedef union {
  struct {
    UINT64  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    UINT64  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    UINT64  UserSupervisor:1;         // 0 = Supervisor, 1=User
    UINT64  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    UINT64  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    UINT64  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64  Dirty:1;                  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64  MustBe1:1;                // Must be 1
    UINT64  Global:1;                 // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64  Available:3;              // Available for use by system software
    UINT64  PAT:1;                    //
    UINT64  MustBeZero:8;             // Must be zero;
    UINT64  PageTableBaseAddress:31;  // Page Table Base Address
    UINT64  AvailableHigh:11;        // Available for use by system software
    UINT64  Nx:1;                     // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_ENTRY;

//
// Page Table Entry 1GB
//
typedef union {
  struct {
    UINT64  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    UINT64  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    UINT64  UserSupervisor:1;         // 0 = Supervisor, 1=User
    UINT64  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    UINT64  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    UINT64  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64  Dirty:1;                  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64  MustBe1:1;                // Must be 1
    UINT64  Global:1;                 // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64  Available:3;              // Available for use by system software
    UINT64  PAT:1;                    //
    UINT64  MustBeZero:17;            // Must be zero;
    UINT64  PageTableBaseAddress:22;  // Page Table Base Address
    UINT64  AvailableHigh:11;        // Available for use by system software
    UINT64  Nx:1;                     // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_1G_ENTRY;

#define MAX_IMAGE_NAME_SIZE   100

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
