/** @file
  This application will locate all variables and aquire their status as deletable.

  Copyright (c) 2016, Microsoft Corporation

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

#ifndef LOCK_TEST_XML_H
#define LOCK_TEST_XML_H

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
    <OsRuntime>
      <ReadStatus></ReadStatus>
      <WriteStatus></WriteStatus>
    </OsRuntime>
  </Variable>
...
</Variables>
**/
#define LIST_ELEMENT_NAME               "Variables"
#define VARIABLE_ENTRY_ELEMENT_NAME       "Variable"
#define VAR_NAME_ATTRIBUTE_NAME               "Name"
#define VAR_GUID_ATTRIBUTE_NAME               "Guid"
#define VAR_ATTRIBUTES_ELEMENT_NAME         "Attributes"
#define VAR_SIZE_ELEMENT_NAME               "Size"
#define VAR_DATA_ELEMENT_NAME               "Data"
#define VAR_READYTOBOOT_ELEMENT_NAME        "ReadyToBoot"
#define VAR_OSRUNTIME_ELEMENT_NAME          "OsRuntime"
#define VAR_READ_STATUS_ELEMENT_NAME          "ReadStatus"
#define VAR_WRITE_STATUS_ELEMENT_NAME         "WriteStatus"



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
New_VariablesNodeList();


/**
Creates a new XmlNode for a var and adds it to the list

Return NULL if error occurs. Otherwise return a pointer Variable Node.

List must be freed using FreeXmlTree

return pointer will be the variable element node
VAR_XML_TEMPLATE

**/
XmlNode *
EFIAPI
New_VariableNodeInList(
  IN CONST XmlNode  *RootNode,
  IN CONST CHAR16* VarName,
  IN CONST GUID*   VarGuid,
  IN UINT32 Attributes,
  IN UINTN DataSize,
  IN CONST UINT8* Data
);

EFI_STATUS
EFIAPI
AddReadyToBootStatusToNode(
  IN CONST XmlNode *Node,
  IN EFI_STATUS ReadStatus,
  IN EFI_STATUS WriteStatus
);


EFI_STATUS
EFIAPI
GetNameGuidMembersFromNode(
  IN CONST  XmlNode   *Node,
  OUT       CHAR16      **VarName,
  OUT       EFI_GUID    *VarGuid
);

EFI_STATUS
EFIAPI
ConvertAsciiStringToGuid(
  IN CONST CHAR8 *StringGuid,
  IN OUT EFI_GUID *Guid
);

#endif