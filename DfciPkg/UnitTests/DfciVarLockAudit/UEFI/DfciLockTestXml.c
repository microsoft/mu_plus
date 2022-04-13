/** @file
  Support using XML as the File format for var report data

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

  **/

#include "DfciLockTestXml.h"

#define LIST_XML_TEMPLATE   "<?xml version=\"1.0\" encoding=\"utf-8\"?><Variables></Variables>"
#define READY_XML_TEMPLATE  "<ReadyToBoot></ReadyToBoot>"

#define MAX_STRING_LENGTH  (0x10000)

#define DATA_TO_BIG  ("DATA AS STRING EXCEEDS MAX LENGTH")

// Helper functions

/**
Creates a new XmlNode list following the List
format.

Return NULL if error occurs. Otherwise return a pointer to xml doc root element
of a Variables.

List must be freed using FreeXmlTree

Result will contain
LIST Xml Template

**/
XmlNode *
EFIAPI
New_VariablesNodeList (
  )
{
  EFI_STATUS  Status;
  XmlNode     *Root = NULL;

  Status = CreateXmlTree (LIST_XML_TEMPLATE, sizeof (LIST_XML_TEMPLATE) - 1, &Root);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR;
  }

  // Return the root
  return Root;

ERROR:
  if (Root) {
    FreeXmlTree (&Root);
    // root will be NULL and null returned
  }

  return Root;
}

/**
If success return pointers populated with a copy of the data

**/
EFI_STATUS
EFIAPI
GetNameGuidMembersFromNode (
  IN CONST  XmlNode   *Node,
  OUT       CHAR16    **VarName,
  OUT       EFI_GUID  *VarGuid
  )
{
  EFI_STATUS  Status;
  LIST_ENTRY  *Link = NULL;
  UINTN       i, Length;

  if (VarName == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - VarName is NULL\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }

  *VarName = NULL;

  if (Node == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Node is NULL\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }

  if (VarGuid == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - VarGuid is NULL\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }

  if (AsciiStrnCmp (DFCI_ENTRY_ELEMENT_NAME, Node->Name, sizeof (DFCI_ENTRY_ELEMENT_NAME)) == 0) {
    DEBUG ((DEBUG_ERROR, "%a - Skipping Dfci Entry\n", __FUNCTION__));
    Status = EFI_UNSUPPORTED;
    goto EXIT;
  }

  if (AsciiStrnCmp (VARIABLE_ENTRY_ELEMENT_NAME, Node->Name, sizeof (VARIABLE_ENTRY_ELEMENT_NAME)) != 0) {
    DEBUG ((DEBUG_ERROR, "%a - Node is Not a Variable Node.  Element Name = %a \n", __FUNCTION__, Node->Name));
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }

  if (Node->NumAttributes < 2) {
    DEBUG ((DEBUG_ERROR, "%a - Node not in valid state for this function (too few attributes). \n", __FUNCTION__));
    DebugPrintXmlTree (Node, 0);
    Status = EFI_VOLUME_CORRUPTED;
    goto EXIT;
  }

  Link = Node->AttributesListHead.ForwardLink;
  for (i = 0; i < 2; i++, Link = Link->ForwardLink) {
    XmlAttribute  *CurrentAttribute = (XmlAttribute *)Link;
    //
    // TODO:  this code should change to do string compare on attribute name.
    // Assuming order is not safe.
    //
    switch (i) {
      case 0: // Name
        Length   = AsciiStrnLenS (CurrentAttribute->Value, 1024);
        *VarName = (CHAR16 *)AllocateZeroPool ((Length + 1) * sizeof (CHAR16));
        if (*VarName == NULL) {
          DEBUG ((DEBUG_ERROR, "%a Failed to allocate for VarName\n", __FUNCTION__));
          Status = EFI_OUT_OF_RESOURCES;
          goto EXIT;
        }

        Status = AsciiStrToUnicodeStrS (CurrentAttribute->Value, *VarName, Length + 1);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "Error converting Ascii to Unicode for %a, Length=%d, code=%r\n", CurrentAttribute->Value, Length, Status));
        }

        break;

      case 1: // guid
        Status = ConvertAsciiStringToGuid (CurrentAttribute->Value, VarGuid);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a Failed to convert ascii string to guid. %r\n", __FUNCTION__, Status));
          goto EXIT;
        }

        break;

      default:
        DEBUG ((DEBUG_ERROR, "%a logic error: should never hit the default case\n", __FUNCTION__));
        Status = EFI_DEVICE_ERROR;
        goto EXIT;
    }
  }

  Status = EFI_SUCCESS;

EXIT:
  if (EFI_ERROR (Status)) {
    if ((VarName != NULL) && (*VarName != NULL)) {
      FreePool (*VarName);
      *VarName = NULL;
    }
  }

  return Status;
}

/**
Creates a new XmlNode for a var and adds it to the list

Return NULL if error occurs. Otherwise return a pointer Variable Node.

List must be freed using FreeXmlTree

return pointer will be the variable element node
VAR_XML_TEMPLATE

**/
XmlNode *
EFIAPI
New_VariableNodeInList (
  IN CONST XmlNode  *RootNode,
  IN CONST CHAR16   *VarName,
  IN CONST GUID     *VarGuid,
  IN UINT32         Attributes,
  IN UINTN          DataSize,
  IN CONST UINT8    *Data
  )
{
  XmlNode     *NewVarNode  = NULL;
  XmlNode     *TempNode    = NULL;
  CHAR8       *AsciiString = NULL;
  EFI_STATUS  Status;
  UINTN       i;

  AsciiString = AllocatePages (EFI_SIZE_TO_PAGES (MAX_STRING_LENGTH));  // allocate 64kb
  if (AsciiString == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to allocate 64kb for string conversion\n", __FUNCTION__));
    return NULL;
  }

  // 1 - confirm good root node
  if (RootNode == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - RootNode is NULL\n", __FUNCTION__));
    return NULL;
  }

  if (RootNode->XmlDeclaration.Declaration == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - RootNode is not the root node\n", __FUNCTION__));
    ASSERT (RootNode->XmlDeclaration.Declaration != NULL);
    return NULL;
  }

  if (AsciiStrnCmp (RootNode->Name, LIST_ELEMENT_NAME, sizeof (LIST_ELEMENT_NAME)) != 0) {
    DEBUG ((DEBUG_ERROR, "%a - RootNode is not Variable List\n", __FUNCTION__));
    return NULL;
  }

  // RootNode is good.

  // Create the var node with no parent
  Status = AddNode (NULL, VARIABLE_ENTRY_ELEMENT_NAME, NULL, &NewVarNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - AddNode for Var Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  // Create the name attribute
  UnicodeStrToAsciiStrS (VarName, AsciiString, MAX_STRING_LENGTH);
  Status = AddAttributeToNode (NewVarNode, VAR_NAME_ATTRIBUTE_NAME, AsciiString);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - AddAttribute for Name Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  // Create the guid attribute
  *AsciiString = '\0';
  AsciiSPrint (AsciiString, MAX_STRING_LENGTH, "%g", VarGuid);
  Status = AddAttributeToNode (NewVarNode, VAR_GUID_ATTRIBUTE_NAME, AsciiString);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - AddNode for Guid Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  // Create the attributes element
  // Create the name element
  AsciiString[0] = '0';
  AsciiString[1] = 'x';
  AsciiString[2] = '\0';
  AsciiValueToStringS (AsciiString + 2, MAX_STRING_LENGTH - 2, (RADIX_HEX), (INT64)Attributes, 30);

  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) == EFI_VARIABLE_NON_VOLATILE) {
    AsciiStrCatS (AsciiString, MAX_STRING_LENGTH, " NV");
    Attributes ^= EFI_VARIABLE_NON_VOLATILE;
  }

  if ((Attributes & EFI_VARIABLE_BOOTSERVICE_ACCESS) == EFI_VARIABLE_BOOTSERVICE_ACCESS) {
    AsciiStrCatS (AsciiString, MAX_STRING_LENGTH, " BS");
    Attributes ^= EFI_VARIABLE_BOOTSERVICE_ACCESS;
  }

  if ((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == EFI_VARIABLE_RUNTIME_ACCESS) {
    AsciiStrCatS (AsciiString, MAX_STRING_LENGTH, " RT");
    Attributes ^= EFI_VARIABLE_RUNTIME_ACCESS;
  }

  if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    AsciiStrCatS (AsciiString, MAX_STRING_LENGTH, " HW-Error");
    Attributes ^= EFI_VARIABLE_HARDWARE_ERROR_RECORD;
  }

  if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) {
    AsciiStrCatS (AsciiString, MAX_STRING_LENGTH, " Auth-WA");
    Attributes ^= EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;
  }

  if ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) {
    AsciiStrCatS (AsciiString, MAX_STRING_LENGTH, " Auth-TIME-WA");
    Attributes ^= EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
  }

  if ((Attributes & EFI_VARIABLE_APPEND_WRITE) == EFI_VARIABLE_APPEND_WRITE) {
    AsciiStrCatS (AsciiString, MAX_STRING_LENGTH, " APPEND-W");
    Attributes ^= EFI_VARIABLE_APPEND_WRITE;
  }

  // Show ?? if attributes contained bit set of unknown type
  if (Attributes != 0) {
    AsciiStrCatS (AsciiString, MAX_STRING_LENGTH, " ?????");
  }

  Status = AddNode (NewVarNode, VAR_ATTRIBUTES_ELEMENT_NAME, AsciiString, &TempNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - AddNode for Attributes Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  // Create the size element
  *AsciiString = '\0';
  AsciiValueToStringS (AsciiString, MAX_STRING_LENGTH, 0, (INT64)DataSize, 30);
  Status = AddNode (NewVarNode, VAR_SIZE_ELEMENT_NAME, AsciiString, &TempNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - AddNode for DataSize Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  *AsciiString = '\0';
  if (DataSize * 2 < MAX_STRING_LENGTH) {
    // convert the data into hex bytes
    for (i = 0; i < DataSize; i++) {
      AsciiValueToStringS ((AsciiString + (i * 2)), MAX_STRING_LENGTH - (i * 2), (RADIX_HEX | PREFIX_ZERO), (INT64)Data[i], 2);
    }

    Status = AddNode (NewVarNode, VAR_DATA_ELEMENT_NAME, AsciiString, &TempNode);
  } else {
    DEBUG ((DEBUG_INFO, "%a - Data Size Too Large for String conversion 0x%X\n", __FUNCTION__, DataSize * 2));
    Status = AddNode (NewVarNode, VAR_DATA_ELEMENT_NAME, DATA_TO_BIG, &TempNode);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - AddNode for Data Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  // Add the NewVarNode to the end of the RootNode children
  Status = AddChildTree ((XmlNode *)RootNode, NewVarNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Can't add new var to list.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  return NewVarNode;

ERROR_EXIT:
  if (NewVarNode != NULL) {
    FreeXmlTree (&NewVarNode);
  }

  if (AsciiString != NULL) {
    FreePages (AsciiString, EFI_SIZE_TO_PAGES (MAX_STRING_LENGTH));
  }

  return NewVarNode;
}

XmlNode *
EFIAPI
New_DfciStatusNodeInList (
  IN CONST XmlNode  *RootNode
  )
{
  XmlNode     *NewVarNode = NULL;
  EFI_STATUS  Status;

  // 1 - confirm good root node
  if (RootNode == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - RootNode is NULL\n", __FUNCTION__));
    return NULL;
  }

  if (RootNode->XmlDeclaration.Declaration == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - RootNode is not the root node\n", __FUNCTION__));
    ASSERT (RootNode->XmlDeclaration.Declaration != NULL);
    return NULL;
  }

  if (AsciiStrnCmp (RootNode->Name, LIST_ELEMENT_NAME, sizeof (LIST_ELEMENT_NAME)) != 0) {
    DEBUG ((DEBUG_ERROR, "%a - RootNode is not Variable List\n", __FUNCTION__));
    return NULL;
  }

  // RootNode is good.

  // Create the var node with no parent
  Status = AddNode (NULL, DFCI_ENTRY_ELEMENT_NAME, NULL, &NewVarNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - AddNode for Var Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  // Add the NewVarNode to the end of the RootNode children
  Status = AddChildTree ((XmlNode *)RootNode, NewVarNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Can't add new var to list.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  return NewVarNode;

ERROR_EXIT:
  if (NewVarNode != NULL) {
    FreeXmlTree (&NewVarNode);
  }

  return NewVarNode;
}

EFI_STATUS
EFIAPI
AddReadyToBootStatusToNode (
  IN CONST XmlNode  *Node,
  IN EFI_STATUS     ReadStatus,
  IN EFI_STATUS     WriteStatus
  )
{
  XmlNode     *StatusNode = NULL;
  EFI_STATUS  Status;
  CHAR8       AsciiString[100];// hold the ascii for UINT64 converted to string

  Status = CreateXmlTree (READY_XML_TEMPLATE, sizeof (READY_XML_TEMPLATE) - 1, &StatusNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed.  Status %r\n", __FUNCTION__, Status));
    return Status;
  }

  // Create the Read Status element
  AsciiSPrint (AsciiString, 100, "0x%lx %r", ReadStatus, ReadStatus);
  Status = AddNode (StatusNode, VAR_READ_STATUS_ELEMENT_NAME, AsciiString, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - AddNode for ReadStatus Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  AsciiSPrint (AsciiString, 100, "0x%lx %r", WriteStatus, WriteStatus);
  Status = AddNode (StatusNode, VAR_WRITE_STATUS_ELEMENT_NAME, AsciiString, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - AddNode for Write Status Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  // Add the StatusNode to the end of the Node
  Status = AddChildTree ((XmlNode *)Node, StatusNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Can't add status node to var node.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  return EFI_SUCCESS;

ERROR_EXIT:
  if (StatusNode != NULL) {
    FreeXmlTree (&StatusNode);
  }

  return Status;
}

EFI_STATUS
EFIAPI
AddDfciErrorToNode (
  IN XmlNode  *StatusNode,
  IN CHAR8    *DfciStatusString
  )
{
  EFI_STATUS  Status;

  Status = AddNode (StatusNode, VAR_DFCI_CHECK_ELEMENT_NAME, DfciStatusString, NULL);
  return Status;
}
