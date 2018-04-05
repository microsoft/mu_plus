/** @file
Local header file to support the different implementation files for DfciSettingPermissionLib

Copyright (c) 2015, Microsoft Corporation. 

**/

#ifndef DFCI_SETTING_PERMISSION_H
#define DFCI_SETTING_PERMISSION_H

#include <PiDxe.h>
#include <DfciSystemSettingTypes.h>

#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <XmlTypes.h>
#include <Library/XmlTreeLib.h>
#include <Library/XmlTreeQueryLib.h>
#include <Library/BaseLib.h>
#include <Library/DfciSettingsLib.h>
#include <Library/DfciV1SupportLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DfciSettingPermissionLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DfciAuthentication.h>
#include <Guid/DfciInternalVariableGuid.h>

#include <Settings/DfciSettings.h>

#define DFCI_PERMISSION_LIST_ENTRY_SIGNATURE SIGNATURE_32('M','P','L','S')

typedef struct {
  UINTN Signature;
  LIST_ENTRY Link;
  DFCI_SETTING_ID_STRING Id;              // Pointer to IdStore
  DFCI_PERMISSION_MASK   Perm;
  UINT8                  IdSize;
  CHAR8                  IdStore[];
} DFCI_PERMISSION_ENTRY;


typedef struct {
  UINT32 Version;
  UINT32 Lsv;
  BOOLEAN Modified;
  EFI_TIME CreatedOn;
  EFI_TIME SavedOn;
  DFCI_PERMISSION_MASK Default;
  LIST_ENTRY PermissionsListHead;
} DFCI_PERMISSION_STORE;

extern DFCI_AUTHENTICATION_PROTOCOL *mAuthenticationProtocol; 
extern DFCI_PERMISSION_STORE *mPermStore; 


EFI_STATUS
EFIAPI
SaveToFlash(IN DFCI_PERMISSION_STORE *Store);

EFI_STATUS
EFIAPI
LoadFromFlash(IN DFCI_PERMISSION_STORE **Store);


// ----- funcs for DFCI_PERMISSION_STORE --------
/**
Initialize a Permission Store to the defaults
**/
EFI_STATUS
EFIAPI
InitPermStore(DFCI_PERMISSION_STORE **Store);

/**
Free all the linked list entries in the Permission Store and then
Free the store.
**/
VOID
EFIAPI
FreePermissionStore(IN DFCI_PERMISSION_STORE *Store);

/**
Get the number of permission entires in the linked list
**/
UINTN
EFIAPI
GetNumberOfPermissionEntires(IN CONST DFCI_PERMISSION_STORE *Store, OPTIONAL OUT UINTN *TotalIdSize);

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
IN DFCI_PERMISSION_MASK Perm);

/**
Find Permission Entry for a given Id.

If doesn't exist return NULL
**/
DFCI_PERMISSION_ENTRY*
EFIAPI
FindPermissionEntry(
IN CONST DFCI_PERMISSION_STORE *Store,
IN DFCI_SETTING_ID_STRING Id);

/**
Print the current state of the Permission Store using Debug
**/
VOID
EFIAPI
DebugPrintPermissionStore(IN CONST DFCI_PERMISSION_STORE *Store);



/**
Main Entry point into the Xml Provisioning code.
This will check the incomming variables, authenticate them, and apply permission settings.
**/
VOID
EFIAPI
CheckForPendingPermissionChanges();

#endif // DFCI_SETTING_PERMISSION_H