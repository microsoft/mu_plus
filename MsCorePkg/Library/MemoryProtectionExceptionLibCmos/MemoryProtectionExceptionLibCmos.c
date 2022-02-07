/**@file

Library provides access to the memory protection setting which may exist
in the platform-specific early store (in this case, CMOS) due to a memory related exception being triggered.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

#include <Library/MemoryProtectionExceptionLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>

typedef UINT16 MEMORY_PROTECTION_OVERRIDE_CHECKSUM;

#define CMOS_MEM_PROT_CHECKSUM_START  0x10
#define CMOS_MEM_PROT_CHECKSUM_SIZE   sizeof (MEMORY_PROTECTION_OVERRIDE_CHECKSUM)
#define CMOS_MEM_PROT_DATA_START      CMOS_MEM_PROT_CHECKSUM_START + CMOS_MEM_PROT_CHECKSUM_SIZE
#define CMOS_MEM_PROT_DATA_SIZE       sizeof (MEMORY_PROTECTION_OVERRIDE)

#define PCAT_RTC_LO_ADDRESS_PORT  0x70
#define PCAT_RTC_LO_DATA_PORT     0x71

/**

Reads bytes from CMOS. FOR INTERNAL USE ONLY - does not check the input sanity.

@param[in, out]   Ptr                       The pointer to data
@param[in]        Size                      Number of bytes to read
@param[in]        Address                   The CMOS address to read

**/
STATIC
VOID
MemProtCmosRead (
  OUT   VOID       *Ptr,
  IN        UINT8  Size,
  IN        UINT8  Address
  )
{
  UINT8  *buffer = Ptr;
  UINT8  i;

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
  IN  VOID   *Ptr,
  IN  UINT8  Size,
  IN  UINT8  Address
  )
{
  UINT8  *buffer = Ptr;
  UINT8  i;

  for (i = 0; i < Size; i++) {
    IoWrite8 (PCAT_RTC_LO_ADDRESS_PORT, Address + i);
    IoWrite8 (PCAT_RTC_LO_DATA_PORT, buffer[i]);
  }
}

/**

Sums across bytes in memory protection CMOS region

**/
STATIC
MEMORY_PROTECTION_OVERRIDE_CHECKSUM
MemProtSum (
  VOID
  )
{
  UINT8                                i, Address;
  MEMORY_PROTECTION_OVERRIDE_CHECKSUM  Sum = 0;

  Address = CMOS_MEM_PROT_DATA_START;

  for (i = 0; i < CMOS_MEM_PROT_DATA_SIZE; i++) {
    IoWrite8 (PCAT_RTC_LO_ADDRESS_PORT, Address + i);
    Sum += IoRead8 (PCAT_RTC_LO_DATA_PORT);
  }

  return Sum;
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
  MEMORY_PROTECTION_OVERRIDE           Sum;
  MEMORY_PROTECTION_OVERRIDE_CHECKSUM  Checksum;

  MemProtCmosRead (
    &Checksum,
    CMOS_MEM_PROT_CHECKSUM_SIZE,
    CMOS_MEM_PROT_CHECKSUM_START
    );

  Sum = MemProtSum ();

  return Checksum == Sum;
}

/**
  Gets a memory protections setting from CMOS (if it's valid).

  @param[out] CmosBytes    Pointer to where the CMOS bytes will be populated.

  @retval EFI_SUCCESS            Getting CMOS bytes was successful
  @retval EFI_INVALID_PARAMETER  Checksum was invalid

**/
EFI_STATUS
MemoryProtectionReadCmosBytes (
  OUT  MEMORY_PROTECTION_OVERRIDE  *CmosBytes
  )
{
  if (!MemoryProtectionIsChecksumValid () || (CmosBytes == NULL)) {
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
  Routine to update the checksum based on the memory protection CMOS bytes.

**/
VOID
MemoryProtectionUpdateChecksumCmos (
  VOID
  )
{
  MEMORY_PROTECTION_OVERRIDE_CHECKSUM  Checksum = 0;

  Checksum = MemProtSum ();

  MemProtCmosWrite (
    &Checksum,
    CMOS_MEM_PROT_CHECKSUM_SIZE,
    CMOS_MEM_PROT_CHECKSUM_START
    );
}

/**
  Writes to the memory protections variable region in CMOS

  @param[in] Value   Value to be written to memory protection CMOS region.

**/
VOID
MemoryProtectionWriteCmosBytes (
  IN  MEMORY_PROTECTION_OVERRIDE  Value
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
  Gets a memory protections setting from CMOS (if it's valid).

  @param[in]     VarToken   MEMORY_PROTECTION_VAR_TOKEN representing variable.
  @param[in,out] Setting    UINT32 populated with bitmask for current memory protection setting.

  @retval EFI_SUCCESS            Setting now contains bitmask for String memory setting.
  @retval EFI_NOT_FOUND          Memory protections variable region in CMOS is invalid.
  @retval EFI_INVALID_PARAMETER  Setting address was NULL or checksum was invalid

**/
EFI_STATUS
GetMemoryProtectionCmosSetting (
  IN    MEMORY_PROTECTION_VAR_TOKEN  VarToken,
  OUT   UINT32                       *Setting
  )
{
  MEMORY_PROTECTION_OVERRIDE  CmosVal;
  EFI_STATUS                  Status;

  Status = MemoryProtectionReadCmosBytes (&CmosVal);

  if ((Setting == NULL) || EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  if (CmosVal & MEM_PROT_VALID_BIT) {
    if (VarToken == MEM_PROT_GLOBAL_TOGGLE_SETTING) {
      *Setting = (CmosVal & MEM_PROT_TOG_BIT) >> 1;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Checks if an exception was hit on a previous boot.

  @retval TRUE          Checksum is valid and an exception was hit on a previous boot
  @retval FALSE         Checksum was false or an exception was not hit on a previous boot

**/
BOOLEAN
EFIAPI
MemoryProtectionExceptionOccurred (
  VOID
  )
{
  MEMORY_PROTECTION_OVERRIDE  CmosVal = 0;

  MemoryProtectionReadCmosBytes (&CmosVal);

  if ((CmosVal & (MEM_PROT_VALID_BIT | MEM_PROT_EX_HIT_BIT)) ==
      (MEM_PROT_VALID_BIT | MEM_PROT_EX_HIT_BIT))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Gets a memory protection setting from the platform-specific early store. This setting value is only intended
  to exist in early store if an exception was hit potentially related to memory protections.

  @param[in]     VarToken   MEMORY_PROTECTION_VAR_TOKEN representing variable.
  @param[in,out] Setting    UINT32 populated with bitmask for current memory protection setting.

  @retval EFI_SUCCESS             Setting now contains bitmask for String memory setting.
  @retval EFI_NOT_FOUND           Memory protections variable region in CMOS is invalid.
  @retval EFI_INVALID_PARAMETER   Setting was NULL

**/
EFI_STATUS
EFIAPI
MemoryProtectionExceptionOverrideCheck (
  IN    MEMORY_PROTECTION_VAR_TOKEN  VarToken,
  OUT   UINT32                       *Setting
  )
{
  if (Setting == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return GetMemoryProtectionCmosSetting (VarToken, Setting);
}

/**
  Clears the memory protection setting from the platform-specific early store.

  @retval EFI_SUCCESS       Always return success

**/
VOID
EFIAPI
MemoryProtectionExceptionOverrideClear (
  VOID
  )
{
  MemoryProtectionWriteCmosBytes ((MEMORY_PROTECTION_OVERRIDE)0);
}

/**
  Writes Input Value to early store

  @param Val MEMORY_PROTECTION_OVERRIDE value to write

**/
VOID
EFIAPI
MemoryProtectionExceptionOverrideWrite (
  MEMORY_PROTECTION_OVERRIDE  Val
  )
{
  MemoryProtectionWriteCmosBytes (Val);
}
