/** @file
DfciXmlPermissionSchemaSupport.c

This library supports the schema used for the Permissions Input, Current, and Result XML files.

Copyright (c) 2018, Microsoft Corporation

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <XmlTypes.h>
#include <Library/XmlTreeLib.h>
#include <Library/XmlTreeQueryLib.h>
#include <DfciSystemSettingTypes.h>
#include <Library/DfciXmlPermissionSchemaSupportLib.h>
#include <Library/BaseLib.h>
#include <Library/DfciV1SupportLib.h>

#define RESULT_XML_TEMPLATE "<?xml version=\"1.0\" encoding=\"utf-8\"?><ResultsPacket xmlns=\"urn:UefiSettings-Schema\"></ResultsPacket>"

// YYYY-MM-DDTHH:MM:SS
#define DATE_STRING_SIZE 20

#define CURRENT_XML_TEMPLATE "<?xml version=\"1.0\" encoding=\"utf-8\"?><CurrentPermissionsPacket xmlns=\"urn:UefiSettings-Schema\"></CurrentPermissionsPacket>"

/**
INTERNAL FUNCTION to covert decimal XML string to permission mask
**/
EFI_STATUS
EFIAPI
ConvertAsciiDecimalToPermissionMask(
  IN CONST CHAR8         *PermAscii, 
  OUT DFCI_PERMISSION_MASK *Mask);


// LIBRARY CLASS FUNCTIONS //
XmlNode*
EFIAPI
GetPermissionPacketNode(
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

  if (AsciiStrnCmp(RootNode->Name, PERMISSIONS_PACKET_ELEMENT_NAME, sizeof(PERMISSIONS_PACKET_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is not Permission Packet Element\n", __FUNCTION__));
    return NULL;
  }

  return (XmlNode*)RootNode;
}

XmlNode*
EFIAPI
GetCurrentPermissionsPacketNode(
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

  if (AsciiStrnCmp(RootNode->Name, CURRENT_PERMISSION_PACKET_ELEMENT_NAME, sizeof(CURRENT_PERMISSION_PACKET_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is not Current Permissions Packet Element\n", __FUNCTION__));
    return NULL;
  }
  // From root node there should only be 1 sibling which is the Packet Node
  return (XmlNode*)RootNode;
}

XmlNode*
EFIAPI
GetPermissionsListNodeFromPacketNode(
IN CONST XmlNode* PacketNode)
{
  ASSERT(PacketNode != NULL);
  return FindFirstChildNodeByName(PacketNode, PERMISSIONS_LIST_ELEMENT_NAME);

}

/**
 * Get Permissin attributes DefaultPMask and DefaultDMask
 *
 * Set the input values to theri default before calling this function.  If the
 * values are not defined in the XML, the PMask or DMask will not be disturbed.
 *
 * @param PermissionListNode
 * @param PMask
 * @param DMask
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
GetPermissionsListDefaultPMask(
IN CONST XmlNode      *PermissionListNode,
OUT DFCI_PERMISSION_MASK  *PMask,
OUT DFCI_PERMISSION_MASK  *DMask)
{
  XmlAttribute *Attr = NULL;
  EFI_STATUS    Status;

  //loop thru attributes and check for the default attribute being set to true
  //Check for a match on one
  if ((PermissionListNode == NULL) || (AsciiStrnCmp(PermissionListNode->Name, PERMISSIONS_LIST_ELEMENT_NAME, sizeof(PERMISSIONS_LIST_ELEMENT_NAME)) != 0))
  {
    DEBUG((DEBUG_ERROR, "%a - Permission List Node is not a Permission List Node\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  if (PMask == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  Attr = FindFirstAttributeByName(PermissionListNode, PERMISSIONS_LIST_DEFAULT_ATTRIBUTE_NAME);
  if(Attr == NULL)
  {
    DEBUG((DEBUG_INFO, "%a - Attribute Not Found\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  DEBUG((DEBUG_INFO, "%a - Attribute Found.  Value %a\n", __FUNCTION__, Attr->Value));
  Status = ConvertAsciiDecimalToPermissionMask(Attr->Value, PMask);
  if (!EFI_ERROR(Status))
  {
    Attr = FindFirstAttributeByName(PermissionListNode, PERMISSIONS_LIST_DELEGATED_ATTRIBUTE_NAME);
    if(Attr == NULL)
    {
      DEBUG((DEBUG_INFO, "%a - Default DefaultDMask Attribute Not Found\n", __FUNCTION__));
      return EFI_SUCCESS;
    }

    DEBUG((DEBUG_INFO, "%a - Default DefaultDMask Attribute Found.  Value %a\n", __FUNCTION__, Attr->Value));
    Status = ConvertAsciiDecimalToPermissionMask(Attr->Value, DMask);
  }
  return Status;
}

/**
Returns true if Permission Entries should be
appended to existing Permission List
**/
EFI_STATUS
EFIAPI
PermissionListEntriesAppend(
IN CONST XmlNode *PermissionListNode,
OUT BOOLEAN        *Result)
{
  XmlAttribute *Attr = NULL;
  //loop thru attributes and check for the append attribute being set to true
  //Check for a match on one
  if ((PermissionListNode == NULL) || (AsciiStrnCmp(PermissionListNode->Name, PERMISSIONS_LIST_ELEMENT_NAME, sizeof(PERMISSIONS_LIST_ELEMENT_NAME)) != 0))
  {
    DEBUG((DEBUG_ERROR, "%a - Permission List Node is not a Permission List Node\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  if (Result == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  Attr = FindFirstAttributeByName(PermissionListNode, PERMISSIONS_LIST_APPEND_ATTRIBUTE_NAME);
  if(Attr == NULL)
  {
    DEBUG((DEBUG_INFO, "%a - Attribute Not Found\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  DEBUG((DEBUG_INFO, "%a - Attribute Found.  Value %a\n", __FUNCTION__, Attr->Value));
  if (AsciiStrnCmp(Attr->Value, PERMISSIONS_LIST_APPEND_ATTRIBUTE_TRUE_VALUE, sizeof(PERMISSIONS_LIST_APPEND_ATTRIBUTE_TRUE_VALUE)) == 0)
  {
    *Result = TRUE;
  } 
  else
  {
    *Result = FALSE;
  }
  return EFI_SUCCESS;
}

/**
 * Get Input Permission
 *
 * Gets the PermissionMask and the DMask from the input XML
 *
 * The PermissionMask is required, and the DMask is optional. If
 * the DMask is not present, the return value is not distrubed.
 *
 *
 * @param ParentPermissionNode
 * @param Id
 * @param PMask
 * @param DMask
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
GetInputPermission(
IN CONST XmlNode* ParentPermissionNode,
OUT DFCI_SETTING_ID_STRING *Id,
OUT DFCI_PERMISSION_MASK *PMask,
OUT DFCI_PERMISSION_MASK *DMask)
{
  EFI_STATUS  Status;
  XmlNode    *Temp = NULL;
  DFCI_SETTING_ID_STRING IdTemp;

  //Given the parent node go get
  //the value of the Id node and the value
  //of the Permission Mask node
  if ((Id == NULL) || (PMask == NULL) || (DMask == NULL) || (ParentPermissionNode == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //Check for a match on one
  if ((ParentPermissionNode->ParentNode == NULL) || (AsciiStrnCmp(ParentPermissionNode->ParentNode->Name, PERMISSIONS_LIST_ELEMENT_NAME, sizeof(PERMISSIONS_LIST_ELEMENT_NAME)) != 0))
  {
    DEBUG((DEBUG_ERROR, "%a - Parent Permission Node is not a Permission Node\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  Temp = FindFirstChildNodeByName(ParentPermissionNode, PERMISSION_ID_ELEMENT_NAME);
  if (Temp == NULL)
  {
    DEBUG((DEBUG_INFO, "%a - Failed to find Id Element\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  // Translate all incoming packet data Id to internal string notation.
  *Id = Temp->Value;
  if ((Temp->Value[0] >= '0') && (Temp->Value[0] <= '9'))
  {
    IdTemp = DfciV1TranslateString (Temp->Value);
    if (IdTemp != NULL) {
        *Id = IdTemp;
    }
  }

  Temp = FindFirstChildNodeByName(ParentPermissionNode, PERMISSION_MASK_VALUE_ELEMENT_NAME);
  if (Temp == NULL)
  {
    DEBUG((DEBUG_INFO, "%a - Failed to find Permission Mask Element\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }
  Status = ConvertAsciiDecimalToPermissionMask(Temp->Value, PMask);

  *DMask = DFCI_IDENTITY_NOT_SPECIFIED;
  if (!EFI_ERROR(Status))
  {
    Temp = FindFirstChildNodeByName(ParentPermissionNode, PERMISSION_DELEGATED_MASK_VALUE_ELEMENT_NAME);
    if (Temp == NULL)
    {
      DEBUG((DEBUG_INFO, "%a - Failed to find DefaultDMask DefaultPMask Element\n", __FUNCTION__));
    } else {
      Status = ConvertAsciiDecimalToPermissionMask(Temp->Value, DMask);
    }
  }
  return Status;
}

XmlNode *
EFIAPI
New_CurrentPermissionsPacketNodeList(EFI_TIME *Date)
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
  Status = AddNode(Root, CURRENT_PERMISSION_DATE_ELEMENT_NAME, &DateString[0], &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to add node for date. %r\n", __FUNCTION__, Status));
    goto ERROR;
  }

  // Add PermissionsList Node
  Status = AddNode(Root, CURRENT_PERMISSION_LIST_ELEMENT_NAME, NULL, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to add node for Permissions. %r\n", __FUNCTION__, Status));
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

///// CURRENT PERMISSIONS

EFI_STATUS
EFIAPI
SetCurrentPermissions(
  IN CONST XmlNode *ParentPermissionsListNode,
  IN CONST CHAR8* Id,
  IN CONST UINT8  Value,
  IN CONST UINT8  DelegatedValue)
{
  XmlNode   *Temp = NULL;
  XmlNode   *Permission = NULL;
  EFI_STATUS Status;
  CHAR8      PermString[8];

  if ((ParentPermissionsListNode == NULL) || (Id == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (AsciiStrnCmp(ParentPermissionsListNode->Name, CURRENT_PERMISSION_LIST_ELEMENT_NAME, sizeof(CURRENT_PERMISSION_LIST_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - Parent Permission Node is not Permission Node List\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  //Make the <PermissionCurrent>
  Status = AddNode((XmlNode*)ParentPermissionsListNode, CURRENT_PERMISSION_ELEMENT_NAME, NULL, &Permission);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create PermissionCurrent node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  //Make the <Id>
  Status = AddNode((XmlNode*)Permission, CURRENT_PERMISSION_ID_ELEMENT_NAME, Id, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Id node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  //Make the <Value>
  Status = AsciiValueToStringS ( PermString,
                                 sizeof(PermString),
                                 0,
                                 Value,
                                 5);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to convert PMask to integer string %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  Status = AddNode((XmlNode*)Permission, CURRENT_PERMISSION_VALUE_ELEMENT_NAME, PermString, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Value node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  if (DelegatedValue != 0)
  {
    //Make <DMask>
    Status = AsciiValueToStringS ( PermString,
                                   sizeof(PermString),
                                   0,
                                   DelegatedValue,
                                   5);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to convert DMask to integer string %r\n", __FUNCTION__, Status));
      return EFI_DEVICE_ERROR;
    }

    Status = AddNode((XmlNode*)Permission, PERMISSION_DELEGATED_MASK_VALUE_ELEMENT_NAME, PermString, &Temp);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to create Value node %r\n", __FUNCTION__, Status));
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
AddPermissionsLsvNode(
  IN CONST XmlNode* CurrentPermissionsPacketNode,
  IN CONST CHAR8* Lsv)
{
  EFI_STATUS Status;
  if ((CurrentPermissionsPacketNode == NULL) || (Lsv == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }
  //Make sure its our expected node
  if (AsciiStrnCmp(CurrentPermissionsPacketNode->Name, CURRENT_PERMISSION_PACKET_ELEMENT_NAME, sizeof(CURRENT_PERMISSION_PACKET_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - CurrentPermissionsPacketNode is not Current Permissions Packet Element\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  //Add LSV Node
  Status = AddNode((XmlNode*)CurrentPermissionsPacketNode, CURRENT_PERMISSION_LSV_ELEMENT_NAME, Lsv, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Lsv node %r\n", __FUNCTION__, Status));
  }
  return Status;
}

/**
 * Add Attributes to CurrentPermissionPacket
 *
 * @param CurrentPermissionsPacketNode
 * @param PMask
 * @param DelegatedPMask
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
AddCurrentAttributes(
  IN CONST XmlNode *CurrentPermissionsPacketNode,
  IN CONST UINT8  PMask,
  IN CONST UINT8  DelegatedPMask) {

  EFI_STATUS Status;
  CHAR8      PermString[8];

  //Make the <PMask string value>
  Status = AsciiValueToStringS ( PermString,
                                 sizeof(PermString),
                                 0,
                                 PMask,
                                 5);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to convert PMask to integer string Code=%r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  Status = AddAttributeToNode((XmlNode *) CurrentPermissionsPacketNode,
                              PERMISSIONS_LIST_DEFAULT_ATTRIBUTE_NAME,
                              PermString);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to add attributes to Current Attributes. Code=%r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  //Make the <DelegatedPMask string value>
  Status = AsciiValueToStringS ( PermString,
                                 sizeof(PermString),
                                 0,
                                 DelegatedPMask,
                                 5);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to convert DelegatedPMask to integer string Code=%r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  Status = AddAttributeToNode((XmlNode *) CurrentPermissionsPacketNode,
                              PERMISSIONS_LIST_DELEGATED_ATTRIBUTE_NAME,
                              PermString);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to add DelegatePMask to Current Attributes. Code=%r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  return Status;
}

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
  <Permissions />
</ResultsPacket>

**/
XmlNode *
EFIAPI
New_ResultPermissionPacketNodeList(EFI_TIME *Date )
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

  // Add PermissionsList Node
  Status = AddNode(Root, RESULTS_PERMISSIONS_LIST_ELEMENT_NAME, NULL, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to add node for Permissions. %r\n", __FUNCTION__, Status));
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

XmlNode*
EFIAPI
GetResultsPermissionPacketNode(
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
    DEBUG((DEBUG_ERROR, "%a - RootNode is not Result Permissions Packet Element\n", __FUNCTION__));
    return NULL;
  }
  // From root node there should only be 1 sibling which is the Packet Node
  return (XmlNode*)RootNode;
}

/**
Function to Create the XML nodes for a single permission output status

@param[in] ParentPermissionsListNode:  The <Permissions> element node that all <ResultPermissions> are under
@param[in] Id:        String for the Id
@param[in] Result:    String of the status code value for the operation

@retval Success if created and added to the xml successfully
@retval Error if it could not be created or added to the xml
**/
EFI_STATUS
EFIAPI
SetOutputPermissionStatus(
  IN CONST XmlNode* ParentPermissionsListNode,
  IN CONST CHAR8* Id,
  IN CONST CHAR8* Result
  )
{
  XmlNode *Temp = NULL;
  XmlNode *Permission = NULL;
  EFI_STATUS Status;

  if ((ParentPermissionsListNode == NULL) || (Id == NULL) || (Result == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (AsciiStrnCmp(ParentPermissionsListNode->Name, RESULTS_PERMISSIONS_LIST_ELEMENT_NAME, sizeof(RESULTS_PERMISSIONS_LIST_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - Parent Permission Node is not Permission Node List\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  //Make the <PermissionResult>
  Status = AddNode((XmlNode*)ParentPermissionsListNode, RESULTS_PERMISSIONS_ELEMENT_NAME, NULL, &Permission);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create PermissionResult node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  //Make the <Id>
  Status = AddNode((XmlNode*)Permission, RESULTS_PERMISSIONS_ID_ELEMENT_NAME, (CHAR8*)Id, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Id node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  //Make the <PermissionId>
  Status = AddNode((XmlNode*)Permission, RESULTS_PERMISSIONS_STATUS_ELEMENT_NAME, (CHAR8*)Result, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Result node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

////// _ INTERNAL FUNCTIONS - ///////
EFI_STATUS
EFIAPI
ConvertAsciiDecimalToPermissionMask(
  IN CONST CHAR8          *PermAscii, 
  OUT DFCI_PERMISSION_MASK  *Mask)
{
  UINTN t = 0;
  if ((PermAscii == NULL) || (Mask == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }
  t = AsciiStrDecimalToUintn(PermAscii);
  if (t > (UINTN)DFCI_PERMISSION_MASK__ALL)
  {
    DEBUG((DEBUG_ERROR, "%a - Invalid Mask %a\n", __FUNCTION__, PermAscii));
    return EFI_INVALID_PARAMETER;
  }


  *Mask = (DFCI_PERMISSION_MASK)t;
  return EFI_SUCCESS;
}

