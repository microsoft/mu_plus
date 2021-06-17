/**@file

Common functionality suppporting MemoryProtectionLib

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _MEM_PROT_COMMON_H_
#define _MEM_PROT_COMMON_H_

typedef struct {
    BOOLEAN  MemProtGlobalToggle;
} MEM_PROT_SETTINGS;

#define MEMORY_PROTECTION_SETTINGS_VAR_NAME L"MemProtUefiVar"

/**
  Gets a memory protection setting from the HOB. 

  @param[out] Entry  The location to which the variable struct should be written

  @retval EFI_SUCCESS     HOB entry found and copied to Entry
  @retval EFI_NOT_FOUND   HOB entry wasn't found

**/
EFI_STATUS
GetMemoryProtectionHobEntry (
    OUT   MEM_PROT_SETTINGS  *Entry
  );

/**
  Gets the memory protections setting struct. This call will simply copy the previously
  fetched struct if available. Otherwise, it will get the data from the HOB.

  @param[out] Entry  The location to which the variable struct should be written

  @retval EFI_SUCCESS      Memory protection settings fetched
  @retval EFI_NOT_FOUND    Memory protection settings weren't found

**/
EFI_STATUS
InternalGetMemoryProtectionSettings (
    OUT MEM_PROT_SETTINGS  *Entry
  );

/**
  Sets a memory protections setting. Valid MEMORY_PROTECTION_VAR_TOKEN values are
  defined MemoryProtectionExceptionLib.h.

  @param[in] VarToken   MEMORY_PROTECTION_VAR_TOKEN representing variable
  @param[in] Setting    UINT32 populated with desired bitmask for memory protection setting

  @retval EFI_SUCCESS       Uefi variable has been updated
  @retval EFI_NOT_READY     gBS hasn't been initialized yet
  @retval EFI_NOT_FOUND     Could not find the variable

**/
EFI_STATUS
InternalSetMemoryProtectionSetting (
    IN    MEMORY_PROTECTION_VAR_TOKEN      VarToken,
    IN    UINT32                           Setting
  );

#endif
