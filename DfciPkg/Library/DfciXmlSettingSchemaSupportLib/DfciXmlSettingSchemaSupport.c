/** @file
DfciXmlSettingSchemaSupport.c

This library supports the schema used for the Settings Input and Result XML files.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <DfciSystemSettingTypes.h>
#include <XmlTypes.h>

#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/XmlTreeLib.h>
#include <Library/XmlTreeQueryLib.h>
#include <Library/DfciV1SupportLib.h>
#include <Library/DfciXmlSettingSchemaSupportLib.h>
#include <Library/BaseLib.h>

#define RESULT_XML_TEMPLATE "<?xml version=\"1.0\" encoding=\"utf-8\"?><ResultsPacket xmlns=\"urn:UefiSettings-Schema\"></ResultsPacket>"

// YYYY-MM-DDTHH:MM:SS
#define DATE_STRING_SIZE 20 

#define CURRENT_XML_TEMPLATE "<?xml version=\"1.0\" encoding=\"utf-8\"?><CurrentSettingsPacket xmlns=\"urn:UefiSettings-Schema\"></CurrentSettingsPacket>"

/**
Creates a new XmlNode list following the ResultPacket
format.

Return NULL if error occurs. Otherwise return a pointer to xml doc root element
of a ResultPacketNodeList.

List must be freed using FreeXmlTree

Result will contain
<?XML ****?>
<ResultsPacket>
  <AppliedOn>Datetime</AppliedOn>
  <Settings />
</ResultsPacket>

**/
XmlNode *
EFIAPI
New_ResultPacketNodeList(EFI_TIME *Date )
{
  EFI_STATUS Status; 
  XmlNode *Root = NULL;
  XmlNode *Temp = NULL;
  CHAR8  DateString[DATE_STRING_SIZE];
  
  
  Status = CreateXmlTree(RESULT_XML_TEMPLATE, sizeof(RESULT_XML_TEMPLATE) - 1, &Root);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR;
  }
  
  // Add AppliedOn Node
  AsciiSPrint(&DateString[0], DATE_STRING_SIZE, "%d-%02d-%02dT%02d:%02d:%02d", Date->Year, Date->Month, Date->Day, Date->Hour, Date->Minute, Date->Second);
  Status = AddNode(Root, RESULTS_APPLIED_ON_ELEMENT_NAME, &DateString[0], &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to add node for applied date. %r\n", __FUNCTION__, Status));
    goto ERROR;
  }

  // Add SettingsList Node
  Status = AddNode(Root, RESULTS_SETTINGS_LIST_ELEMENT_NAME, NULL, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to add node for Settings. %r\n", __FUNCTION__, Status));
    goto ERROR;
  }

  //Return the root
  return Root;

ERROR:
  if (Root)
  {
    FreeXmlTree(&Root);
    //root will be NULL and null returned
  }
  return Root;
}

/**
Funtion to get the SettingsPacket node given the root node

**/
XmlNode*
EFIAPI
GetSettingsPacketNode(
  IN CONST XmlNode* RootNode)
{
  if (RootNode == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is NULL\n", __FUNCTION__));
    return NULL;
  }

  if (RootNode->XmlDeclaration.Declaration == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is not the root node\n", __FUNCTION__));
    ASSERT(RootNode->XmlDeclaration.Declaration != NULL);
    return NULL;
  }

  if (AsciiStrnCmp(RootNode->Name, SETTINGS_PACKET_ELEMENT_NAME, sizeof(SETTINGS_PACKET_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is not Settings Packet Element\n", __FUNCTION__));
    return NULL;
  }

  return (XmlNode*)RootNode;
}

XmlNode*
EFIAPI
GetResultsPacketNode(
  IN CONST XmlNode* RootNode)
{
  if (RootNode == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is NULL\n", __FUNCTION__));
    return NULL;
  }

  if (RootNode->XmlDeclaration.Declaration == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is not the root node\n", __FUNCTION__));
    ASSERT(RootNode->XmlDeclaration.Declaration != NULL);
    return NULL;
  }

  if (AsciiStrnCmp(RootNode->Name, RESULTS_PACKET_ELEMENT_NAME, sizeof(RESULTS_PACKET_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is not Result Settings Packet Element\n", __FUNCTION__));
    return NULL;
  }
  // From root node there should only be 1 sibling which is the Packet Node
  return (XmlNode*)RootNode;
}

XmlNode*
EFIAPI
GetSettingsListNodeFromPacketNode(
  IN CONST XmlNode* PacketNode)
{
  ASSERT(PacketNode != NULL);
  return FindFirstChildNodeByName(PacketNode, SETTINGS_LIST_ELEMENT_NAME);
}

/**
Function to get the Id and Value Strings from the Xml for a single setting 
Ptrs will be updated to point to their strings in the XML memory
Don't free the output as its part of the XML node structure and will be cleaned up
once the XML list is freed. 

@param[in] ParentSettingNode:  The <Setting> element node to get Id and Value for
@param[out] Id:       String ptr that will be updated to point to the Id String
@param[out] Value:    String of the status code value for the operation

@retval Success if both Id and Value are updated correctly
@retval Error if both Id and Value could not be updated to point to correct strings.
**/
EFI_STATUS
EFIAPI
GetInputSettings(
  IN CONST XmlNode* ParentSettingNode,
  OUT CONST CHAR8** Id,
  OUT CONST CHAR8** Value)
{
  XmlNode  *Temp = NULL;
  //Given the parent node go get
  //the value of the Id node and the value
  //of the Value node
  if ((Id == NULL) || (Value == NULL) || (ParentSettingNode == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //Check for a match on one
  if ((ParentSettingNode->ParentNode == NULL) || (AsciiStrnCmp(ParentSettingNode->ParentNode->Name, SETTINGS_LIST_ELEMENT_NAME, sizeof(SETTINGS_LIST_ELEMENT_NAME)) != 0))
  {
    DEBUG((DEBUG_ERROR, "%a - Parent Setting Node is not a Setting Node\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  Temp = FindFirstChildNodeByName(ParentSettingNode, SETTING_ID_ELEMENT_NAME);
  if (Temp == NULL)
  {
    DEBUG((DEBUG_INFO, "%a - Failed to find Id Element\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }

//  Disable translating settings response to strings.
//  if ((Temp->Value[0] >= '0') && (Temp->Value[0] <= '9'))
//  {
//      *Id = DfciV1TranslateString (Temp->Value);
//  } else {
      *Id = Temp->Value;
//  }
  Temp = FindFirstChildNodeByName(ParentSettingNode, SETTING_VALUE_ELEMENT_NAME);
  if (Temp == NULL)
  {
    DEBUG((DEBUG_INFO, "%a - Failed to find Value Element\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }
  *Value = Temp->Value;
  return EFI_SUCCESS;
}


/**
Function to Create the XML nodes for a single setting output status

@param[in] ParentSettingsListNode:  The <Settings> element node that all <ResultSettings> are under
@param[in] Id:        String for the Id
@param[in] Result:    String of the status code value for the operation
@param[in,opt] Flags: optional String of the return flags

@retval Success if created and added to the xml successfully
@retval Error if it could not be created or added to the xml
**/
EFI_STATUS
EFIAPI
SetOutputSettingsStatus(
  IN CONST XmlNode* ParentSettingsListNode,
  IN CONST CHAR8* Id,
  IN CONST CHAR8* Result,
  IN CONST CHAR8* Flags  OPTIONAL
  )
{
  XmlNode *Temp = NULL;
  XmlNode *Setting = NULL;
  EFI_STATUS Status;

  if ((ParentSettingsListNode == NULL) || (Id == NULL) || (Result == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (AsciiStrnCmp(ParentSettingsListNode->Name, RESULTS_SETTINGS_LIST_ELEMENT_NAME, sizeof(RESULTS_SETTINGS_LIST_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - Parent Setting Node is not Setting Node List\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  //Make the <SettingResult>
  Status = AddNode((XmlNode*)ParentSettingsListNode, RESULTS_SETTING_ELEMENT_NAME, NULL, &Setting);  
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create SettingResult node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  //Make the <Id>
  Status = AddNode((XmlNode*)Setting, RESULTS_SETTING_ID_ELEMENT_NAME, (CHAR8*)Id, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Id node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  //Make the <Flags>
  if (Flags != NULL)
  {
    Status = AddNode((XmlNode*)Setting, RESULTS_SETTING_FLAG_ELEMENT_NAME, (CHAR8*)Flags, &Temp);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to create Flags node %r\n", __FUNCTION__, Status));
      return EFI_DEVICE_ERROR;
    }
  }

  //Make the <Id>
  Status = AddNode((XmlNode*)Setting, RESULTS_SETTING_STATUS_ELEMENT_NAME, (CHAR8*)Result, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Result node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


///// CURRENT SETTINGS

XmlNode*
EFIAPI
GetCurrentSettingsPacketNode(
  IN CONST XmlNode* RootNode)
{
  if (RootNode == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is NULL\n", __FUNCTION__));
    return NULL;
  }

  if (RootNode->XmlDeclaration.Declaration == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is not the root node\n", __FUNCTION__));
    ASSERT(RootNode->XmlDeclaration.Declaration != NULL);
    return NULL;
  }

  if (AsciiStrnCmp(RootNode->Name, CURRENT_PACKET_ELEMENT_NAME, sizeof(CURRENT_PACKET_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is not Current Settings Packet Element\n", __FUNCTION__));
    return NULL;
  }
  // From root node there should only be 1 sibling which is the Packet Node
  return (XmlNode*)RootNode;
}



EFI_STATUS
EFIAPI
SetCurrentSettings(
  IN CONST XmlNode *ParentSettingsListNode,
  IN CONST CHAR8* Id,
  IN CONST CHAR8* Value)
{
  XmlNode *Temp = NULL;
  XmlNode *Setting = NULL;
  EFI_STATUS Status;

  if ((ParentSettingsListNode == NULL) || (Id == NULL) || (Value == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (AsciiStrnCmp(ParentSettingsListNode->Name, CURRENT_SETTINGS_LIST_ELEMENT_NAME, sizeof(CURRENT_SETTINGS_LIST_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - Parent Setting Node is not Setting Node List\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  //Make the <SettingCurrent>
  Status = AddNode((XmlNode*)ParentSettingsListNode, CURRENT_SETTING_ELEMENT_NAME, NULL, &Setting);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create SettingCurrent node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  //Make the <Id>
  Status = AddNode((XmlNode*)Setting, CURRENT_SETTING_ID_ELEMENT_NAME, Id, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Id node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  //Make the <Value>
  if (Value != NULL)
  {
    Status = AddNode((XmlNode*)Setting, CURRENT_SETTING_VALUE_ELEMENT_NAME, Value, &Temp);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to create Value node %r\n", __FUNCTION__, Status));
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

XmlNode *
EFIAPI
New_CurrentSettingsPacketNodeList(EFI_TIME *Date)
{
  EFI_STATUS Status;
  XmlNode *Root = NULL;
  XmlNode *Temp = NULL;
  CHAR8  DateString[DATE_STRING_SIZE];


  Status = CreateXmlTree(CURRENT_XML_TEMPLATE, sizeof(CURRENT_XML_TEMPLATE) - 1, &Root);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR;
  }

  // Add current date Node
  AsciiSPrint(&DateString[0], DATE_STRING_SIZE, "%d-%02d-%02dT%02d:%02d:%02d", Date->Year, Date->Month, Date->Day, Date->Hour, Date->Minute, Date->Second);
  Status = AddNode(Root, CURRENT_DATE_ELEMENT_NAME, &DateString[0], &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to add node for date. %r\n", __FUNCTION__, Status));
    goto ERROR;
  }

  // Add SettingsList Node
  Status = AddNode(Root, CURRENT_SETTINGS_LIST_ELEMENT_NAME, NULL, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to add node for Settings. %r\n", __FUNCTION__, Status));
    goto ERROR;
  }

  //Return the root
  return Root;

ERROR:
  if (Root)
  {
    FreeXmlTree(&Root);
    //root will be NULL and null returned
  }
  return Root;
}

EFI_STATUS
EFIAPI
AddSettingsLsvNode(
  IN CONST XmlNode* CurrentSettingsPacketNode, 
  IN CONST CHAR8* Lsv)
{
  EFI_STATUS Status;
  if ((CurrentSettingsPacketNode == NULL) || (Lsv == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }
  //Make sure its our expected node
  if (AsciiStrnCmp(CurrentSettingsPacketNode->Name, CURRENT_PACKET_ELEMENT_NAME, sizeof(CURRENT_PACKET_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - CurrentSettingsPacketNode is not Current Settings Packet Element\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  //Add LSV Node
  Status = AddNode((XmlNode*)CurrentSettingsPacketNode, CURRENT_LSV_ELEMENT_NAME, Lsv, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Lsv node %r\n", __FUNCTION__, Status));
  }
  return Status;
}
