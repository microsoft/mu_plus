/** @file
DfciSettingPermissionProvisioned

This file supports loading internal data (previously provisioned) from flash so that 
SettingPermission code can use it.  


Copyright (c) 2015, Microsoft Corporation. 

**/
#include "DfciSettingPermission.h"


//Define the local structure for the variable (this is for internal use only)
// Variable namespace - Use gEfiCallerIdGuid since this is internal only

#define VAR_NAME  L"_SPP"
#define VAR_HEADER_SIG SIGNATURE_32('S', 'B', 'C', 'Z')
#define VAR_VERSION (1)
#define MAX_SIZE_FOR_VAR (1024 * 2)  

#pragma warning(push)
#pragma warning(disable: 4200) // zero-sized array
#pragma pack (push, 1)

typedef struct {
  UINT32 Id;
  DFCI_PERMISSION_MASK Permissions;
} DFCI_PERM_INTERNAL_TABLE_ENTRY;

typedef struct {
  UINT32 HeaderSignature;     // 'S', 'B', 'C', 'Z'
  UINT8  HeaderVersion;       // 1
  UINT32 Version; 
  UINT32 LowestSupportedVersion; 
  EFI_TIME CreatedOn;
  EFI_TIME SavedOn;
  DFCI_PERMISSION_MASK Default; //For any ID not found in the entry list this is the permission
  UINT16 NumberOfEntries;     // Number of entris in the PermTable 
  DFCI_PERM_INTERNAL_TABLE_ENTRY PermTable[];
} DFCI_PERM_INTERNAL_PROVISONED_VAR;

#pragma pack (pop)
#pragma warning(pop)


EFI_STATUS
EFIAPI
LoadFromFlash(IN DFCI_PERMISSION_STORE **Store)
{
  EFI_STATUS Status;
  DFCI_PERM_INTERNAL_PROVISONED_VAR *Var = NULL;
  UINTN VarSize = 0;
  UINTN ComputedSize = 0;
  UINT32 VarAttributes = 0;

  if (Store == NULL)
  {
    ASSERT(Store != NULL);
    return EFI_INVALID_PARAMETER;
  }

  //1. Load Variable
  Status = GetVariable3(VAR_NAME, &gDfciInternalVariableGuid, &Var, &VarSize, &VarAttributes);
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
  if (Var->HeaderSignature != VAR_HEADER_SIG)
  {
    DEBUG((DEBUG_INFO, "%a - Var Header Signature wrong.\n", __FUNCTION__));
    Status = EFI_COMPROMISED_DATA;
    goto EXIT;
  }
  if (Var->HeaderVersion != VAR_VERSION)
  {
    DEBUG((DEBUG_INFO, "%a - Var Header Version Wrong %d.\n", __FUNCTION__, Var->HeaderVersion));
    Status = EFI_COMPROMISED_DATA;
    goto EXIT;
  }

  ComputedSize = sizeof(DFCI_PERM_INTERNAL_PROVISONED_VAR) + (Var->NumberOfEntries * sizeof(DFCI_PERM_INTERNAL_TABLE_ENTRY));
  if (VarSize != ComputedSize)
  {
    DEBUG((DEBUG_ERROR, "%a - VarSize (0x%X) != ComputedSize (0x%X)\n", __FUNCTION__, VarSize, ComputedSize));
    ASSERT(VarSize == ComputedSize);
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

  DEBUG((DEBUG_INFO, "%a - Loaded valid variable\n", __FUNCTION__));

  //4. Process variable to load it into Store
  ComputedSize = sizeof(DFCI_PERMISSION_STORE);
  *Store = (DFCI_PERMISSION_STORE *) AllocateZeroPool(ComputedSize);
  if (*Store == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to allocate memory for Store\n", __FUNCTION__));
    Status= EFI_ABORTED;
    goto EXIT;
  }
  
  (*Store)->Version = Var->Version;
  (*Store)->Lsv = Var->LowestSupportedVersion;
  (*Store)->Modified = FALSE;  //since flash matches store modified is false
  (*Store)->Default = Var->Default;
  (*Store)->CreatedOn = Var->CreatedOn;
  (*Store)->SavedOn = Var->SavedOn;
  InitializeListHead(&((*Store)->PermissionsListHead));
  for (UINT16 i = 0; i < Var->NumberOfEntries; i++)
  {
    Status = AddPermissionEntry(*Store, Var->PermTable[i].Id, Var->PermTable[i].Permissions);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to add a permission entry. %r\n", __FUNCTION__, Status));
      ASSERT_EFI_ERROR(Status);
      //continue anyway and hope for the best :) 
    }
  }
  DEBUG((DEBUG_INFO, "%a - Loaded from flash successfully.\n", __FUNCTION__));
  Status = EFI_SUCCESS;

EXIT:
  if (Var != NULL)
  {
    FreePool(Var);
  }
  return Status;
}

EFI_STATUS
EFIAPI
SaveToFlash(IN DFCI_PERMISSION_STORE *Store)
{
  EFI_STATUS Status;
  DFCI_PERM_INTERNAL_PROVISONED_VAR *Var = NULL;
  UINTN VarSize = 0;
  UINTN NumEntries = 0;
  UINT16 i = 0;
  EFI_TIME t;

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
  NumEntries = GetNumberOfPermissionEntires(Store);
  //Figure out our size
  VarSize = sizeof(DFCI_PERM_INTERNAL_PROVISONED_VAR) + (NumEntries * sizeof(DFCI_PERM_INTERNAL_TABLE_ENTRY));
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
  Var->HeaderSignature = VAR_HEADER_SIG;
  Var->HeaderVersion = VAR_VERSION;
  Var->Version = Store->Version;
  Var->LowestSupportedVersion = Store->Lsv;  
  Var->Default = Store->Default;
  CopyMem(&(Var->CreatedOn), &(Store->CreatedOn), sizeof(Var->CreatedOn));
  
  Var->NumberOfEntries = (UINT16)NumEntries;  //can't exceed UINT16 size because of var size requirements and previous checks
  for (LIST_ENTRY *Link = Store->PermissionsListHead.ForwardLink; Link != &(Store->PermissionsListHead); Link = Link->ForwardLink)
  {
    ASSERT(i < NumEntries);
    DFCI_PERMISSION_ENTRY *Temp = CR(Link, DFCI_PERMISSION_ENTRY, Link, DFCI_PERMISSION_LIST_ENTRY_SIGNATURE);
    Var->PermTable[i].Id = Temp->Id;
    Var->PermTable[i].Permissions = Temp->Perm;
    i++;
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
