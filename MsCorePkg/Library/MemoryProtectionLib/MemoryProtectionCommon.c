/**@file

Common functionality suppporting MemoryProtectionLib

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "MemoryProtectionCommon.h"

/**
  Gets a memory protection setting from the HOB. 

  @param[out] Entry  The location to which the variable struct should be written

  @retval EFI_SUCCESS     HOB entry found and copied to Entry
  @retval EFI_NOT_FOUND   HOB entry wasn't found

**/
EFI_STATUS
GetMemoryProtectionHobEntry (
    OUT   MEM_PROT_SETTINGS  *Entry
  )
{
  VOID *Ptr = GetFirstGuidHob(&gHobMemoryProtectionsGuid);

  if(Ptr == NULL) {
    return EFI_NOT_FOUND;
  }

  CopyMem(Entry, GET_GUID_HOB_DATA(Ptr), sizeof(MEM_PROT_SETTINGS));
  return EFI_SUCCESS;
}

/**
 Updates the memory protection global toggle

  @retval TRUE            Memory protection global toggle is on
  @retval FALSE           Memory protection global toggle is off, meaning
                          no memory protections can be initialized

 **/
BOOLEAN
EFIAPI
IsMemoryProtectionGlobalToggleEnabled (
  VOID
  )
{
  MEM_PROT_SETTINGS Settings;

  if (EFI_ERROR(InternalGetMemoryProtectionSettings(&Settings))) {
    return TRUE;
  }

  return Settings.MemProtGlobalToggle;
}

/**
 Updates the memory protection global toggle

  @param[in] Setting      What the memory protection global toggle should
                          be set to

  @retval EFI_SUCCESS     Memory protection global toggle was updated
  @retval EFI_UNSUPPORTED This function is either not available in this
                          execution phase or a null instance is used

 **/
EFI_STATUS
EFIAPI
SetMemoryProtectionGlobalToggle (
  IN BOOLEAN     Setting
  )
{
  return InternalSetMemoryProtectionSetting(MEM_PROT_GLOBAL_TOGGLE_SETTING, (UINT32) Setting);
}
