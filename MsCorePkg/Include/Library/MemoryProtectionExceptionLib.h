/**@file

Library provides access to the memory protection setting which may exist
in the platform-specific early store due to a memory related exception being triggered.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _MEM_PROT_EX_LIB_H_
#define _MEM_PROT_EX_LIB_H_

typedef UINT8   MEMORY_PROTECTION_VAR_TOKEN;
typedef UINT16  MEMORY_PROTECTION_OVERRIDE;

// Bit definitions for MEMORY_PROTECTION_OVERRIDE
#define MEM_PROT_VALID_BIT       BIT0   // Is the override valid
#define MEM_PROT_TOG_BIT         BIT1   // Is the global toggle on
#define MEM_PROT_EX_HIT_BIT      BIT7   // Was an exception hit

#define MEM_PROT_GLOBAL_TOGGLE_SETTING ((MEMORY_PROTECTION_VAR_TOKEN) 0x0)

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
  );

/**
  Checks if an exception was hit on a previous boot.

  @retval TRUE          Checksum is valid and an exception was hit on a previous boot.
  @retval FALSE         Checksum was false or an exception was not hit on a previous boot.

**/
BOOLEAN
EFIAPI
MemoryProtectionExceptionOccurred (
  VOID
  );

/**
  Clears the memory protection setting from the platform-specific early store.

  @retval EFI_SUCCESS       Always return success

**/
VOID
EFIAPI
MemoryProtectionExceptionOverrideClear (
  VOID
  );


/**
  Writes Input Value to early store

  @param Val MEMORY_PROTECTION_OVERRIDE value to write

**/
VOID
EFIAPI
MemoryProtectionExceptionOverrideWrite (
  MEMORY_PROTECTION_OVERRIDE Val
  );

#endif
