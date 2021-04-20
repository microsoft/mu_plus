/**@file

DXE/SMM Library for controlling memory protection variables/settings

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>

#include "MemoryProtectionCommon.h"

STATIC BOOLEAN           HaveFetchedHob = FALSE;
STATIC MEM_PROT_SETTINGS mFetchedHob;

/**
  Gets the memory protections setting struct. This call will simply copy the previously
  fetched struct if available. Otherwise, it will get the data from the HOB.

  @param[out] Entry  The location to which the variable struct should be written

  @retval EFI_SUCCESS      Memory protection fetched
  @retval EFI_NOT_FOUND    HOB entry wasn't found

**/
EFI_STATUS
InternalGetMemoryProtectionSettings (
  OUT MEM_PROT_SETTINGS  *Entry
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  if(!HaveFetchedHob) {
    Status = GetMemoryProtectionHobEntry(&mFetchedHob);
    if(EFI_ERROR(Status)) {
      DEBUG((DEBUG_WARN, "%a: - DxeSmmMemoryProtectionLib could not locate the HOB entry for \
        Memory Protections.\n", __FUNCTION__));
    }
  }

  if (!EFI_ERROR(Status)) {
    CopyMem (Entry, &mFetchedHob, sizeof(MEM_PROT_SETTINGS));
  }

  return Status;
}

/**
  Sets a memory protections setting. Valid MEMORY_PROTECTION_VAR_TOKEN values are
  defined MemoryProtectionExceptionLib.h.

  @param VarToken   MEMORY_PROTECTION_VAR_TOKEN representing variable.
  @param Setting    UINT32 populated with desired bitmask for memory protection setting.

  @retval EFI_SUCCESS       Uefi variable has been updated.
  @retval EFI_NOT_READY     gBS hasn't been initialized yet.
  @retval EFI_NOT_FOUND     Could not find the variable.

**/
EFI_STATUS
InternalSetMemoryProtectionSetting (
  IN    MEMORY_PROTECTION_VAR_TOKEN      VarToken,
  IN    UINT32                           Setting
  )
{
  return EFI_UNSUPPORTED;
}


/**
  Constructor for Dxe/Smm memory protection lib

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

 **/
EFI_STATUS
EFIAPI
DxeSmmMemoryProtectionLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = GetMemoryProtectionHobEntry(&mFetchedHob);

  if(EFI_ERROR(Status)) {
    DEBUG((DEBUG_WARN, "%a: - DxeSmmMemoryProtectionLib could not locate the HOB entry for \
      Memory Protections.\n", __FUNCTION__));
    return EFI_SUCCESS;
  }

  HaveFetchedHob = TRUE;
  return EFI_SUCCESS;

}