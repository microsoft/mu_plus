/** @file
DfciXmlDeviceIdSchemaSupportLib.c

This library supports the schema used for the UEFIDeviceId XML content.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <XmlTypes.h>
#include <Library/XmlTreeLib.h>
#include <Library/XmlTreeQueryLib.h>
#include <DfciSystemSettingTypes.h>
#include <Library/DfciXmlDeviceIdSchemaSupportLib.h>
#include <Library/BaseLib.h>

// YYYY-MM-DDTHH:MM:SS
#define DATE_STRING_SIZE 20

#define DEVICE_ID_XML_TEMPLATE "<?xml version=\"1.0\" encoding=\"utf-8\"?><UEFIDeviceIdentifierPacket></UEFIDeviceIdentifierPacket>"

// LIBRARY CLASS FUNCTIONS //
XmlNode*
EFIAPI
GetDeviceIdPacketNode(
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

  if (AsciiStrnCmp(RootNode->Name, DEVICE_ID_PACKET_ELEMENT_NAME, sizeof(DEVICE_ID_PACKET_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is not Device Id Packet Element\n", __FUNCTION__));
    return NULL;
  }

  return (XmlNode*)RootNode;
}

XmlNode*
EFIAPI
GetDeviceIdListNodeFromPacketNode(
IN CONST XmlNode* PacketNode)
{
  ASSERT(PacketNode != NULL);
  return FindFirstChildNodeByName(PacketNode, DEVICE_ID_LIST_ELEMENT_NAME);

}

XmlNode *
EFIAPI
New_DeviceIdPacketNodeList(VOID)
{
  EFI_STATUS Status;
  XmlNode *Root = NULL;
  XmlNode *Temp = NULL;


  Status = CreateXmlTree(DEVICE_ID_XML_TEMPLATE, sizeof(DEVICE_ID_XML_TEMPLATE) - 1, &Root);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR;
  }

  // Add IdentifiersList Node
  Status = AddNode(Root, DEVICE_ID_LIST_ELEMENT_NAME, NULL, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to add node for Identifiers. %r\n", __FUNCTION__, Status));
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
 * Add the current DFCI Version
 *
 * @param IdPacketNode
 * @param DfciVersion
 */
EFI_STATUS
EFIAPI
AddDfciVersionNode(
  IN CONST XmlNode *IdPacketNode,
  IN CONST CHAR8   *DfciVersion)
{
  EFI_STATUS Status;
  if ((IdPacketNode == NULL) || (DfciVersion == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }
  //Make sure its our expected node
  if (AsciiStrnCmp(IdPacketNode->Name, DEVICE_ID_PACKET_ELEMENT_NAME, sizeof(DEVICE_ID_PACKET_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - IdPacketNode is not Id Packet Element\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  //Add Dfci Version Node
  Status = AddNode((XmlNode*)IdPacketNode, DEVICE_ID_DFCI_VERSION_ELEMENT_NAME, DfciVersion, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Dfci Version node %r\n", __FUNCTION__, Status));
  }
  return Status;
}

/**
 * Set Device Id Element
 *
 *
 * @param ParentIdentifiersListNode
 * @param Id
 * @param Value
 *
 * @return EFI_STATUS EFIAPI
 */

EFI_STATUS
EFIAPI
SetDeviceIdIdentifier (
  IN CONST XmlNode *ParentIdentifiersListNode,
  IN CONST CHAR8 *Id,
  IN CONST CHAR8 *Value) {

  XmlNode   *Temp = NULL;
  XmlNode   *Identifier = NULL;
  EFI_STATUS Status;

  if ((ParentIdentifiersListNode == NULL) || (Id == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (AsciiStrnCmp(ParentIdentifiersListNode->Name, DEVICE_ID_LIST_ELEMENT_NAME, sizeof(DEVICE_ID_LIST_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - Parent Identifier Node is not an Identifiers Node List\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  //Make the <Identifier>
  Status = AddNode((XmlNode*)ParentIdentifiersListNode, DEVICE_ID_ELEMENT_NAME, NULL, &Identifier);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Identifier node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  //Make the <Id>
  Status = AddNode((XmlNode*)Identifier, DEVICE_ID_ID_ELEMENT_NAME, Id, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Id node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  Status = AddNode((XmlNode*)Identifier, DEVICE_ID_VALUE_ELEMENT_NAME, Value, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Value node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


