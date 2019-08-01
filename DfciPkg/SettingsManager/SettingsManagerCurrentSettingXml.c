/**@file
SettingsManagerCurrentSettingXml.c

Settings Manager component to create the Current Settings variable

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "SettingsManager.h"

/**
Clear the cached Current Settings string
so the next boot it will be repopulated.
**/
VOID
EFIAPI
ClearCacheOfCurrentSettings()
{
  EFI_STATUS Status;
  Status = gRT->SetVariable(DFCI_SETTINGS_CURRENT_OUTPUT_VAR_NAME, &gDfciSettingsManagerVarNamespace, 0, 0, NULL);
  DEBUG((DEBUG_INFO, "Delete Current Xml Settings %r\n", Status));
}


/**
Create an XML string from all the current settings

**/
EFI_STATUS
EFIAPI
CreateXmlStringFromCurrentSettings(
  OUT CHAR8** XmlString,
  OUT UINTN*  StringSize,
  IN  BOOLEAN V1Compatible
  )
{
  EFI_STATUS Status;
  XmlNode* List = NULL;
  XmlNode* CurrentSettingsNode = NULL;
  XmlNode* CurrentSettingsListNode = NULL;
  CHAR8     LsvString[20];
  EFI_TIME Time;
  UINT32 Lsv = 0;
  DFCI_SETTING_INTERNAL_DATA *InternalData = NULL;

  if ((XmlString == NULL) || (StringSize == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  PERF_FUNCTION_BEGIN ();

  Status = SMID_LoadFromFlash(&InternalData);
  if (EFI_ERROR(Status))
  {
    if (Status != EFI_NOT_FOUND)
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to load Settings Manager Internal Data.  LSV is 0. Status = %r\n", __FUNCTION__, Status));
    }
    else
    {
      DEBUG((DEBUG_INFO, "%a - Internal Data Var not found.  LSV will be 0.\n", __FUNCTION__));
    }
  }
  else
  {
    Lsv = InternalData->LSV;
    FreePool(InternalData);
    InternalData = NULL;
  }

  //create basic xml
  Status = gRT->GetTime(&Time, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get time. %r\n", __FUNCTION__, Status));
    goto EXIT;
  }

  List = New_CurrentSettingsPacketNodeList(&Time);
  if (List == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create new Current Settings Packet List Node\n", __FUNCTION__));
    Status = EFI_ABORTED;
    goto EXIT;
  }

  //Get SettingsPacket Node
  CurrentSettingsNode = GetCurrentSettingsPacketNode(List);
  if (CurrentSettingsNode == NULL)
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
  AsciiValueToStringS(&(LsvString[0]), sizeof(LsvString), 0, (UINT32)Lsv, 19);
  Status = AddSettingsLsvNode(CurrentSettingsNode, LsvString);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_INFO, "Failed to set LSV Node for current settings. %r\n", Status));
    goto EXIT;
  }

  //
  //Get the Settings Node List Node
  //
  CurrentSettingsListNode = GetSettingsListNodeFromPacketNode(CurrentSettingsNode);
  if (CurrentSettingsListNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get Settings List Node from Packet Node\n"));
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  //
  // Loop each setting in the provider list
  //
  for (LIST_ENTRY* Link = mProviderList.ForwardLink; Link != &mProviderList; Link = Link->ForwardLink)
  {
    CHAR8 *Value = NULL;
    DFCI_SETTING_PROVIDER_LIST_ENTRY *Prov = PROV_LIST_ENTRY_FROM_LINK(Link);
    Value = ProviderValueAsAscii(&(Prov->Provider), TRUE);

    if (V1Compatible)
    {
      DFCI_SETTING_ID_STRING NumberString;
      NumberString = DfciV1NumberFromId (Prov->Provider.Id);
      Status = SetCurrentSettings(CurrentSettingsListNode, NumberString, Value);
    } else {
      Status = SetCurrentSettings(CurrentSettingsListNode, Prov->Provider.Id, Value);
    }

    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Error from Set Current Settings.  Status = %r\n", __FUNCTION__, Status));
      DEBUG((DEBUG_ERROR, "ID %a\nValue %a\n", Prov->Provider.Id, Value));
    }
    if (Value != NULL)
    {
      FreePool(Value);
    }
  } //end for loop

  DEBUG((DEBUG_INFO, "Processing Group settings\n"));
  //
  // Loop to present all group settings
  //
  for (LIST_ENTRY *Link = GetFirstNode (&mGroupList)
       ; !IsNull (&mGroupList, Link)
       ; Link = GetNextNode (&mGroupList, Link)
       )
  {
    UINTN  ValueSize;
    UINT8  Value;
    CHAR8 *ReturnValue;
    DFCI_GROUP_LIST_ENTRY *Group;

    Group = GROUP_LIST_ENTRY_FROM_GROUP_LINK(Link);
    Status = SystemSettingAccessGet (
                   &mSystemSettingAccessProtocol,
                    Group->GroupId,
                    NULL,
                    DFCI_SETTING_TYPE_ENABLE,
                  &ValueSize,
                  &Value,
                   NULL);

    if (EFI_ERROR(Status))
    {
      ReturnValue = "Error";
    }
    else
    {
      switch (Value) {
        case ENABLE_FALSE:
            ReturnValue = "Disabled";
            break;
        case ENABLE_TRUE:
            ReturnValue = "Enabled";
            break;
        case ENABLE_INCONSISTENT:
            ReturnValue = "Inconsistent";
            break;
        default:
            ReturnValue = "Unknown";
            break;
      }
    }

    DEBUG((DEBUG_INFO, "   Setting Group Setting %a to %a\n", Group->GroupId, ReturnValue));
    Status = SetCurrentSettings(CurrentSettingsListNode, Group->GroupId, ReturnValue);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "Error %r\n", Status));
    }
  }

  //print the list
  DEBUG((DEBUG_INFO, "PRINTING CURRENT SETTINGS XML - Start\n"));
  DebugPrintXmlTree(List, 0);
  DEBUG((DEBUG_INFO, "PRINTING CURRENT SETTINGS XML - End\n"));

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
  PERF_FUNCTION_END ();
  return Status;
}

EFI_STATUS
EFIAPI
PopulateCurrentSettingsIfNeeded()
{
  EFI_STATUS Status;
  CHAR8* Var = NULL;
  UINTN  VarSize = 0;
  Status = GetVariable2(DFCI_SETTINGS_CURRENT_OUTPUT_VAR_NAME,
    &gDfciSettingsManagerVarNamespace,
    (VOID **) &Var,
    &VarSize
    );

  if (!EFI_ERROR(Status))
  {
    FreePool(Var);
    DEBUG((DEBUG_INFO, "%a - Current Settings already set\n", __FUNCTION__));
    return EFI_SUCCESS;
  }

  //Error.  Need to repopulate after first cleaning up
  if (Status != EFI_NOT_FOUND)
  {
    DEBUG((DEBUG_ERROR, "%a - Unexpected Error getting Current Settings %r\n", __FUNCTION__, Status));
    ClearCacheOfCurrentSettings();
    if (Var != NULL)
    {
      FreePool(Var);
      Var = NULL;
    }
    VarSize = 0;
  }

  //Create string of Xml
  Status = CreateXmlStringFromCurrentSettings(&Var, &VarSize, FALSE);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create xml string from current %r\n", __FUNCTION__, Status));
    goto EXIT;
  }

  //Save variable
  Status = gRT->SetVariable(DFCI_SETTINGS_CURRENT_OUTPUT_VAR_NAME, &gDfciSettingsManagerVarNamespace, DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES, VarSize, Var);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to write current setting Xml variable %r\n", __FUNCTION__, Status));
    goto EXIT;
  }
  //Success
  DEBUG((DEBUG_INFO, "%a - Current Settings Xml Var Set with data size: 0x%X\n", __FUNCTION__, VarSize));
  Status = EFI_SUCCESS;

  //
  //
  // TEMP HACK Set V1 Current Settings
  //
  //

  FreePool (Var);
  Var = NULL;

  //Create string of Xml
  Status = CreateXmlStringFromCurrentSettings(&Var, &VarSize, TRUE);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create xml string from current %r\n", __FUNCTION__, Status));
    goto EXIT;
  }

  //Save variable
  Status = gRT->SetVariable(L"UEFISettingsCurrent", &gDfciSettingsManagerVarNamespace, DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES, VarSize, Var);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to write current setting Xml variable %r\n", __FUNCTION__, Status));
    goto EXIT;
  }
  //Success
  DEBUG((DEBUG_INFO, "%a - Current Settings Xml Var Set with data size: 0x%X\n", __FUNCTION__, VarSize));
  Status = EFI_SUCCESS;



EXIT:
  if (Var != NULL)
  {
    FreePool(Var);
    Var = NULL;
  }
  return Status;
}

