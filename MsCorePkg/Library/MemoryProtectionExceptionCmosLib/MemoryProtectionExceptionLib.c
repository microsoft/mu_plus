/**@file

Library provides access to the memory protection setting which may exist
in the platform-specific early store due to a memory related exception being triggered.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

#include <Library/MemoryProtectionLib.h>
#include <Library/MemoryProtectionExceptionLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>

#include "MemoryProtectionExceptionCommon.h"

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
  IN    MEMORY_PROTECTION_VAR_TOKEN      VarToken,
  OUT   UINT32                          *Setting
  )
{
  UINT16 CmosVal;
  EFI_STATUS Status;

  Status = MemoryProtectionReadCmosBytes (&CmosVal);

  if (Setting == NULL || EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  if (CmosVal & CMOS_MEM_PROT_VALID_BIT) {
  
    if (VarToken == MEM_PROT_GLOBAL_TOGGLE_SETTING) {
      *Setting = (CmosVal & CMOS_MEM_PROT_TOG_BIT) >> 1;
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
MemoryProtectionDidSystemHitException (
  VOID
  )
{
  UINT16 CmosVal = 0;

  MemoryProtectionReadCmosBytes (&CmosVal);

  if ((CmosVal & (CMOS_MEM_PROT_VALID_BIT | CMOS_MEM_PROT_EX_HIT_BIT)) == 
      (CMOS_MEM_PROT_VALID_BIT | CMOS_MEM_PROT_EX_HIT_BIT)) {
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
  IN    MEMORY_PROTECTION_VAR_TOKEN      VarToken,
  OUT   UINT32                          *Setting
  )
{
  if (Setting == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return GetMemoryProtectionCmosSetting(VarToken, Setting);
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
  MemoryProtectionWriteCmosBytes(0);
}
