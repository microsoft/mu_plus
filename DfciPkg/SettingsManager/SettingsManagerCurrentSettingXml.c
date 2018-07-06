
#include "SettingsManager.h"
#include <XmlTypes.h>
#include <Library/XmlTreeLib.h>
#include <Library/XmlTreeQueryLib.h>
#include <Library/DfciV1SupportLib.h>
#include <Library/DfciXmlSettingSchemaSupportLib.h>
#include <Library/PrintLib.h>
#include <Guid/DfciSettingsManagerVariables.h>

/**
Clear the cached Current Settings string 
so the next boot it will be repopulated.
**/
VOID
EFIAPI
ClearCacheOfCurrentSettings()
{
  EFI_STATUS Status;
  Status = gRT->SetVariable(XML_SETTINGS_CURRENT_OUTPUT_VAR_NAME, &gDfciSettingsManagerVarNamespace, 0, 0, NULL);
  DEBUG((DEBUG_INFO, "Delete Current Xml Settings %r\n", Status));
}


/**
Create an XML string from all the current settings

**/
EFI_STATUS
EFIAPI
CreateXmlStringFromCurrentSettings(
  OUT CHAR8** XmlString,
  OUT UINTN*  StringSize
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
    return Status;
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
    DFCI_SETTING_PROVIDER_LIST_ENTRY *Prov = CR(Link, DFCI_SETTING_PROVIDER_LIST_ENTRY, Link, DFCI_SETTING_PROVIDER_LIST_ENTRY_SIGNATURE);
    Value = ProviderValueAsAscii(&(Prov->Provider), TRUE);

// TEMP Set XML to Number string for compatibility
    {
        DFCI_SETTING_ID_STRING NumberString;
        NumberString = DfciV1NumberFromId (Prov->Provider.Id);
        Status = SetCurrentSettings(CurrentSettingsListNode, NumberString, Value);
    }
//     Status = SetCurrentSettings(CurrentSettingsListNode, Prov->Provider.Id, Value);

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
  return Status;
}

EFI_STATUS
EFIAPI
PopulateCurrentSettingsIfNeeded()
{
  EFI_STATUS Status;
  CHAR8* Var = NULL;
  UINTN  VarSize = 0;
  Status = GetVariable2(XML_SETTINGS_CURRENT_OUTPUT_VAR_NAME,
    &gDfciSettingsManagerVarNamespace,
    &Var,
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
  Status = CreateXmlStringFromCurrentSettings(&Var, &VarSize);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create xml string from current %r\n", __FUNCTION__, Status));
    goto EXIT;
  }

  //Save variable
  Status = gRT->SetVariable(XML_SETTINGS_CURRENT_OUTPUT_VAR_NAME, &gDfciSettingsManagerVarNamespace, DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES, VarSize, Var);
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

