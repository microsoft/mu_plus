/** @file
  This application will locate all DFCI variables and acquire their status for DFCI.

  Copyright (C) Microsoft Corporation. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  **/

#ifndef DFCI_LOCK_TEST_XML_H
#define DFCI_LOCK_TEST_XML_H

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <XmlTypes.h>
#include <Library/XmlTreeLib.h>
#include <Library/XmlTreeQueryLib.h>
#include <Library/ShellLib.h>

/**
<Variables>
  <Variable Name="" Guid="">
    <Attributes></Attributes>
    <Size></Size>
    <Data></Data>
    <ReadyToBoot>
      <ReadStatus></ReadStatus>
      <WriteStatus></WriteStatus>
    </ReadyToBoot>

  </Variable>
...
  <DfciStatus></DfciStatus>
</Variables>
**/
#define LIST_ELEMENT_NAME              "Variables"
#define VARIABLE_ENTRY_ELEMENT_NAME    "Variable"
#define VAR_NAME_ATTRIBUTE_NAME        "Name"
#define VAR_GUID_ATTRIBUTE_NAME        "Guid"
#define VAR_ATTRIBUTES_ELEMENT_NAME    "Attributes"
#define VAR_SIZE_ELEMENT_NAME          "Size"
#define VAR_DATA_ELEMENT_NAME          "Data"
#define VAR_READ_STATUS_ELEMENT_NAME   "ReadStatus"
#define VAR_WRITE_STATUS_ELEMENT_NAME  "WriteStatus"
#define DFCI_ENTRY_ELEMENT_NAME        "DfciStatus"
#define VAR_DFCI_CHECK_ELEMENT_NAME    "DfciError"

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
  );

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
  );

EFI_STATUS
EFIAPI
AddReadyToBootStatusToNode (
  IN CONST XmlNode  *Node,
  IN EFI_STATUS     ReadStatus,
  IN EFI_STATUS     WriteStatus
  );

EFI_STATUS
EFIAPI
GetNameGuidMembersFromNode (
  IN CONST  XmlNode   *Node,
  OUT       CHAR16    **VarName,
  OUT       EFI_GUID  *VarGuid
  );

EFI_STATUS
EFIAPI
ConvertAsciiStringToGuid (
  IN CONST CHAR8   *StringGuid,
  IN OUT EFI_GUID  *Guid
  );

XmlNode *
EFIAPI
New_DfciStatusNodeInList (
  IN CONST XmlNode  *RootNode
  );

EFI_STATUS
EFIAPI
AddDfciErrorToNode (
  IN XmlNode  *Node,
  IN CHAR8    *DfciMessage
  );

#endif
