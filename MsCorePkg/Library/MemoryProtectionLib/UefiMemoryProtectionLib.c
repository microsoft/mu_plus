/**@file

DXE Library for controlling memory protection variables/settings

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include "MemoryProtectionCommon.h"

/**
  Gets the memory protections setting struct. This will first attempt to copy the Varstore variable into Entry
  and, if that does not exist, it will copy the HOB variable.

  @param[out] Entry  The location to which the variable struct should be written 

  @retval EFI_SUCCESS      Memory protection fetched
  @retval EFI_NOT_FOUND    HOB entry and UEFI variable weren't found

**/
EFI_STATUS
InternalGetMemoryProtectionSettings (
  OUT   MEM_PROT_SETTINGS  *Entry
  )
{

  EFI_STATUS   Status;
  UINTN        Size = sizeof(MEM_PROT_SETTINGS);

  Status = gRT->GetVariable(MEMORY_PROTECTION_SETTINGS_VAR_NAME,
                            &gMemoryProtectionsGuid,
                            NULL,
                            &Size,
                            Entry);

  if(EFI_ERROR(Status)) {
    Status = GetMemoryProtectionHobEntry(Entry);
  }

  return Status;
}

/**
  Sets a memory protections setting. Valid MEMORY_PROTECTION_VAR_TOKEN values are
  defined MemoryProtectionExceptionLib.h.

  @param[in] VarToken   MEMORY_PROTECTION_VAR_TOKEN representing variable.
  @param[in] Setting    UINT32 populated with desired bitmask for memory protection setting.

  @retval EFI_SUCCESS             Uefi variable has been updated.
  @retval EFI_NOT_FOUND           Could not find the variable.
  @retval EFI_OUT_OF_RESOURCES    Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR        The variable could not be retrieved due to a hardware error.
  @retval EFI_WRITE_PROTECTED     The variable in question is read-only.
  @retval EFI_WRITE_PROTECTED     The variable in question cannot be deleted.
  @retval EFI_SECURITY_VIOLATION  The variable could not be written due to EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACESSS being set,
                                  but the AuthInfo does NOT pass the validation check carried out by the firmware.

**/
EFI_STATUS
InternalSetMemoryProtectionSetting (
  IN    MEMORY_PROTECTION_VAR_TOKEN      VarToken,
  IN    UINT32                           Setting
  )
{
  EFI_STATUS         Status = EFI_SUCCESS;
  MEM_PROT_SETTINGS  Var;

  // Get the current settings
  InternalGetMemoryProtectionSettings(&Var);

  // Check which setting is being updated
  switch(VarToken)
  {
    case MEM_PROT_GLOBAL_TOGGLE_SETTING:
      Var.MemProtGlobalToggle = (BOOLEAN) Setting;
      break;
    default:
      return EFI_NOT_FOUND;
  }

  // Update the setting
  Status = gRT->SetVariable(MEMORY_PROTECTION_SETTINGS_VAR_NAME,
                            &gMemoryProtectionsGuid,
                            EFI_VARIABLE_NON_VOLATILE |
                            EFI_VARIABLE_BOOTSERVICE_ACCESS,
                            sizeof(MEM_PROT_SETTINGS),
                            &Var);

  // Clear the exception override to ensure we use this updated setting from now on
  if(!EFI_ERROR (Status)) {
    ClearMemoryProtectionExceptionOverride();
  }  

  return Status;
}
