/** @file -- DxePagingAuditCommon.h

This DXE Driver writes page table and memory map information to SFS when triggered
by an event.

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

#define    MEM_INFO_DATABASE_REALLOC_CHUNK   0x1000
#define    MEM_INFO_DATABASE_MAX_STRING_SIZE 0x400  
#define    MAX_STRING_SIZE 0x1000
#define    ADDRESS_BITS 0x0000007FFFFFF000ull

#define IndexToAddress(a, b, c, d)   ((UINT64) (a << 39) + (b << 30) + (c <<  21) + (d << 12))

// volatile UINT32 jason = 0;

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


#pragma pack()