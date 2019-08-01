/**@file
DfciSettingPermissionCurrentPermissionXml.c

Create an XML string from all the current settings

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DfciSettingPermission.h"

static
EFI_STATUS
CreateXmlStringFromCurrentPermissions(
  OUT CHAR8** XmlString,
  OUT UINTN*  StringSize
  )
{
  EFI_STATUS Status;
  XmlNode  *List = NULL;
  XmlNode  *CurrentPermissionsNode = NULL;
  XmlNode  *CurrentPermissionsListNode = NULL;
  CHAR8     LsvString[20];   
  EFI_TIME  Time;
  UINT32    Lsv = 0;

  if ((XmlString == NULL) || (StringSize == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //create basic xml 
  Status = gRT->GetTime(&Time, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get time. %r\n", __FUNCTION__, Status));
    return Status;
  }

  List = New_CurrentPermissionsPacketNodeList(&Time);
  if (List == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create new Current Settings Packet List Node\n", __FUNCTION__));
    Status = EFI_ABORTED;
    goto EXIT;
  }

  //Get SettingsPacket Node 
  CurrentPermissionsNode = GetCurrentPermissionsPacketNode(List);
  if (CurrentPermissionsNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get GetCurrentSettingsPacketNode Node\n"));
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  //Record Status result
  //
  // Add the Lowest Supported Version Node
  //
  ZeroMem(LsvString, sizeof(LsvString));
  AsciiValueToStringS(&(LsvString[0]), sizeof(LsvString), 0, (UINT32)Lsv, sizeof(LsvString)-1);
  Status = AddPermissionsLsvNode(CurrentPermissionsNode, LsvString);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_INFO, "Failed to set LSV Node for current permissions. %r\n", Status));
    goto EXIT;
  }

  //
  //Get the Settings Node List Node
  //
  CurrentPermissionsListNode = GetPermissionsListNodeFromPacketNode(CurrentPermissionsNode);
  if (CurrentPermissionsListNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get Permissions List Node from Packet Node\n"));
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  //
  // Loop each permission in the permissions store
  //

  // mPermStore is set by the library constructor
  ASSERT (mPermStore != NULL);
  if (mPermStore == NULL)
  {
      DEBUG((DEBUG_ERROR, "mPermStore has not been initialized\n"));
      Status = EFI_NOT_FOUND;
      goto EXIT;
  }

  Status = AddCurrentAttributes(CurrentPermissionsNode, mPermStore->DefaultPMask, mPermStore->DefaultDMask);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Unable to add permissions attributes. Code=%r\n",Status));
    goto EXIT;
  }

  for (LIST_ENTRY *Link = mPermStore->PermissionsListHead.ForwardLink; Link != &(mPermStore->PermissionsListHead); Link = Link->ForwardLink)
  {
      DFCI_PERMISSION_ENTRY *Temp;
      Temp = CR(Link, DFCI_PERMISSION_ENTRY, Link, DFCI_PERMISSION_LIST_ENTRY_SIGNATURE);
      DEBUG((DEBUG_INFO, "   PERM ENTRY - Id: %a  Permission: 0x%X  DelegatedPermission: 0x%X\n", Temp->Id, Temp->PMask, Temp->DMask));
      Status = SetCurrentPermissions(CurrentPermissionsListNode, Temp->Id, Temp->PMask, Temp->DMask);
  } //end for loop

  //print the list
  DEBUG((DEBUG_INFO, "PRINTING CURRENT PERMISSIONS XML - Start\n"));
  DebugPrintXmlTree(List, 0);
  DEBUG((DEBUG_INFO, "PRINTING CURRENT PERMISSIONS XML - End\n"));

  //now output as xml string

  Status = XmlTreeToString(List, TRUE, StringSize, XmlString);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - XmlTreeToString failed.  %r\n", __FUNCTION__, Status));
  }

EXIT:
  if (List != NULL)
  {
	  FreeXmlTree(&List);
  }

  if (EFI_ERROR(Status))
  {
    //free memory since it was an error
    if (*XmlString != NULL)
    {
      FreePool(*XmlString);
      *XmlString = NULL;
      *StringSize = 0;
    }
  }
  return Status;
}

EFI_STATUS
EFIAPI
PopulateCurrentPermissions(BOOLEAN Force)
{
  EFI_STATUS Status;
  UINT32     Attributes;
  CHAR8     *Var = NULL;
  UINTN      VarSize;

  VarSize = 0;
  Status = gRT->GetVariable(DFCI_PERMISSION_POLICY_CURRENT_VAR_NAME,
                           &gDfciPermissionManagerVarNamespace,
                           &Attributes,
                           &VarSize,
                            NULL);
  if ((EFI_BUFFER_TOO_SMALL == Status) &&
      (DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES == Attributes))
  {
      DEBUG((DEBUG_INFO, "%a - Current Permissions Xml already set\n", __FUNCTION__));
      if (!Force)
      {
          return EFI_SUCCESS;
      }
  }
  if ((EFI_SUCCESS == Status) &&
      (DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES != Attributes))
  {
      // Delete the current variable if it has incorrect attributes
      Status = gRT->SetVariable(DFCI_PERMISSION_POLICY_CURRENT_VAR_NAME,
                               &gDfciPermissionManagerVarNamespace,
                               0,
                               0,
                               NULL);
  }

  //Create string of Xml
  Status = CreateXmlStringFromCurrentPermissions(&Var, &VarSize);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create xml string from current permissions %r\n", __FUNCTION__, Status));
    goto EXIT;
  }

  //Save variable
  Status = gRT->SetVariable(DFCI_PERMISSION_POLICY_CURRENT_VAR_NAME, &gDfciPermissionManagerVarNamespace, DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES, VarSize, Var);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to write current permissions Xml variable %r\n", __FUNCTION__, Status));
    goto EXIT;
  }
  //Success
  DEBUG((DEBUG_INFO, "%a - Current Permissions Xml Var Set with data size: 0x%X\n", __FUNCTION__, VarSize));
  Status = EFI_SUCCESS;

EXIT:
  if (Var != NULL)
  {
    FreePool(Var);
    Var = NULL;
  }
  return Status;
}
