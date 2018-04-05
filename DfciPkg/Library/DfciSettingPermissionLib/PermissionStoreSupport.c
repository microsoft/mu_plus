/** @file
Permission Store Support

Routines that support working with Permission Store Object

Copyright (c) 2015, Microsoft Corporation. 

**/
#include "DfciSettingPermission.h"

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
  (*Store)->Default = DFCI_PERMISSION_MASK__DEFAULT;  //Set to default which is local user
  InitializeListHead( &((*Store)->PermissionsListHead));
  
  Status = gRT->GetTime(&((*Store)->CreatedOn), NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get time %r\n", __FUNCTION__, Status));
    //Leave time zeroed by allocate zero pool
  }
  //SavedOn will be zero until it is saved.
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
IN DFCI_PERMISSION_MASK Perm)
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

  Temp->Perm = Perm;
  InsertTailList(&(Store->PermissionsListHead), &(Temp->Link));
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
IN DFCI_SETTING_ID_STRING Id)
{
  UINTN     IdSize;
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
  DEBUG((DEBUG_INFO, " Default Permission: 0x%X\n", Store->Default));
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
    DEBUG((DEBUG_INFO, "   PERM ENTRY - Id: %a  Permission: 0x%X\n", Temp->Id, Temp->Perm));
  }
  DEBUG((DEBUG_INFO, "---------- END PRINTING DFCI_PERMISSION_STORE ---------\n\n"));
}