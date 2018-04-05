/** @file
XmlSettingSchemaSupport

This library supports the schema used for the Settings Input and Result XML files. 

Copyright (c) 2015, Microsoft Corporation. 

**/
#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <XmlTypes.h>
#include <Library/XmlTreeLib.h>
#include <Library/XmlTreeQueryLib.h>
#include <DfciSystemSettingTypes.h>
#include <Library/DfciXmlSettingSchemaSupportLib.h>
#include <Library/BaseLib.h>





#define PERMISSIONS_PACKET_ELEMENT_NAME  "PermissionsPacket"
#define PERMISSIONS_VERSION_ELEMENT_NAME "Version"
#define PERMISSIONS_LSV_ELEMENT_NAME     "LowestSupportedVersion"
#define PERMISSIONS_LIST_ELEMENT_NAME    "Permissions"
#define PERMISSIONS_LIST_DEFAULT_ATTRIBUTE_NAME "Default"
#define PERMISSIONS_LIST_APPEND_ATTRIBUTE_NAME  "Append"
#define PERMISSIONS_LIST_APPEND_ATTRIBUTE_TRUE_VALUE  "True"
#define PERMISSION_ELEMENT_NAME          "Permission"
#define PERMISSION_ID_ELEMENT_NAME       "Id"
#define PERMISSION_MASK_VALUE_ELEMENT_NAME    "PMask"

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
GetPermissionsListNodeFromPacketNode(
IN CONST XmlNode* PacketNode)
{
  ASSERT(PacketNode != NULL);
  return FindFirstChildNodeByName(PacketNode, PERMISSIONS_LIST_ELEMENT_NAME);

}

EFI_STATUS
EFIAPI
GetPermissionsListDefaultPMask(
IN CONST XmlNode      *PermissionListNode,
OUT DFCI_PERMISSION_MASK  *PMask)
{
  XmlAttribute *Attr = NULL;
  //loop thru attributes and check for the append attribute being set to true
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
  return ConvertAsciiDecimalToPermissionMask(Attr->Value, PMask);
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

EFI_STATUS
EFIAPI
GetInputPermission(
IN CONST XmlNode* ParentPermissionNode,
OUT DFCI_SETTING_ID_STRING *Id,
OUT DFCI_PERMISSION_MASK *PMask)
{
  XmlNode  *Temp = NULL;

  //Given the parent node go get
  //the value of the Id node and the value
  //of the Permission Mask node
  if ((Id == NULL) || (PMask == NULL) || (ParentPermissionNode == NULL))
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

  *Id = Temp->Value;

  Temp = FindFirstChildNodeByName(ParentPermissionNode, PERMISSION_MASK_VALUE_ELEMENT_NAME);
  if (Temp == NULL)
  {
    DEBUG((DEBUG_INFO, "%a - Failed to find Permission Mask Element\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }
  return ConvertAsciiDecimalToPermissionMask(Temp->Value, PMask);
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

