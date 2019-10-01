/**@file
PermissionStoreSupport.c

Routines that support working with Permission Store Object

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DfciSettingPermission.h"

EFI_STATUS
EFIAPI
AddRequiredPermissionEntry (
    IN DFCI_PERMISSION_STORE *Store,
    IN DFCI_SETTING_ID_STRING Id,
    IN DFCI_PERMISSION_MASK   PMask,
    IN DFCI_PERMISSION_MASK   DMask
) {
  DFCI_PERMISSION_ENTRY *Entry;
  EFI_STATUS             Status = EFI_SUCCESS;

  Entry = FindPermissionEntry (Store, Id);

  if (Entry == NULL)
  {
    Status = AddPermissionEntry(Store, Id, PMask, DMask);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed adding %a. Code = %r\n", __FUNCTION__, Status));
    }
  } else {
    Entry->PMask = PMask;
    Entry->DMask = DMask;
  }
  return Status;
}

EFI_STATUS
AddRequiredPermissions (
    IN DFCI_PERMISSION_STORE *Store
  ) {

  AddRequiredPermissionEntry (Store, DFCI_SETTING_ID__OWNER_KEY,    DFCI_IDENTITY_LOCAL |
                                                                    DFCI_IDENTITY_SIGNER_ZTD, DFCI_IDENTITY_SIGNER_OWNER);
  AddRequiredPermissionEntry (Store, DFCI_SETTING_ID__ZTD_KEY,      DFCI_IDENTITY_LOCAL,      DFCI_PERMISSION_MASK__NONE);
  AddRequiredPermissionEntry (Store, DFCI_SETTING_ID__ZTD_UNENROLL, DFCI_IDENTITY_INVALID,    DFCI_PERMISSION_MASK__NONE);
  AddRequiredPermissionEntry (Store, DFCI_SETTING_ID__ZTD_RECOVERY, DFCI_IDENTITY_INVALID,   DFCI_PERMISSION_MASK__NONE);

  return EFI_SUCCESS;
}


/**
  Initialize a Permission Store to the defaults
**/
EFI_STATUS
EFIAPI
InitPermStore(DFCI_PERMISSION_STORE **Store)
{
  EFI_STATUS Status;

  if (Store == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  *Store = (DFCI_PERMISSION_STORE *)AllocateZeroPool(sizeof(DFCI_PERMISSION_STORE));
  if (*Store == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to allocate memory for Store\n", __FUNCTION__));
    Status = EFI_ABORTED;
    goto EXIT;
  }

  (*Store)->Version = 0;
  (*Store)->Lsv = 0;
  (*Store)->Modified = TRUE;
  (*Store)->DefaultPMask = DFCI_PERMISSION_MASK__DEFAULT;              //Set to default which is local user
  (*Store)->DefaultDMask = DFCI_PERMISSION_MASK__DELEGATED_DEFAULT;  //Set to default which is SignerOwner
  InitializeListHead( &((*Store)->PermissionsListHead));

  Status = gRT->GetTime(&((*Store)->CreatedOn), NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get time %r\n", __FUNCTION__, Status));
    //Leave time zeroed by allocate zero pool
  }
  //SavedOn will be zero until it is saved.

  Status = AddRequiredPermissions (*Store);

  Status = EFI_SUCCESS;

EXIT:
  return Status;
}


/**
  Free all the linked list entries in the Permission Store and then
  Free the store.
**/
VOID
EFIAPI
FreePermissionStore(IN DFCI_PERMISSION_STORE *Store)
{
  DFCI_PERMISSION_ENTRY *Temp = NULL;
  LIST_ENTRY *Link = NULL;

  if (Store == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a Can't free a NULL store\n", __FUNCTION__));
    return;
  }

  Link = Store->PermissionsListHead.ForwardLink;

  //look thru the permissions and free each Entry
  while (Link != &(Store->PermissionsListHead))
  {
    Temp = CR(Link, DFCI_PERMISSION_ENTRY, Link, DFCI_PERMISSION_LIST_ENTRY_SIGNATURE);
    Link = RemoveEntryList(Link);
    FreePool(Temp);
    Temp = NULL;
  }
  FreePool(Store);
}


/**
  Get the number of permission entires in the linked list
**/
UINTN
EFIAPI
GetNumberOfPermissionEntires(IN CONST DFCI_PERMISSION_STORE *Store, OPTIONAL OUT UINTN *TotalIdSize)
{
  UINTN Count = 0;
  DFCI_PERMISSION_ENTRY *Temp = NULL;

  if (Store == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a Can't get number of entires from NULL Store\n", __FUNCTION__));
    ASSERT(Store != NULL);
    return Count;
  }
  if (TotalIdSize != NULL)
  {
    *TotalIdSize = 0;
  }

  for (LIST_ENTRY *Link = Store->PermissionsListHead.ForwardLink; Link != &(Store->PermissionsListHead); Link = Link->ForwardLink)
  {
    Count++;
    if (TotalIdSize != NULL)
    {
      Temp = CR(Link, DFCI_PERMISSION_ENTRY, Link, DFCI_PERMISSION_LIST_ENTRY_SIGNATURE);
      *TotalIdSize += Temp->IdSize;
    }
  }
  DEBUG((DEBUG_VERBOSE, "%a - %d Permission Entries in Store.\n", __FUNCTION__, Count));
  return Count;
}

/**
  Add a new Permission entry to the list at the end.

  This doesn't check if an existing entry already exists.  Caller should make sure
  entry doesn't already exist.
**/
EFI_STATUS
EFIAPI
AddPermissionEntry(
IN DFCI_PERMISSION_STORE *Store,
IN DFCI_SETTING_ID_STRING Id,
IN DFCI_PERMISSION_MASK PMask,
IN DFCI_PERMISSION_MASK DMask)
{
  DFCI_PERMISSION_ENTRY* Temp = NULL;
  UINTN                  IdSize;
  if (Store == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - NULL Store pointer\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  if (Id == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - NULL Id pointer\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  if ((Id[0] >= '0') && (Id[0] <= '9'))
  {
    // Number didnt' get converted.  That means this is an unsupported item.
    DEBUG((DEBUG_ERROR, "%a - %a is an invalid permission.\n", __FUNCTION__, Id));
    return EFI_SUCCESS;
  }

  IdSize = AsciiStrnSizeS(Id, DFCI_MAX_ID_SIZE);
  if ((IdSize < 1) || (IdSize > DFCI_MAX_ID_SIZE))
  {
    DEBUG((DEBUG_ERROR, "%a - Invalid ID length %d for %a\n", __FUNCTION__, IdSize, Id));
    return EFI_INVALID_PARAMETER;
  }

  Temp = AllocatePool(sizeof(DFCI_PERMISSION_ENTRY) + IdSize);
  if (Temp == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to allocate Memory for entry\n", __FUNCTION__));
    return EFI_ABORTED;
  }

  Temp->Signature = DFCI_PERMISSION_LIST_ENTRY_SIGNATURE;
  Temp->Id = (DFCI_SETTING_ID_STRING) &Temp->IdStore;
  Temp->IdSize = (UINT8) IdSize;    // Validated as < 255

  CopyMem (Temp->IdStore, Id, IdSize);

  Temp->PMask = PMask;
  Temp->DMask = DMask;
  InsertTailList(&(Store->PermissionsListHead), &(Temp->Link));
  return EFI_SUCCESS;
}

EFI_STATUS
MarkPermissionEntriesForDeletion (
IN DFCI_PERMISSION_STORE *Store,
IN DFCI_IDENTITY_ID       Id
) {
  LIST_ENTRY *Link = NULL;

  if (Store == NULL)
  {
    DEBUG((DEBUG_ERROR,"%a - Invalid Perm Store\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG((DEBUG_INFO,"%a - Marking permission entries owned by %x\n", __FUNCTION__, Id));
  EFI_LIST_FOR_EACH(Link, &(Store->PermissionsListHead))
  {
      DFCI_PERMISSION_ENTRY *Temp = CR(Link, DFCI_PERMISSION_ENTRY, Link, DFCI_PERMISSION_LIST_ENTRY_SIGNATURE);
      if ((Temp->DMask & DFCI_PERMISSION_MASK__USERS) != 0)
      {
        if (Id & HIGHEST_IDENTITY(Temp->DMask))
        {
          DEBUG((DEBUG_INFO,"%a - marking perm Mask=%x, %a.\n", __FUNCTION__, Temp->DMask, Temp->Id));
          Temp->DMask |= DFCI_PERMISSION_DELETE;
        }
      }
  }
  return EFI_SUCCESS;
}

EFI_STATUS
DeleteMarkedPermissionEntries (
IN DFCI_PERMISSION_STORE *Store
) {
  LIST_ENTRY *Link = NULL;
  LIST_ENTRY *NextLink = NULL;

  if (Store == NULL)
  {
    DEBUG((DEBUG_ERROR,"%a - Invalid Perm Store\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  EFI_LIST_FOR_EACH_SAFE(Link, NextLink, &(Store->PermissionsListHead))
  {
      DFCI_PERMISSION_ENTRY *Temp = CR(Link, DFCI_PERMISSION_ENTRY, Link, DFCI_PERMISSION_LIST_ENTRY_SIGNATURE);
      if (Temp->DMask & DFCI_PERMISSION_DELETE)
      {
        DEBUG((DEBUG_INFO,"%a - deleting perm Mask=%x, Entry %a.\n", __FUNCTION__, Temp->DMask, Temp->Id));
        RemoveEntryList (Link);
        FreePool (Temp);
      }
  }
  return EFI_SUCCESS;
}

/**
 Find Permission Entry for a given Id.

 If doesn't exist return NULL
**/
DFCI_PERMISSION_ENTRY*
EFIAPI
FindPermissionEntry(
IN CONST DFCI_PERMISSION_STORE *Store,
IN DFCI_SETTING_ID_STRING       Id
)
{
  UINTN      IdSize;


  if (Store == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - NULL Store pointer\n", __FUNCTION__));
    return NULL;
  }
  if (Id == NULL)
  {
      DEBUG((DEBUG_ERROR, "%a - NULL Id pointer\n", __FUNCTION__));
      return NULL;
  }
  IdSize = AsciiStrnSizeS(Id, DFCI_MAX_ID_SIZE);
  if ((IdSize < 1) || (IdSize > DFCI_MAX_ID_SIZE))
  {
      DEBUG((DEBUG_ERROR, "%a - Invalid ID length\n", __FUNCTION__));
      return NULL;
  }

  for (LIST_ENTRY *Link = Store->PermissionsListHead.ForwardLink; Link != &(Store->PermissionsListHead); Link = Link->ForwardLink)
  {
      DFCI_PERMISSION_ENTRY *Temp = CR(Link, DFCI_PERMISSION_ENTRY, Link, DFCI_PERMISSION_LIST_ENTRY_SIGNATURE);
      if (IdSize == Temp->IdSize)
      {
          if (0 == AsciiStrnCmp(Temp->Id, Id, IdSize))
          {
              DEBUG((DEBUG_VERBOSE, "%a - Found Permission Entry\n", __FUNCTION__));
              return Temp;
          }
      }
  }

  DEBUG((DEBUG_VERBOSE, "%a - Didn't find Permission Entry\n", __FUNCTION__));
  return NULL;
}

/**
Print the current state of the Permission Store using Debug
**/
VOID
EFIAPI
DebugPrintPermissionStore(IN CONST DFCI_PERMISSION_STORE *Store)
{
  if (Store == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - NULL Store pointer\n", __FUNCTION__));
    return;
  }

  DEBUG((DEBUG_INFO, "\n---------- START PRINTING DFCI_PERMISSION_STORE ---------\n"));
  DEBUG((DEBUG_INFO, " Version: %d\n LSV: %d\n Modified: %d\n", Store->Version, Store->Lsv, Store->Modified));
  DEBUG((DEBUG_INFO, " DefaultPMask Permission: 0x%X - DefaultPMask DefaultDMask: 0x%X\n", Store->DefaultPMask, Store->DefaultDMask));
  DEBUG((DEBUG_INFO, " Created On:    %d-%02d-%02d %02d:%02d:%02d\n", Store->CreatedOn.Year, Store->CreatedOn.Month, Store->CreatedOn.Day, Store->CreatedOn.Hour, Store->CreatedOn.Minute, Store->CreatedOn.Second));
  DEBUG((DEBUG_INFO, " Last saved On: %d-%02d-%02d %02d:%02d:%02d\n", Store->SavedOn.Year, Store->SavedOn.Month, Store->SavedOn.Day, Store->SavedOn.Hour, Store->SavedOn.Minute, Store->SavedOn.Second));
  DEBUG((DEBUG_INFO, " Number Of Permission Entries: %d\n", GetNumberOfPermissionEntires(Store, NULL)));
  for (LIST_ENTRY *Link = Store->PermissionsListHead.ForwardLink; Link != &(Store->PermissionsListHead); Link = Link->ForwardLink)
  {
    //
    // THIS IS UGLY -
    // USE #define so that compiler doens't complain on RELEASE build.
    //
    #define Temp (CR(Link, DFCI_PERMISSION_ENTRY, Link, DFCI_PERMISSION_LIST_ENTRY_SIGNATURE))
    DEBUG((DEBUG_INFO, "   PERM ENTRY - Id: %a  Permission: 0x%X  DefaultDMask Permission: 0x%X\n", Temp->Id, Temp->PMask, Temp->DMask));
  }
  DEBUG((DEBUG_INFO, "---------- END PRINTING DFCI_PERMISSION_STORE ---------\n\n"));
}