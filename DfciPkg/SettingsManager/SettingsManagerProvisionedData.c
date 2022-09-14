/**@file
SettingsManagerProvisionedData

This file supports loading and saving internal data (previously provisioned) from flash so that
SettingsManager code can use it.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SettingsManager.h"

// Define the local structure for the variable (this is for internal use only)
// Variable namespace - Use gEfiCallerIdGuid since this is internal only

#define VAR_NAME        L"_SMID"
#define VAR_HEADER_SIG  SIGNATURE_32('S', 'M', 'I', 'D')
#define VAR_VERSION     (1)

// determine min size to make sure variable is big enough to evaluate.  This is header signature plus header version
#define MIN_VAR_SIZE  (sizeof(UINT32) + sizeof(UINT8))

#pragma pack (push, 1)

typedef struct {
  UINT32      HeaderSignature; // 'S', 'M', 'I', 'D'
  UINT8       HeaderVersion;   // 1
  UINT32      Version;
  UINT32      LowestSupportedVersion;
  EFI_TIME    CreatedOn;
  EFI_TIME    SavedOn;
} DFCI_INTERNAL_DATA_VAR;

#pragma pack (pop)

EFI_STATUS
EFIAPI
SMID_InitInternalData (
  IN DFCI_SETTING_INTERNAL_DATA  **InternalData
  )
{
  EFI_STATUS  Status;

  if (InternalData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *InternalData = (DFCI_SETTING_INTERNAL_DATA *)AllocateZeroPool (sizeof (DFCI_SETTING_INTERNAL_DATA));
  if (*InternalData == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to allocate memory for System Setting Internal Data\n", __FUNCTION__));
    Status = EFI_ABORTED;
    goto EXIT;
  }

  (*InternalData)->CurrentVersion = 0;
  (*InternalData)->LSV            = 0;
  (*InternalData)->Modified       = TRUE;
  Status                          = gRT->GetTime (&((*InternalData)->CreatedOn), NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to get time %r\n", __FUNCTION__, Status));
    // Leave time zeroed by allocate zero pool
  }

  Status = EFI_SUCCESS;

EXIT:
  return Status;
}

EFI_STATUS
EFIAPI
SMID_TransitionInternalVariableData (
  IN OUT DFCI_INTERNAL_DATA_VAR  **VarPtr,
  IN OUT UINTN                   *VarSize
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  if ((VarPtr == NULL) || (*VarPtr == NULL) || (VarSize == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  // Basically get version and transition
  // Need to free the original var after done copying
  // Need to update the varsize to match new size

  // At the moment we don't have more than 1 version so this should never happen
  DEBUG ((DEBUG_ERROR, "%a - Unsupported Version.  No conversion method set. 0x%X\n", __FUNCTION__, (*VarPtr)->Version));
  Status = EFI_UNSUPPORTED;

Exit:
  ASSERT_EFI_ERROR (Status);
  return Status;
}

EFI_STATUS
EFIAPI
SMID_LoadFromFlash (
  IN DFCI_SETTING_INTERNAL_DATA  **InternalData
  )
{
  EFI_STATUS              Status;
  DFCI_INTERNAL_DATA_VAR  *Var          = NULL;
  UINTN                   VarSize       = 0;
  UINT32                  VarAttributes = 0;

  if (InternalData == NULL) {
    ASSERT (InternalData != NULL);
    return EFI_INVALID_PARAMETER;
  }

  // 1. Load Variable
  Status = GetVariable3 (VAR_NAME, &gDfciInternalVariableGuid, (VOID **)&Var, &VarSize, &VarAttributes);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      DEBUG ((DEBUG_INFO, "%a - Var not found.  1st boot after flash?\n", __FUNCTION__));
    } else {
      DEBUG ((DEBUG_ERROR, "%a - Error getting variable %r\n", __FUNCTION__, Status));
      ASSERT_EFI_ERROR (Status);
    }

    return Status;
  }

  // Check the size
  if (VarSize < MIN_VAR_SIZE) {
    DEBUG ((DEBUG_INFO, "%a - Var less than min size. 0x%X\n", __FUNCTION__, VarSize));
    ASSERT (VarSize >= MIN_VAR_SIZE);
    Status = EFI_NOT_FOUND;
    goto EXIT;
  }

  // 2. Check attributes to make sure they are correct
  if (VarAttributes != DFCI_INTERNAL_VAR_ATTRIBUTES) {
    DEBUG ((DEBUG_INFO, "%a - Var Attributes wrong. 0x%X\n", __FUNCTION__, VarAttributes));
    ASSERT (VarAttributes == DFCI_INTERNAL_VAR_ATTRIBUTES);
    Status = EFI_NOT_FOUND;
    goto EXIT;
  }

  // 3. Check out variable to make sure it is valid
  if (Var->HeaderSignature != VAR_HEADER_SIG) {
    DEBUG ((DEBUG_INFO, "%a - Var Header Signature wrong.\n", __FUNCTION__));
    Status = EFI_COMPROMISED_DATA;
    goto EXIT;
  }

  // Check version to see if we need to transition
  if (Var->HeaderVersion != VAR_VERSION) {
    Status = SMID_TransitionInternalVariableData (&Var, &VarSize);
    // transition the Variable structure to new variable structure
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Var Transition failed.  Status = %r\n", __FUNCTION__, Status));
      goto EXIT;
    }
  }

  // Check again to make sure it was transitioned
  if (Var->HeaderVersion != VAR_VERSION) {
    // Double check.  Should have been transitioned if necessary
    DEBUG ((DEBUG_ERROR, "%a - Var wrong version.  Version = 0x%X\n", __FUNCTION__, Var->Version));
    goto EXIT;
  }

  // size should be correct now
  if (VarSize != sizeof (DFCI_INTERNAL_DATA_VAR)) {
    DEBUG ((DEBUG_INFO, "%a - Var Header Version Wrong %d.\n", __FUNCTION__, Var->HeaderVersion));
    Status = EFI_COMPROMISED_DATA;
    goto EXIT;
  }

  // Make sure version is not below lowest version
  if (Var->Version < Var->LowestSupportedVersion) {
    DEBUG ((DEBUG_ERROR, "%a - Version (0x%X) < LowestSupportedVersion (0x%X)\n", __FUNCTION__, Var->Version, Var->LowestSupportedVersion));
    ASSERT (Var->Version >= Var->LowestSupportedVersion);
    Status = EFI_COMPROMISED_DATA;
    goto EXIT;
  }

  DEBUG ((DEBUG_INFO, "%a - Loaded valid variable\n", __FUNCTION__));

  // 4. Process variable to load it into Internal Data Struct
  (*InternalData) = AllocatePool (sizeof (DFCI_SETTING_INTERNAL_DATA));
  if (*InternalData == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to allocate memory for Internal Data\n", __FUNCTION__));
    Status = EFI_ABORTED;
    goto EXIT;
  }

  //
  // Set the data members of the Internal Data Store
  //
  (*InternalData)->CurrentVersion = Var->Version;
  (*InternalData)->LSV            = Var->LowestSupportedVersion;
  CopyMem (&((*InternalData)->CreatedOn), &(Var->CreatedOn), sizeof (Var->CreatedOn));
  (*InternalData)->Modified = FALSE;

  DEBUG ((DEBUG_INFO, "%a - Loaded from flash successfully.\n", __FUNCTION__));
  Status = EFI_SUCCESS;

EXIT:
  if (Var != NULL) {
    FreePool (Var);
  }

  //
  // If in error condition make sure we freed the Internal data
  //
  if (EFI_ERROR (Status)) {
    if (*InternalData != NULL) {
      FreePool (*InternalData);
      (*InternalData) = NULL;
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
SMID_SaveToFlash (
  IN DFCI_SETTING_INTERNAL_DATA  *InternalData
  )
{
  EFI_STATUS              Status;
  DFCI_INTERNAL_DATA_VAR  *Var    = NULL;
  UINTN                   VarSize = 0;
  EFI_TIME                t;

  if (InternalData == NULL) {
    ASSERT (InternalData != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (!(InternalData->Modified)) {
    DEBUG ((DEBUG_INFO, "%a - Not Modified.  No action needed.\n", __FUNCTION__));
    return EFI_SUCCESS;
  }

  VarSize = sizeof (DFCI_INTERNAL_DATA_VAR);
  Var     = (DFCI_INTERNAL_DATA_VAR *)AllocateZeroPool (VarSize);
  if (Var == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to allocate memory for Variable\n", __FUNCTION__));
    Status = EFI_ABORTED;
    goto EXIT;
  }

  Var->HeaderSignature        = VAR_HEADER_SIG;
  Var->HeaderVersion          = VAR_VERSION;
  Var->Version                = InternalData->CurrentVersion;
  Var->LowestSupportedVersion = InternalData->LSV;
  CopyMem (&(Var->CreatedOn), &(InternalData->CreatedOn), sizeof (InternalData->CreatedOn));

  // Handle saved time
  Status = gRT->GetTime (&(t), NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to get time %r\n", __FUNCTION__, Status));
    // Leave time zeroed by allocate zero pool
  }

  CopyMem (&(Var->SavedOn), &t, sizeof (Var->SavedOn));

  Status = gRT->SetVariable (VAR_NAME, &gDfciInternalVariableGuid, DFCI_INTERNAL_VAR_ATTRIBUTES, VarSize, Var);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - failed to save variable.  Status %r\n", __FUNCTION__, Status));
  } else {
    DEBUG ((DEBUG_INFO, "%a - Saved to flash successfully.\n", __FUNCTION__));
    InternalData->Modified = FALSE;
  }

EXIT:
  if (Var != NULL) {
    FreePool (Var);
  }

  return Status;
}

EFI_STATUS
EFIAPI
SMID_ResetInFlash (
  )
{
  EFI_STATUS  Status;

  Status = gRT->SetVariable (VAR_NAME, &gDfciInternalVariableGuid, DFCI_INTERNAL_VAR_ATTRIBUTES, 0, NULL);
  if (Status == EFI_NOT_FOUND) {
    // Special case for not found.  If var doesn't exist then our job has been done successfully.
    Status = EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - failed to Reset the internal data variable.  Status %r\n", __FUNCTION__, Status));
  }

  return Status;
}
