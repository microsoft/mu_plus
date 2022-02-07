/**@file
SettingsManagerCurrentSettingXml.c

Settings Manager component to create the Current Settings variable

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "SettingsManager.h"

/**
Create an XML string from all the current settings

**/
EFI_STATUS
EFIAPI
CreateXmlStringFromCurrentSettings (
  OUT CHAR8    **XmlString,
  OUT UINTN    *StringSize,
  IN  BOOLEAN  V1Compatible
  )
{
  EFI_STATUS                  Status;
  XmlNode                     *List                    = NULL;
  XmlNode                     *CurrentSettingsNode     = NULL;
  XmlNode                     *CurrentSettingsListNode = NULL;
  CHAR8                       LsvString[20];
  EFI_TIME                    Time;
  UINT32                      Lsv           = 0;
  DFCI_SETTING_INTERNAL_DATA  *InternalData = NULL;

  if ((XmlString == NULL) || (StringSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PERF_FUNCTION_BEGIN ();

  Status = SMID_LoadFromFlash (&InternalData);
  if (EFI_ERROR (Status)) {
    if (Status != EFI_NOT_FOUND) {
      DEBUG ((DEBUG_ERROR, "%a - Failed to load Settings Manager Internal Data.  LSV is 0. Status = %r\n", __FUNCTION__, Status));
    } else {
      DEBUG ((DEBUG_INFO, "%a - Internal Data Var not found.  LSV will be 0.\n", __FUNCTION__));
    }
  } else {
    Lsv = InternalData->LSV;
    FreePool (InternalData);
    InternalData = NULL;
  }

  // create basic xml
  Status = gRT->GetTime (&Time, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to get time. %r\n", __FUNCTION__, Status));
    goto EXIT;
  }

  List = New_CurrentSettingsPacketNodeList (&Time);
  if (List == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to create new Current Settings Packet List Node\n", __FUNCTION__));
    Status = EFI_ABORTED;
    goto EXIT;
  }

  // Get SettingsPacket Node
  CurrentSettingsNode = GetCurrentSettingsPacketNode (List);
  if (CurrentSettingsNode == NULL) {
    DEBUG ((DEBUG_INFO, "Failed to Get GetCurrentSettingsPacketNode Node\n"));
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  // Record Status result
  //
  // Add the Lowest Supported Version Node
  //
  ZeroMem (LsvString, sizeof (LsvString));
  AsciiValueToStringS (&(LsvString[0]), sizeof (LsvString), 0, (UINT32)Lsv, 19);
  Status = AddSettingsLsvNode (CurrentSettingsNode, LsvString);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Failed to set LSV Node for current settings. %r\n", Status));
    goto EXIT;
  }

  //
  // Get the Settings Node List Node
  //
  CurrentSettingsListNode = GetSettingsListNodeFromPacketNode (CurrentSettingsNode);
  if (CurrentSettingsListNode == NULL) {
    DEBUG ((DEBUG_INFO, "Failed to Get Settings List Node from Packet Node\n"));
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  //
  // Loop each setting in the provider list
  //
  for (LIST_ENTRY *Link = mProviderList.ForwardLink; Link != &mProviderList; Link = Link->ForwardLink) {
    CHAR8                             *Value = NULL;
    DFCI_SETTING_PROVIDER_LIST_ENTRY  *Prov  = PROV_LIST_ENTRY_FROM_LINK (Link);
    Value = ProviderValueAsAscii (&(Prov->Provider), TRUE);

    if (V1Compatible) {
      DFCI_SETTING_ID_STRING  NumberString;
      NumberString = DfciV1NumberFromId (Prov->Provider.Id);
      Status       = SetCurrentSettings (CurrentSettingsListNode, NumberString, Value);
    } else {
      Status = SetCurrentSettings (CurrentSettingsListNode, Prov->Provider.Id, Value);
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Error from Set Current Settings.  Status = %r\n", __FUNCTION__, Status));
      DEBUG ((DEBUG_ERROR, "ID %a\nValue %a\n", Prov->Provider.Id, Value));
    }

    if (Value != NULL) {
      FreePool (Value);
    }
  } // end for loop

  DEBUG ((DEBUG_INFO, "Processing Group settings\n"));
  //
  // Loop to present all group settings
  //
  for (LIST_ENTRY *Link = GetFirstNode (&mGroupList)
       ; !IsNull (&mGroupList, Link)
       ; Link = GetNextNode (&mGroupList, Link)
       )
  {
    UINTN                   ValueSize;
    UINT8                   Value;
    CHAR8                   *ReturnValue;
    DFCI_GROUP_LIST_ENTRY   *Group;
    DFCI_SETTING_TYPE       GroupType;
    DFCI_MEMBER_LIST_ENTRY  *Member;
    DFCI_SETTING_PROVIDER   *Provider;

    ValueSize = sizeof (*ReturnValue);
    Group     = GROUP_LIST_ENTRY_FROM_GROUP_LINK (Link);
    GroupType = DFCI_SETTING_TYPE_UNDEFINED;

    if (!IsListEmpty (&Group->MemberHead)) {
      Member   = MEMBER_LIST_ENTRY_FROM_MEMBER_LINK (Group->MemberHead.ForwardLink);
      Provider = FindProviderById (Member->Id);
      if (Provider == NULL) {
        return EFI_NOT_FOUND;
      }

      GroupType = Provider->Type;
    } else {
      return EFI_NOT_FOUND;
    }

    Status = SystemSettingAccessGet (
               &mSystemSettingAccessProtocol,
               Group->GroupId,
               NULL,
               GroupType,
               &ValueSize,
               &Value,
               NULL
               );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Error accessing %a, Code=%r\n", __FUNCTION__, Group->GroupId, Status));
      ReturnValue = "Error";
    } else {
      switch (GroupType) {
        case DFCI_SETTING_TYPE_ENABLE:
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

          break;

        case DFCI_SETTING_TYPE_USBPORTENUM:
          switch (Value) {
            case DfciUsbPortHwDisabled:
              ReturnValue = "UsbPortHwDisabled";
              break;

            case DfciUsbPortEnabled:
              ReturnValue = "UsbPortEnabled";
              break;

            case DfciUsbPortDataDisabled:
              ReturnValue = "UsbPortDataDisabled";
              break;

            case ENABLE_INCONSISTENT:
              ReturnValue = "Inconsistent";
              break;

            default:
              ReturnValue = "UnsupportedValue";
              break;
          }

          break;

        default:
          DEBUG ((DEBUG_ERROR, "%a: Group entries for type(%d) not supported\n", __FUNCTION__, GroupType));
          ReturnValue = "UnsupportedValue";
          break;
      }
    }

    DEBUG ((DEBUG_INFO, "   Group Setting %a is %a\n", Group->GroupId, ReturnValue));
    Status = SetCurrentSettings (CurrentSettingsListNode, Group->GroupId, ReturnValue);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Error %r\n", Status));
    }
  }

  // print the list
  DEBUG ((DEBUG_INFO, "PRINTING CURRENT SETTINGS XML - Start\n"));
  DebugPrintXmlTree (List, 0);
  DEBUG ((DEBUG_INFO, "PRINTING CURRENT SETTINGS XML - End\n"));

  // now output as xml string

  Status = XmlTreeToString (List, TRUE, StringSize, XmlString);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - XmlTreeToString failed.  %r\n", __FUNCTION__, Status));
  }

EXIT:
  if (List != NULL) {
    FreeXmlTree (&List);
  }

  if (EFI_ERROR (Status)) {
    // free memory since it was an error
    if (*XmlString != NULL) {
      FreePool (*XmlString);
      *XmlString  = NULL;
      *StringSize = 0;
    }
  }

  PERF_FUNCTION_END ();
  return Status;
}

/**
  PopulateCurrentSettingsIfNeeded

  Populate the current settings if they don't exist or if the settings have changed

  @param  NONE

  @retval  EFI_SUCCESS  - Settings not changed, or settings updated.
  @retval  other        - error occurred creating the settings XML
**/
EFI_STATUS
EFIAPI
PopulateCurrentSettingsIfNeeded (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       Len;
  UINTN       OldLen;
  CHAR8       *OldVar         = NULL;
  CHAR8       *OldVarCompare1 = NULL;
  CHAR8       *OldVarCompare2 = NULL;
  UINTN       OldVarSize      = 0;
  CHAR8       *Var            = NULL;
  CHAR8       *VarCompare1    = NULL;
  CHAR8       *VarCompare2    = NULL;
  UINTN       VarSize         = 0;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

  Status = GetVariable2 (
             DFCI_SETTINGS_CURRENT_OUTPUT_VAR_NAME,
             &gDfciSettingsManagerVarNamespace,
             (VOID **)&OldVar,
             &OldVarSize
             );

  if (EFI_ERROR (Status)) {
    OldVar     = NULL;
    OldVarSize = 0;
  } else {
    ASSERT (OldVar[OldVarSize - 1] == 0);            // Ensure null terminator is present
    OldVarCompare1 = AsciiStrStr (OldVar, "<Date>"); // Find the start of the date
    OldVarCompare2 = AsciiStrStr (OldVar, "/Date>"); // Find the end of the date
    ASSERT (OldVarCompare1 != NULL);                 // Ensure end of date found
    ASSERT (OldVarCompare2 != NULL);                 // Ensure end of date found
  }

  // Create string of Xml
  Status = CreateXmlStringFromCurrentSettings (&Var, &VarSize, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to create xml string from current %r\n", __FUNCTION__, Status));
    goto EXIT;
  }

  if (OldVarCompare1 != NULL) {
    ASSERT (Var[VarSize - 1] == 0);                    // Ensure null terminator is present
    VarCompare1 = AsciiStrStr (Var, "<Date>");         // Find the start of the date
    VarCompare2 = AsciiStrStr (Var, "/Date>");         // Find the end of the date
    ASSERT (VarCompare1 != NULL);                      // Ensure end of date found
    ASSERT (VarCompare2 != NULL);                      // Ensure end of date found

    Len    = VarCompare1 - Var;
    OldLen = OldVarCompare1 - OldVar;

    DEBUG ((DEBUG_INFO, "OldVar = %p, OldVarCompare1=%p, OldVarCompare2=%p, OldLen = %d\n", OldVar, OldVarCompare1, OldVarCompare2, OldLen));
    DEBUG ((DEBUG_INFO, "   Var = %p,    VarCompare1=%p,    VarCompare2=%p,    Len = %d\n", Var, VarCompare1, VarCompare2, Len));

    if ((Len == OldLen) &&
        (0 == CompareMem (Var, OldVar, Len)) &&               // Compare Xml up to <Date
        (0 == AsciiStrCmp (OldVarCompare2, VarCompare2)))     // Resume compare of Xml at /Date>
    //
    // Compare the settings, ignoring the date.  If none of the settings have changed,
    // skip updating the settings variables.
    //
    {
      DEBUG ((DEBUG_INFO, "%a - Settings not changed, skipping write of current settings\n", __FUNCTION__));
      goto EXIT;
    }
  }

  // Save variable
  Status = gRT->SetVariable (
                  DFCI_SETTINGS_CURRENT_OUTPUT_VAR_NAME,
                  &gDfciSettingsManagerVarNamespace,
                  DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES,
                  VarSize,
                  Var
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to write current setting Xml variable %r\n", __FUNCTION__, Status));
    goto EXIT;
  }

  // Success
  DEBUG ((DEBUG_INFO, "%a - Current Settings Xml Var Set with data size: 0x%X\n", __FUNCTION__, VarSize));

  //
  //
  // TEMP HACK Set V1 Current Settings
  //
  //

  FreePool (Var);
  Var = NULL;

  // Create V1 string of Xml
  Status = CreateXmlStringFromCurrentSettings (&Var, &VarSize, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - V1 Failed to create xml string from current %r\n", __FUNCTION__, Status));
    goto EXIT;
  }

  // Save variable
  Status = gRT->SetVariable (L"UEFISettingsCurrent", &gDfciSettingsManagerVarNamespace, DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES, VarSize, Var);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to write current setting Xml variable %r\n", __FUNCTION__, Status));
    goto EXIT;
  }

  // Success
  DEBUG ((DEBUG_INFO, "%a - V1 Current Settings Xml Var Set with data size: 0x%X\n", __FUNCTION__, VarSize));

EXIT:
  if (OldVar != NULL) {
    FreePool (OldVar);
    OldVar = NULL;
  }

  if (Var != NULL) {
    FreePool (Var);
    Var = NULL;
  }

  return Status;
}
