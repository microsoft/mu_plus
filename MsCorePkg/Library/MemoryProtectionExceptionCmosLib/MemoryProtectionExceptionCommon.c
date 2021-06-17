/**@file

Common functionality suppporting MemoryProtectionExceptionLib

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

#include <Library/IoLib.h>

#include "MemoryProtectionExceptionCommon.h"

/**

Reads bytes from CMOS. FOR INTERNAL USE ONLY - does not check the input sanity.

@param[in, out]   Ptr                       The pointer to data
@param[in]        Size                      Number of bytes to read
@param[in]        Address                   The CMOS address to read

**/
STATIC
VOID
MemProtCmosRead (
      OUT   VOID      *Ptr,
  IN        UINT8     Size,
  IN        UINT8     Address
  )
{
  UINT8 *buffer = Ptr;
  UINT8 i;

  for (i = 0; i < Size; i++) {
    IoWrite8 (PCAT_RTC_LO_ADDRESS_PORT, Address + i);
    buffer[i] = IoRead8 (PCAT_RTC_LO_DATA_PORT);
  }
}

/**

Writes bytes to CMOS. FOR INTERNAL USE ONLY - does not check the input sanity.

@param[in]  Ptr                       The pointer to data
@param[in]  Size                      Number of bytes to write
@param[in]  Address                   The CMOS address to write to

**/
STATIC
VOID
MemProtCmosWrite (
  IN  VOID       *Ptr,
  IN  UINT8      Size,
  IN  UINT8      Address
  )
{
  UINT8 *buffer = Ptr;
  UINT8 i;

  for (i = 0; i < Size; i++) {
    IoWrite8 (PCAT_RTC_LO_ADDRESS_PORT, Address + i);
    IoWrite8 (PCAT_RTC_LO_DATA_PORT, buffer[i]);
  }
}

/**

Sums across bytes in CMOS

@param[in]  Size                      Number of bytes to read
@param[in]  Address                   The CMOS address to begin summation

**/
STATIC
UINT16
MemProtSum (
  IN  UINT8      Size,
  IN  UINT8      Address
  ) 
{
  UINT8 i;
  UINT16 Sum = 0;

  for (i = 0; i < Size; i++) {
    IoWrite8 (PCAT_RTC_LO_ADDRESS_PORT, Address + i);
    Sum += IoRead8 (PCAT_RTC_LO_DATA_PORT);
  }

  return Sum;
}

/**
  Gets a memory protections setting from CMOS (if it's valid).

  @param[out] CmosBytes    Pointer to where the CMOS bytes will be populated.

  @retval EFI_SUCCESS            Getting CMOS bytes was successful
  @retval EFI_INVALID_PARAMETER  Checksum was invalid

**/
EFI_STATUS
MemoryProtectionReadCmosBytes (
  OUT  UINT16      *CmosBytes
  )
{

  if (!MemoryProtectionIsChecksumValid () || CmosBytes == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  MemProtCmosRead (
    CmosBytes,
    CMOS_MEM_PROT_DATA_SIZE,
    CMOS_MEM_PROT_DATA_START
    );

  return EFI_SUCCESS;
}

/**
  Writes to the memory protections variable region in CMOS

  @param[in] Value   Value to be written to memory protection CMOS region.

**/
VOID
MemoryProtectionWriteCmosBytes (
  IN  UINT16 Value
  )
{
  MemProtCmosWrite (
    &Value,
    CMOS_MEM_PROT_DATA_SIZE,
    CMOS_MEM_PROT_DATA_START
    );

  MemoryProtectionUpdateChecksumCmos ();
}

/**
  Routine to update the checksum based on the memory protection CMOS bytes.

**/
VOID
MemoryProtectionUpdateChecksumCmos (
  VOID
  )
{
  UINT16 Checksum = 0;

  Checksum = MemProtSum (
               CMOS_MEM_PROT_DATA_SIZE,
               CMOS_MEM_PROT_DATA_START
               );

  MemProtCmosWrite (
    &Checksum,
    CMOS_MEM_PROT_CHECKSUM_SIZE,
    CMOS_MEM_PROT_CHECKSUM_START
    );
}

/**
  Returns if the checksum region value matches the sum of memory protection CMOS data

  @retval BOOLEAN   If the memory protection CMOS checksum is valid

**/
BOOLEAN
MemoryProtectionIsChecksumValid (
  VOID
  )
{
  UINT16 Checksum, Sum;

  MemProtCmosRead (
    &Checksum,
    CMOS_MEM_PROT_CHECKSUM_SIZE,
    CMOS_MEM_PROT_CHECKSUM_START
    );

  Sum = MemProtSum (
          CMOS_MEM_PROT_DATA_SIZE,
          CMOS_MEM_PROT_DATA_START
          );
   
  return Checksum == Sum;
}
