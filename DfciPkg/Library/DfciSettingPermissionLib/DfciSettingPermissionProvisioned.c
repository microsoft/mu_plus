/**@file
DfciSettingPermissionProvisioned.c

This file supports loading internal data (previously provisioned) from flash so that
SettingPermission code can use it.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DfciSettingPermission.h"


//Define the local structure for the variable (this is for internal use only)
// Variable name-space - Use gEfiCallerIdGuid since this is internal only

#define VAR_NAME  L"_SPP"
#define VAR_HEADER_SIG SIGNATURE_32('S', 'B', 'C', 'Z')
#define VAR_VERSION_V1 (1)
#define VAR_VERSION_V2 (2)
#define VAR_VERSION_V3 (3)
#define VAR_VERSION_V4 (4)
#define MAX_SIZE_FOR_VAR (1024 * 2)

#pragma pack (push, 1)

typedef struct {
  DFCI_SETTING_ID_V1_ENUM Id;     // DFCI_SETTING_ID_ENUM is no longer
  DFCI_PERMISSION_MASK    Permissions;
} DFCI_PERM_INTERNAL_TABLE_ENTRY_V1;

typedef struct {
  DFCI_PACKET_SIGNATURE Header;         // 'S', 'B', 'C', 'Z'
                                     // Header Version = 1
  UINT32 Version;
  UINT32 LowestSupportedVersion;
  EFI_TIME CreatedOn;
  EFI_TIME SavedOn;
  DFCI_PERMISSION_MASK DefaultPMask; //For any ID not found in the entry list this is the permission
  UINT16 NumberOfEntries;     // Number of entries in the PermTable
  DFCI_PERM_INTERNAL_TABLE_ENTRY_V1 PermTable[];
} DFCI_PERM_INTERNAL_PROVISONED_VAR_V1;

typedef struct {
  DFCI_PERMISSION_MASK Permissions;
  UINT8                IdSize;          // Setting ID's have a max length of 97
  CHAR8                Id[];            // Must be variable in length;
} DFCI_PERM_INTERNAL_TABLE_ENTRY_V2;

typedef struct {
  DFCI_PACKET_SIGNATURE  Header;    // 'S', 'B', 'C', 'Z'
                                    //  Version = 2
  UINT32 Version;
  UINT32 LowestSupportedVersion;
  EFI_TIME CreatedOn;
  EFI_TIME SavedOn;
  DFCI_PERMISSION_MASK DefaultPMask; //For any ID not found in the entry list this is the permission
  UINT16 NumberOfEntries;     // Number of entries in the PermTable
  DFCI_PERM_INTERNAL_TABLE_ENTRY_V2 PermTableStart; // First variable length entry
} DFCI_PERM_INTERNAL_PROVISONED_VAR_V2;

typedef struct {
  DFCI_PERMISSION_MASK Permissions;
  DFCI_PERMISSION_MASK DMask;
  UINT8                IdSize;          // Setting ID's have a max length of 97
  CHAR8                Id[];            // Must be variable in length;
} DFCI_PERM_INTERNAL_TABLE_ENTRY;

typedef struct {
  DFCI_PACKET_SIGNATURE  Header;    // 'S', 'B', 'C', 'Z'
                                    //  Version = 3 and Version = 4
  UINT32 Version;
  UINT32 LowestSupportedVersion;
  EFI_TIME CreatedOn;
  EFI_TIME SavedOn;
  DFCI_PERMISSION_MASK DefaultPMask; //For any ID not found in the entry list this is the permission
  DFCI_PERMISSION_MASK DefaultDMask; //For any ID not found in the entry list this is the delegated permission
  UINT16 NumberOfEntries;     // Number of entries in the PermTable
  DFCI_PERM_INTERNAL_TABLE_ENTRY PermTableStart; // First variable length entry
} DFCI_PERM_INTERNAL_PROVISONED_VAR;

#pragma pack (pop)

/**
 * ConvertV1ToLatest
 *
 * Convert the ENUM setting ID's to String settings ID's
 *
 **/
EFI_STATUS
ConvertFromV1 (IN DFCI_PERMISSION_STORE            **Store,
               IN DFCI_PERM_INTERNAL_PROVISONED_VAR *Var,
               UINTN                                 VarSize) {

  EFI_STATUS Status;
  DFCI_PERM_INTERNAL_PROVISONED_VAR_V1 *Var1;
  UINTN ComputedSize;
  DFCI_SETTING_ID_STRING Id;


  Var1 = (DFCI_PERM_INTERNAL_PROVISONED_VAR_V1 *) Var;
  ComputedSize = sizeof(DFCI_PERM_INTERNAL_PROVISONED_VAR_V1) + (Var1->NumberOfEntries * sizeof(DFCI_PERM_INTERNAL_TABLE_ENTRY_V1));
  if (VarSize != ComputedSize)
  {
    DEBUG((DEBUG_ERROR, "%a - VarSize (0x%X) != ComputedSize (0x%X)\n", __FUNCTION__, VarSize, ComputedSize));
    ASSERT(VarSize == ComputedSize);
    return EFI_COMPROMISED_DATA;
  }
  for (UINT16 i = 0; i < Var1->NumberOfEntries; i++)
  {
    Id = DfciV1TranslateEnum(Var1->PermTable[i].Id);
    Status = AddPermissionEntry(*Store, Id, Var1->PermTable[i].Permissions, DFCI_IDENTITY_SIGNER_OWNER);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to add a permission entry. %r\n", __FUNCTION__, Status));
      ASSERT_EFI_ERROR(Status);
      //continue anyway and hope for the best :)
    }
  }
  if ((*Store)->DefaultPMask == DFCI_IDENTITY_LOCAL)
  {
    (*Store)->DefaultPMask = DFCI_PERMISSION_MASK__DEFAULT;
  }
  (*Store)->DefaultDMask = DFCI_PERMISSION_MASK__DELEGATED_DEFAULT;

  (*Store)->Modified = TRUE;
  SaveToFlash(*Store);  // Complete the translation from V1 to latest version
  DEBUG((DEBUG_INFO, "%a - Permission store converted to V%d.\n", __FUNCTION__, VAR_VERSION_V4));

  return EFI_SUCCESS;
}

/**
 * ConvertFromV2
 *
 * Initialize a DefaultDMASK, and update each of the Permission Entries with an default DMask
 *
 **/
EFI_STATUS
ConvertFromV2 (IN DFCI_PERMISSION_STORE            **Store,
               IN DFCI_PERM_INTERNAL_PROVISONED_VAR *Var,
               UINTN                                 VarSize) {

  EFI_STATUS Status;
  DFCI_PERM_INTERNAL_TABLE_ENTRY_V2    *PermEntry;
  DFCI_PERM_INTERNAL_PROVISONED_VAR_V2 *Var2;
  UINTN                                 Count;
  CHAR8                                *PermPtr;
  CHAR8                                *EndPtr;


  Var2 = (DFCI_PERM_INTERNAL_PROVISONED_VAR_V2 *) Var;
  PermPtr = (CHAR8 *) &Var2->PermTableStart;
  EndPtr = (CHAR8 *) Var2;
  EndPtr += VarSize;
  Count = 0;
  for (UINT16 i = 0; (i < Var2->NumberOfEntries) && (PermPtr < EndPtr); i++)
  {
    Count++;
    PermEntry = (DFCI_PERM_INTERNAL_TABLE_ENTRY_V2 *) PermPtr;
    PermPtr += (sizeof(DFCI_PERM_INTERNAL_TABLE_ENTRY_V2) + PermEntry->IdSize);

    if (PermPtr > EndPtr)
    {
      DEBUG((DEBUG_ERROR, "%a - Poor calculation for variable size.\n", __FUNCTION__));
      ASSERT(FALSE);
      return EFI_COMPROMISED_DATA;
    }
    Status = AddPermissionEntry(*Store, (DFCI_SETTING_ID_STRING)PermEntry->Id, PermEntry->Permissions, DFCI_IDENTITY_SIGNER_OWNER);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to add a permission entry for %a. %r\n", __FUNCTION__, PermEntry->Id, Status));
      ASSERT_EFI_ERROR(Status);
      //continue anyway and hope for the best :)
    }
  }
  if (Count != Var2->NumberOfEntries)
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to process all permission entries. %r\n", __FUNCTION__, Status));
    Status = EFI_COMPROMISED_DATA;
    ASSERT_EFI_ERROR(Status);
    return Status;
  }
  if ((*Store)->DefaultPMask == DFCI_IDENTITY_LOCAL)
  {
    (*Store)->DefaultPMask |= DFCI_PERMISSION_MASK__DEFAULT ;
  }
  (*Store)->DefaultDMask = DFCI_PERMISSION_MASK__DELEGATED_DEFAULT;
  (*Store)->Modified = TRUE;
  SaveToFlash(*Store);  // Complete the translation from V2 to latest version
  DEBUG((DEBUG_INFO, "%a - Permission store converted to V%d.\n", __FUNCTION__, VAR_VERSION_V4));
  return EFI_SUCCESS;
}

/**
 * ConvertV3toLatest
 *
 * For a V3 store, DefaultDMask may be 0.  Correct this by allowing any DFCI user DMask permission.
 **/
EFI_STATUS
ConvertFromV3 (IN DFCI_PERMISSION_STORE **Store) {

  //  Due to a bug where DefaultDMask was not saved, allow all users to have DMask permission for
  //  a version 3 var store.  When the var store changes to V4, this work around will no longer apply.
  //  In the normal case, the next permission packet will set the proper DMask.
  if ((*Store)->DefaultDMask == 0) {               // Don't allow a 0 to propagate into the permission store
    (*Store)->DefaultDMask = DFCI_PERMISSION_MASK__USERS;
  }
  (*Store)->Modified = TRUE;
  SaveToFlash(*Store);                      //  Store the updated DefaultDMask, and update version to latest
  DEBUG((DEBUG_INFO, "%a - Permission store converted to V%d.\n", __FUNCTION__, VAR_VERSION_V4));
  return EFI_SUCCESS;
}

/**
 * LoadCurrentFromFlash
 *
 * Load the current version store (V3 or V4) into memory
 *
 **/
EFI_STATUS
LoadCurrentFromFlash (IN DFCI_PERMISSION_STORE            **Store,
                      IN DFCI_PERM_INTERNAL_PROVISONED_VAR *Var,
                      IN UINTN                              VarSize) {

  EFI_STATUS Status;
  DFCI_PERM_INTERNAL_TABLE_ENTRY *PermEntry;
  UINTN                           Count;
  CHAR8                          *PermPtr;
  CHAR8                          *EndPtr;

  PermPtr = (CHAR8 *) &Var->PermTableStart;
  EndPtr = (CHAR8 *) Var;
  EndPtr += VarSize;
  Count = 0;
  for (UINT16 i = 0; (i < Var->NumberOfEntries) && (PermPtr < EndPtr); i++)
  {
    Count++;
    PermEntry = (DFCI_PERM_INTERNAL_TABLE_ENTRY *) PermPtr;
    PermPtr += (sizeof(DFCI_PERM_INTERNAL_TABLE_ENTRY) + PermEntry->IdSize);

    if (PermPtr > EndPtr)
    {
      DEBUG((DEBUG_ERROR, "%a - Poor calculation for variable size.\n", __FUNCTION__));
      ASSERT(FALSE);
      return EFI_COMPROMISED_DATA;
    }
    Status = AddPermissionEntry(*Store, (DFCI_SETTING_ID_STRING)PermEntry->Id, PermEntry->Permissions, PermEntry->DMask);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to add a permission entry for %a. %r\n", __FUNCTION__, PermEntry->Id, Status));
      ASSERT_EFI_ERROR(Status);
      //continue anyway and hope for the best :)
    }
  }
  if (Count != Var->NumberOfEntries)
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to process all permission entries. %r\n", __FUNCTION__, Status));
    Status = EFI_COMPROMISED_DATA;
    ASSERT_EFI_ERROR(Status);
    return Status;;
  }

  (*Store)->DefaultDMask = Var->DefaultDMask;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LoadFromFlash(IN DFCI_PERMISSION_STORE **Store)
{
  EFI_STATUS Status;
  DFCI_PERM_INTERNAL_PROVISONED_VAR    *Var = NULL;
  UINTN VarSize = 0;
  UINTN ComputedSize = 0;
  UINT32 VarAttributes = 0;

  if (Store == NULL)
  {
    ASSERT(Store != NULL);
    return EFI_INVALID_PARAMETER;
  }

  *Store = NULL;

  //1. Load Variable
  Status = GetVariable3(VAR_NAME, &gDfciInternalVariableGuid, (VOID **) &Var, &VarSize, &VarAttributes);
  if (EFI_ERROR(Status))
  {
    if (Status == EFI_NOT_FOUND)
    {
      DEBUG((DEBUG_INFO, "%a - Var not found.  1st boot after flash?\n", __FUNCTION__));
    }
    else
    {
      DEBUG((DEBUG_ERROR, "%a - Error getting variable %r\n", __FUNCTION__, Status));
      ASSERT_EFI_ERROR(Status);
    }
    return Status;
  }

  //Check the size
  if (VarSize > MAX_SIZE_FOR_VAR)
  {
    DEBUG((DEBUG_INFO, "%a - Var too big. 0x%X\n", __FUNCTION__, VarSize));
    ASSERT(VarSize <= MAX_SIZE_FOR_VAR);
    Status = EFI_NOT_FOUND;
    goto EXIT;
  }

  //2. Check attributes to make sure they are correct
  if (VarAttributes != DFCI_INTERNAL_VAR_ATTRIBUTES)
  {
    DEBUG((DEBUG_INFO, "%a - Var Attributes wrong. 0x%X\n", __FUNCTION__, VarAttributes));
    ASSERT(VarAttributes == DFCI_INTERNAL_VAR_ATTRIBUTES);
    Status = EFI_NOT_FOUND;
    goto EXIT;
  }

  //3. Check out variable to make sure it is valid
  if (Var->Header.Hdr.Signature != VAR_HEADER_SIG)
  {
    DEBUG((DEBUG_INFO, "%a - Var Header Signature wrong.\n", __FUNCTION__));
    Status = EFI_COMPROMISED_DATA;
    goto EXIT;
  }

  if (Var->Version < Var->LowestSupportedVersion)
  {
    DEBUG((DEBUG_ERROR, "%a - Version (0x%X) < LowestSupportedVersion (0x%X)\n", __FUNCTION__, Var->Version, Var->LowestSupportedVersion));
    ASSERT(Var->Version >= Var->LowestSupportedVersion);
    Status = EFI_COMPROMISED_DATA;
    goto EXIT;
  }

  // Allocate new permission store;
  ComputedSize = sizeof(DFCI_PERMISSION_STORE);
  *Store = (DFCI_PERMISSION_STORE *) AllocateZeroPool(sizeof(DFCI_PERMISSION_STORE));
  if (*Store == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to allocate memory for Store\n", __FUNCTION__));
    Status= EFI_ABORTED;
    goto EXIT;
  }

  //
  // All of the versions of the Permission Store variable have the same layout from Header through
  // DefaultPMask.
  //

  (*Store)->Modified = FALSE;  //since flash matches store modified is false
  (*Store)->Version = Var->Version;
  (*Store)->Lsv = Var->LowestSupportedVersion;
  (*Store)->CreatedOn = Var->CreatedOn;
  (*Store)->SavedOn = Var->SavedOn;
  (*Store)->DefaultPMask = Var->DefaultPMask;
  (*Store)->DefaultDMask = DFCI_PERMISSION_MASK__DELEGATED_DEFAULT;

  InitializeListHead(&((*Store)->PermissionsListHead));

  switch (Var->Header.Version)
  {

    //
    // Original version, where settings are an ENUM.  Have to translate the setting id ENUM to a setting id string.
    //
    case VAR_VERSION_V1:
      Status = ConvertFromV1 (Store, Var, VarSize);
      break;

    //
    // Second version.  Here we need to add the default DMask to the Permission store.
    //
    case VAR_VERSION_V2:
      Status = ConvertFromV2 (Store, Var, VarSize);
      break;

    //
    // Third version.  Here, the DefaultDMask may be zero and needs to be corrected.  The format is
    // the same as the current version, but just needs to be fixed up.
    //
    case VAR_VERSION_V3:
      Status = LoadCurrentFromFlash (Store, Var, VarSize);
      if (!EFI_ERROR(Status)) {
        Status = ConvertFromV3 (Store);
      }
      break;

    // Current Version
    case VAR_VERSION_V4:
      Status = LoadCurrentFromFlash (Store, Var, VarSize);
      break;

    default:
      DEBUG((DEBUG_INFO, "%a - Var Header Version %d not supported.\n", __FUNCTION__, Var->Header.Version));
      Status = EFI_COMPROMISED_DATA;
      break;
  }

  if (!EFI_ERROR(Status)) {
    DEBUG((DEBUG_INFO, "%a - Loaded valid variable. Version %d.  Code=%r\n", __FUNCTION__, VAR_VERSION_V4, Status));
  }


EXIT:
  if (Var != NULL)
  {
    FreePool(Var);
  }
  if (EFI_ERROR(Status) && (*Store != NULL))
  {
    FreePool (*Store);
    *Store = NULL;
  }
  return Status;
}

/**
 * SaveToFlash
 *
 *     Always stores the variable in the latest format.
 *
 **/
EFI_STATUS
EFIAPI
SaveToFlash(IN DFCI_PERMISSION_STORE *Store)
{
  EFI_STATUS Status;
  DFCI_PERM_INTERNAL_PROVISONED_VAR *Var = NULL;
  UINTN VarSize = 0;
  UINTN NumEntries = 0;
  EFI_TIME t;
  UINTN                           TotalIdSize;
  DFCI_PERM_INTERNAL_TABLE_ENTRY *PermEntry;
  CHAR8                          *PermPtr;

  if (Store == NULL)
  {
    ASSERT(Store != NULL);
    return EFI_INVALID_PARAMETER;
  }
  if (!Store->Modified)
  {
    DEBUG((DEBUG_INFO, "%a - Not Modified.  No action needed.\n", __FUNCTION__));
    return EFI_SUCCESS;
  }
  NumEntries = GetNumberOfPermissionEntires(Store, &TotalIdSize);

  //Figure out our size
  VarSize = sizeof(DFCI_PERM_INTERNAL_PROVISONED_VAR) + ((NumEntries -1) * sizeof(DFCI_PERM_INTERNAL_TABLE_ENTRY) + TotalIdSize);
  //Check the size
  if (VarSize > MAX_SIZE_FOR_VAR)
  {
    DEBUG((DEBUG_INFO, "%a - Var too big. 0x%X\n", __FUNCTION__, VarSize));
    ASSERT(VarSize <= MAX_SIZE_FOR_VAR);
    return EFI_INVALID_PARAMETER;
  }

  Var = (DFCI_PERM_INTERNAL_PROVISONED_VAR *)AllocateZeroPool(VarSize);
  if (Var == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to allocate memory for Store\n", __FUNCTION__));
    Status = EFI_ABORTED;
    goto EXIT;
  }
  Var->Header.Hdr.Signature = VAR_HEADER_SIG;
  Var->Header.Version = VAR_VERSION_V4;
  Var->Version = Store->Version;
  Var->LowestSupportedVersion = Store->Lsv;
  Var->DefaultPMask = Store->DefaultPMask;
  Var->DefaultDMask = Store->DefaultDMask;
  CopyMem(&(Var->CreatedOn), &(Store->CreatedOn), sizeof(Var->CreatedOn));

  PermEntry = &Var->PermTableStart;
  PermPtr = (CHAR8 *) PermEntry;

  Var->NumberOfEntries = (UINT16)NumEntries;  //can't exceed UINT16 size because of var size requirements and previous checks
  for (LIST_ENTRY *Link = Store->PermissionsListHead.ForwardLink; Link != &(Store->PermissionsListHead); Link = Link->ForwardLink)
  {
    DFCI_PERMISSION_ENTRY *Temp = CR(Link, DFCI_PERMISSION_ENTRY, Link, DFCI_PERMISSION_LIST_ENTRY_SIGNATURE);

    PermEntry->Permissions = Temp->PMask;
    PermEntry->DMask = Temp->DMask;
    PermEntry->IdSize = Temp->IdSize;
    CopyMem (PermEntry->Id, Temp->Id, Temp->IdSize);
    PermPtr += (sizeof(DFCI_PERM_INTERNAL_TABLE_ENTRY) + Temp->IdSize);
    PermEntry = (DFCI_PERM_INTERNAL_TABLE_ENTRY *) PermPtr;
  }

  Status = gRT->GetTime(&(t), NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get time %r\n", __FUNCTION__, Status));
    //Leave time zeroed by allocate zero pool
  }
  CopyMem(&(Var->SavedOn), &t, sizeof(Var->SavedOn));

  Status = gRT->SetVariable(VAR_NAME, &gDfciInternalVariableGuid, DFCI_INTERNAL_VAR_ATTRIBUTES, VarSize, Var);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - failed to save variable.  Status %r\n", __FUNCTION__, Status));
  }
  else
  {
    DEBUG((DEBUG_INFO, "%a - Saved to flash successfully.\n", __FUNCTION__));
    CopyMem(&(Store->SavedOn), &(Var->SavedOn), sizeof(Store->SavedOn));
    Store->Modified = FALSE;
  }

EXIT:
  if (Var != NULL)
  {
    FreePool(Var);
  }
  return Status;
}
