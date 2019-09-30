/**@file
DfciSettingPermission.h

Local header file to support the different implementation files for DfciSettingPermissionLib

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef DFCI_SETTING_PERMISSION_H
#define DFCI_SETTING_PERMISSION_H

#include <PiDxe.h>
#include <DfciSystemSettingTypes.h>
#include <XmlTypes.h>

#include <Guid/DfciInternalVariableGuid.h>
#include <Guid/DfciPacketHeader.h>
#include <Guid/DfciPermissionManagerVariables.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DfciDeviceIdSupportLib.h>
#include <Library/DfciSettingPermissionLib.h>
#include <Library/DfciV1SupportLib.h>
#include <Library/DfciXmlPermissionSchemaSupportLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/XmlTreeLib.h>
#include <Library/XmlTreeQueryLib.h>

#include <Private/DfciGlobalPrivate.h>

#include <Protocol/DfciAuthentication.h>
#include <Protocol/DfciApplyPacket.h>

#include <Settings/DfciSettings.h>

// START - Tianocore Bugzilla 1959.  Remove this once fixed

//
//Iterate through the double linked list. NOT delete safe
//
#define EFI_LIST_FOR_EACH(Entry, ListHead)    \
  for(Entry = (ListHead)->ForwardLink; Entry != (ListHead); Entry = Entry->ForwardLink)

//
//Iterate through the double linked list. This is delete-safe.
//Don't touch NextEntry
//
#define EFI_LIST_FOR_EACH_SAFE(Entry, NextEntry, ListHead)            \
  for(Entry = (ListHead)->ForwardLink, NextEntry = Entry->ForwardLink;\
      Entry != (ListHead); Entry = NextEntry, NextEntry = Entry->ForwardLink)
// END - Tianocore Bugzilla 1959.  Remove this once fixed


#define DFCI_PERMISSION_LIST_ENTRY_SIGNATURE SIGNATURE_32('M','P','L','S')

typedef struct {
  UINTN Signature;
  LIST_ENTRY Link;
  DFCI_SETTING_ID_STRING Id;              // Pointer to IdStore
  DFCI_PERMISSION_MASK   PMask;
  DFCI_PERMISSION_MASK   DMask;
  UINT8                  IdSize;
  CHAR8                  IdStore[];
} DFCI_PERMISSION_ENTRY;


typedef struct {
  UINT32 Version;
  UINT32 Lsv;
  BOOLEAN Modified;
  EFI_TIME CreatedOn;
  EFI_TIME SavedOn;
  DFCI_PERMISSION_MASK DefaultPMask;
  DFCI_PERMISSION_MASK DefaultDMask;
  LIST_ENTRY PermissionsListHead;
} DFCI_PERMISSION_STORE;

extern DFCI_AUTHENTICATION_PROTOCOL *mAuthenticationProtocol;
extern DFCI_PERMISSION_STORE        *mPermStore;
extern DFCI_APPLY_PACKET_PROTOCOL    mApplyPermissionsProtocol;

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
IN DFCI_PERMISSION_MASK PMask,
IN DFCI_PERMISSION_MASK DMask);

/**
Find Permission Entry for a given Id.

If doesn't exist return NULL
**/
DFCI_PERMISSION_ENTRY*
EFIAPI
FindPermissionEntry(
IN CONST DFCI_PERMISSION_STORE *Store,
IN DFCI_SETTING_ID_STRING       Id);

/**
 * Mark permissions entries for deletion
 *
 */
EFI_STATUS
MarkPermissionEntriesForDeletion (
IN DFCI_PERMISSION_STORE *Store,
IN DFCI_IDENTITY_ID       Id);

/**
 * Delete permissions entries of this Id
 *
 */
EFI_STATUS
DeleteMarkedPermissionEntries (
IN DFCI_PERMISSION_STORE *Store);

/**
 * Add a new, or update an existing permission entry
 *
 */
EFI_STATUS
EFIAPI
AddRequiredPermissionEntry (
    IN DFCI_PERMISSION_STORE *Store,
    IN DFCI_SETTING_ID_STRING Id,
    IN DFCI_PERMISSION_MASK   PMask,
    IN DFCI_PERMISSION_MASK   DMask);

/**
Print the current state of the Permission Store using Debug
**/
VOID
EFIAPI
DebugPrintPermissionStore(IN CONST DFCI_PERMISSION_STORE *Store);

EFI_STATUS
EFIAPI
ApplyNewPermissionsPacket (
    IN CONST DFCI_APPLY_PACKET_PROTOCOL *This,
    IN       DFCI_INTERNAL_PACKET       *ApplyPacket
  );

EFI_STATUS
EFIAPI
SetPermissionsResponse(
  IN  CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
  IN        DFCI_INTERNAL_PACKET        *Data
  );

EFI_STATUS
EFIAPI
LKG_Handler(
    IN  CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
    IN        DFCI_INTERNAL_PACKET        *ApplyPacket,
    IN        UINT8                        Operation
  );

EFI_STATUS
EFIAPI
PopulateCurrentPermissions(BOOLEAN Force);

#endif // DFCI_SETTING_PERMISSION_H