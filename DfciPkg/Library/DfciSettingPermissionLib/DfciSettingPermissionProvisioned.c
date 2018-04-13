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
#define VAR_VERSION_V1 (1)
#define VAR_VERSION_V2 (2)
#define MAX_SIZE_FOR_VAR (1024 * 2)  

#pragma warning(push)
#pragma warning(disable: 4200) // zero-sized array
#pragma pack (push, 1)

typedef struct {
  DFCI_SETTING_ID_V1_ENUM Id;     // DFCI_SETTING_ID_ENUM is no longer
  DFCI_PERMISSION_MASK    Permissions;
} DFCI_PERM_INTERNAL_TABLE_ENTRY_V1;

typedef struct {
  UINT32 HeaderSignature;     // 'S', 'B', 'C', 'Z'
  UINT8  HeaderVersion;       // 1
  UINT32 Version; 
  UINT32 LowestSupportedVersion; 
  EFI_TIME CreatedOn;
  EFI_TIME SavedOn;
  DFCI_PERMISSION_MASK Default; //For any ID not found in the entry list this is the permission
  UINT16 NumberOfEntries;     // Number of entris in the PermTable 
  DFCI_PERM_INTERNAL_TABLE_ENTRY_V1 PermTable[];
} DFCI_PERM_INTERNAL_PROVISONED_VAR_V1;

typedef struct {
  DFCI_PERMISSION_MASK Permissions;
  UINT8                IdSize;          // Setting ID's have a max length of 97
  CHAR8                Id[];            // Must be variable in length;
} DFCI_PERM_INTERNAL_TABLE_ENTRY;

typedef struct {
  UINT32 HeaderSignature;     // 'S', 'B', 'C', 'Z'
  UINT8  HeaderVersion;       // 2
  UINT32 Version;
  UINT32 LowestSupportedVersion; // 2
  EFI_TIME CreatedOn;
  EFI_TIME SavedOn;
  DFCI_PERMISSION_MASK Default; //For any ID not found in the entry list this is the permission
  UINT16 NumberOfEntries;     // Number of entris in the PermTable
  DFCI_PERM_INTERNAL_TABLE_ENTRY PermTableStart; // First variable lenght entry
} DFCI_PERM_INTERNAL_PROVISONED_VAR;

#pragma pack (pop)
#pragma warning(pop)


EFI_STATUS
EFIAPI
LoadFromFlash(IN DFCI_PERMISSION_STORE **Store)
{
  EFI_STATUS Status;
  DFCI_PERM_INTERNAL_PROVISONED_VAR_V1 *Var1 = NULL;
  DFCI_PERM_INTERNAL_PROVISONED_VAR    *Var = NULL;
  UINTN VarSize = 0;
  UINTN ComputedSize = 0;
  UINT32 VarAttributes = 0;
  DFCI_SETTING_ID_STRING Id;
  UINTN                           Count;
  CHAR8                          *PermPtr;
  CHAR8                          *EndPtr;
  DFCI_PERM_INTERNAL_TABLE_ENTRY *PermEntry;

  if (Store == NULL)
  {
    ASSERT(Store != NULL);
    return EFI_INVALID_PARAMETER;
  }

  *Store = NULL;

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

  // Two possibilities here.  V1 has old setting ENUMs, and we have to convert these to
  // V2 setting strings here if we run into a V1 provisioned system. All fields except the
  // PermTable are the same between V1 and V2.

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

  (*Store)->Version = Var->Version;
  (*Store)->Lsv = Var->LowestSupportedVersion;
  (*Store)->Modified = FALSE;  //since flash matches store modified is false
  (*Store)->Default = Var->Default;
  (*Store)->CreatedOn = Var->CreatedOn;
  (*Store)->SavedOn = Var->SavedOn;
  InitializeListHead(&((*Store)->PermissionsListHead));

  switch (Var->HeaderVersion)
  {
    case VAR_VERSION_V1:
      Var1 = (DFCI_PERM_INTERNAL_PROVISONED_VAR_V1 *) Var;
      ComputedSize = sizeof(DFCI_PERM_INTERNAL_PROVISONED_VAR_V1) + (Var->NumberOfEntries * sizeof(DFCI_PERM_INTERNAL_TABLE_ENTRY_V1));
      if (VarSize != ComputedSize)
      {
        DEBUG((DEBUG_ERROR, "%a - VarSize (0x%X) != ComputedSize (0x%X)\n", __FUNCTION__, VarSize, ComputedSize));
        ASSERT(VarSize == ComputedSize);
        Status = EFI_COMPROMISED_DATA;
        goto EXIT;
      }
      for (UINT16 i = 0; i < Var->NumberOfEntries; i++)
      {
        Id = DfciV1TranslateEnum(Var1->PermTable[i].Id);
        Status = AddPermissionEntry(*Store, Id, Var1->PermTable[i].Permissions);
        if (EFI_ERROR(Status))
        {
          DEBUG((DEBUG_ERROR, "%a - Failed to add a permission entry. %r\n", __FUNCTION__, Status));
          ASSERT_EFI_ERROR(Status);
          //continue anyway and hope for the best :)
        }
      }
      SaveToFlash (*Store);  // Complete the translation from V1 to V2
      DEBUG((DEBUG_INFO, "%a - Permission store converted to V2.\n", __FUNCTION__));
      break;

    case VAR_VERSION_V2:
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
          break;
        }
        Status = AddPermissionEntry(*Store, (DFCI_SETTING_ID_STRING)PermEntry->Id, PermEntry->Permissions);
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
        goto EXIT;
      }
      break;
    default:
      DEBUG((DEBUG_INFO, "%a - Var Header Version %d not supported.\n", __FUNCTION__, Var->HeaderVersion));
      Status = EFI_COMPROMISED_DATA;
      goto EXIT;
      break;
  }

  DEBUG((DEBUG_INFO, "%a - Loaded valid variable. Version %d.  Code=%r\n", __FUNCTION__, Var->HeaderVersion, Status));

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
  Var->HeaderSignature = VAR_HEADER_SIG;
  Var->HeaderVersion = VAR_VERSION_V2;
  Var->Version = Store->Version;
  Var->LowestSupportedVersion = Store->Lsv;  
  Var->Default = Store->Default;
  CopyMem(&(Var->CreatedOn), &(Store->CreatedOn), sizeof(Var->CreatedOn));

  PermEntry = &Var->PermTableStart;
  PermPtr = (CHAR8 *) PermEntry;

  Var->NumberOfEntries = (UINT16)NumEntries;  //can't exceed UINT16 size because of var size requirements and previous checks
  for (LIST_ENTRY *Link = Store->PermissionsListHead.ForwardLink; Link != &(Store->PermissionsListHead); Link = Link->ForwardLink)
  {
    DFCI_PERMISSION_ENTRY *Temp = CR(Link, DFCI_PERMISSION_ENTRY, Link, DFCI_PERMISSION_LIST_ENTRY_SIGNATURE);

    PermEntry->Permissions = Temp->Perm;
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
