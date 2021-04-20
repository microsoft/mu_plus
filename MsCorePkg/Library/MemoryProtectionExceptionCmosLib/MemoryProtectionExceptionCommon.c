/**@file

Common functionality suppporting MemoryProtectionExceptionLib

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "MemoryProtectionExceptionCommon.h"

/**
  Reads the memory protections variable region in CMOS

  @retval UINT8     Value currently in memory protections variable region in CMOS.

**/
UINT8
CmosReadMemoryProtectionByte (
  VOID
  )
{
  IoWrite8 (PCAT_RTC_LO_ADDRESS_PORT, (UINT8) CMOS_MEM_PROT_BYTE_LOC);
  return IoRead8 (PCAT_RTC_LO_DATA_PORT);
}

/**
  Writes to the memory protections variable region in CMOS

  @retval UINT8     Value written to the memory protections variable region in CMOS.

**/
UINT8
CmosWriteMemoryProtectionByte (
    IN  UINT8 Value
  )
{
  IoWrite8 (PCAT_RTC_LO_ADDRESS_PORT, (UINT8) CMOS_MEM_PROT_BYTE_LOC);
  IoWrite8 (PCAT_RTC_LO_DATA_PORT, Value);
  return Value;
}
