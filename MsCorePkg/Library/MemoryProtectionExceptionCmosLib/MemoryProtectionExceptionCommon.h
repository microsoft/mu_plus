/**@file

Common functionality suppporting MemoryProtectionExceptionLib

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

#include <Library/IoLib.h>

/**
  Reads the memory protections variable region in CMOS

  @retval UINT8     Value currently in memory protections variable region in CMOS.

**/
UINT8
CmosReadMemoryProtectionByte (
  VOID
  );

/**
  Writes to the memory protections variable region in CMOS

  @retval UINT8     Value written to the memory protections variable region in CMOS.

**/
UINT8
CmosWriteMemoryProtectionByte (
    IN  UINT8 Value
  );

#define CMOS_MEM_PROT_BYTE_LOC 0X10
#define CMOS_MEM_PROT_VALID_BIT_MASK 0x1
#define CMOS_MEM_PROT_TOG_BIT_MASK 0x2
#define PCAT_RTC_LO_ADDRESS_PORT 0x70
#define PCAT_RTC_LO_DATA_PORT 0x71
