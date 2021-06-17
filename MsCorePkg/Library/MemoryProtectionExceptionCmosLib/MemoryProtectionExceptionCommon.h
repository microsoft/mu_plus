/**@file

Common functionality suppporting MemoryProtectionExceptionLib

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MEM_PROT_EX_COMMON_H__
#define __MEM_PROT_EX_COMMON_H__

#define CMOS_MEM_PROT_CHECKSUM_START  0x10
#define CMOS_MEM_PROT_CHECKSUM_SIZE   0x2
#define CMOS_MEM_PROT_DATA_START      CMOS_MEM_PROT_CHECKSUM_START + CMOS_MEM_PROT_CHECKSUM_SIZE
#define CMOS_MEM_PROT_DATA_SIZE       0x2

#define CMOS_MEM_PROT_VALID_BIT       BIT0
#define CMOS_MEM_PROT_TOG_BIT         BIT1
#define CMOS_MEM_PROT_EX_HIT_BIT      BIT7
#define PCAT_RTC_LO_ADDRESS_PORT      0x70
#define PCAT_RTC_LO_DATA_PORT         0x71

/**
  Gets a memory protections setting from CMOS (if it's valid).

  @param[out] CmosBytes    Pointer to where the CMOS bytes will be populated.

  @retval EFI_SUCCESS            Getting CMOS bytes was successful
  @retval EFI_INVALID_PARAMETER  Checksum was invalid

**/
EFI_STATUS
MemoryProtectionReadCmosBytes (
  OUT  UINT16      *CmosBytes
  );

/**
  Writes to the memory protections variable region in CMOS

  @param[in] Value   Value to be written to memory protection CMOS region.

**/
VOID
MemoryProtectionWriteCmosBytes (
  IN  UINT16 Value
  );

/**
  Routine to update the checksum based on the memory protection CMOS bytes.

**/
VOID
MemoryProtectionUpdateChecksumCmos (
  VOID
  );

/**
  Returns if the checksum region value matches the sum of memory protection CMOS data

  @retval BOOLEAN   If the memory protection CMOS checksum is valid

**/
BOOLEAN
MemoryProtectionIsChecksumValid (
  VOID
  );

#endif
