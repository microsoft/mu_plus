/**@file

PEI Library for controlling memory protection variables/settings

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>

#include <Ppi/ReadOnlyVariable2.h>
#include <Pi/PiMultiPhase.h>

#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryProtectionExceptionLib.h>
#include <Library/BaseMemoryLib.h>

#include "MemoryProtectionCommon.h"

/**
  Gets default (PCD) memory protection setting. This function will ASSERT if passed an invalid VarToken.

  @param[in] VarToken   MEMORY_PROTECTION_VAR_TOKEN representing variable
  @param[out] Setting    UINT32 populated with desired bitmask for memory protection setting

**/
STATIC
VOID
GetMemoryProtectionDefaultSetting (
    IN    MEMORY_PROTECTION_VAR_TOKEN      VarToken, 
    OUT   UINT32                          *Setting
  )
{
  if (VarToken == MEM_PROT_GLOBAL_TOGGLE_SETTING) {
    *Setting = (UINT32) (PcdGetBool (PcdDefaultMemoryProtectionGlobalToggle));
    return;
  }

  ASSERT (FALSE);
}

/**
  Gets the memory protections setting struct from Varstore.

  @param[out] Entry  The location to which the variable struct should be written

  @retval EFI_SUCCESS           The memory protection variable was fetched
  @retval EFI_OUT_OF_RESOURCES  There is no additional space in the PPI database
  @retval EFI_NOT_FOUND         The variable was not found
  @retval EFI_DEVICE_ERROR      The variable could not be retrieved because of a device error

**/
STATIC
EFI_STATUS
PeiGetMemProtUefiVariable (
  OUT   MEM_PROT_SETTINGS  *Entry
  )
{
  EFI_PEI_READ_ONLY_VARIABLE2_PPI *VariablePpi;
  EFI_STATUS                       Status;
  UINTN                            Size = sizeof (MEM_PROT_SETTINGS);

  Status = PeiServicesLocatePpi (
              &gEfiPeiReadOnlyVariable2PpiGuid,
              0,
              NULL,
              (VOID **) &VariablePpi
              );

  if (!EFI_ERROR (Status)){

    Status = VariablePpi->GetVariable (
                            VariablePpi,
                            MEMORY_PROTECTION_SETTINGS_VAR_NAME,
                            &gMemoryProtectionsGuid,
                            NULL,
                            &Size,
                            Entry
                            );
  }

  return Status;
}

/**
  Uses input to build the memory protection guided HOB.

  @param[in] Entry  Buffer containing a copy of the data written to the HOB
  
  @retval EFI_SUCCESS             HOB entry created
  @retval EFI_INVALID_PARAMETER   Entry was NULL
  @retval EFI_OUT_OF_RESOURCES    Failed to create HOB entry

**/
STATIC
EFI_STATUS
CreateMemoryProtectionHobEntry (
  IN  MEM_PROT_SETTINGS  *Entry
  )
{
  VOID *Ptr = NULL;

  if (Entry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Create the HOB entry
  Ptr = BuildGuidDataHob (
          &gHobMemoryProtectionsGuid,
          Entry,
          sizeof (MEM_PROT_SETTINGS)
          );

  if (Ptr != NULL) {
    return EFI_SUCCESS;
  }

  return EFI_OUT_OF_RESOURCES;
}

/**
  Generates the memory protections HOB entry by checking the Defaults, Overrides and UEFI variables
  on this platform.

  @param[out] Entry  Buffer containing a copy of the data written to the HOB

**/
STATIC
VOID
GenerateMemoryProtectionHobEntry (
  OUT   MEM_PROT_SETTINGS  *Entry
  )
{
  EFI_STATUS         Status;
  UINT32             Setting    = 0;

  if (Entry == NULL) {
    return;
  }

  Status = PeiGetMemProtUefiVariable (Entry);

  // Check if the memory protection variable exists and, if not, populate the setting with the PCD default
  if (EFI_ERROR (Status)) {
    GetMemoryProtectionDefaultSetting (
      MEM_PROT_GLOBAL_TOGGLE_SETTING,
      &Setting
      );

    Entry->MemProtGlobalToggle = (BOOLEAN) Setting;
  }

  Status = MemoryProtectionExceptionOverrideCheck (
             MEM_PROT_GLOBAL_TOGGLE_SETTING,
             &Setting
             );

  // Check if an override exists in platform early store indicating that on a previous boot and exception
  // potentially related to memory protections was hit
  if (!EFI_ERROR (Status)) {
    Entry->MemProtGlobalToggle = (BOOLEAN) Setting;
  }

  return;
}

/**
  Sets a memory protections setting. Valid MEMORY_PROTECTION_VAR_TOKEN values are
  defined MemoryProtectionExceptionLib.h. NOTE: In PEI, a setting can only be set if
  the HOB entry has not already been created. If successful, calling this function will
  create the HOB entry.

  @param[in] VarToken   MEMORY_PROTECTION_VAR_TOKEN representing variable
  @param[in] Setting    UINT32 populated with desired bitmask for memory protection setting

  @retval EFI_SUCCESS             Uefi variable has been updated
  @retval EFI_UNSUPPORTED         HOB Entry already created or failed to be created
  @retval EFI_INVALID_PARAMETER   VarToken invalid

**/
EFI_STATUS
InternalSetMemoryProtectionSetting (
  IN    MEMORY_PROTECTION_VAR_TOKEN      VarToken,
  IN    UINT32                           Setting
  )
{
  MEM_PROT_SETTINGS  Entry;
  EFI_STATUS         Status;

  Status = GetMemoryProtectionHobEntry (&Entry);

  // HOB Entry already created - cannot update any setting now
  if (!EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  GenerateMemoryProtectionHobEntry (&Entry);

  switch (VarToken)
  {
    case MEM_PROT_GLOBAL_TOGGLE_SETTING:
      Entry.MemProtGlobalToggle = (BOOLEAN) Setting;
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return CreateMemoryProtectionHobEntry (&Entry);
}

/**
  Gets the memory protections setting struct.

  @param[out] Entry  The location to which the variable struct should be written

  @retval EFI_SUCCESS             HOB entry found and copied to Entry
  @retval EFI_NOT_FOUND           HOB entry wasn't found
  @retval EFI_INVALID_PARAMETER   Failed to create HOB entry due to error in BuildGuidDataHob() call

**/
EFI_STATUS
InternalGetMemoryProtectionSettings (
  OUT   MEM_PROT_SETTINGS  *Entry
  )
{
  MEM_PROT_SETTINGS  HobEntry;
  EFI_STATUS         Status = EFI_SUCCESS;

  Status = GetMemoryProtectionHobEntry (Entry);

  if (EFI_ERROR (Status)) {
    GenerateMemoryProtectionHobEntry (&HobEntry);
    Status = CreateMemoryProtectionHobEntry (&HobEntry);

    if (!EFI_ERROR (Status)) {
      CopyMem (
        Entry,
        &HobEntry,
        sizeof (MEM_PROT_SETTINGS)
        );
    }
  }

  return Status;
}

