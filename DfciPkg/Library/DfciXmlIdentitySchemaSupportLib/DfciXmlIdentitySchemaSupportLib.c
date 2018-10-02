/** @file
DfciXmlIdentitySchemaSupportLib.c

This library supports the schema used for the the current Identity UEFIDeviceId XML content.

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
#include <Library/DfciXmlIdentitySchemaSupportLib.h>
#include <Library/BaseLib.h>

// YYYY-MM-DDTHH:MM:SS
#define DATE_STRING_SIZE 20

#define IDENTITY_CURRENT_XML_TEMPLATE "<?xml version=\"1.0\" encoding=\"utf-8\"?><UEFIIdentityCurrentPacket></UEFIIdentityCurrentPacket>"

// LIBRARY CLASS FUNCTIONS //
XmlNode*
EFIAPI
GetIdentityCurrentPacketNode(
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

  if (AsciiStrnCmp(RootNode->Name, IDENTITY_CURRENT_PACKET_ELEMENT_NAME, sizeof(IDENTITY_CURRENT_PACKET_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is not an Identity Current Packet Element\n", __FUNCTION__));
    return NULL;
  }

  return (XmlNode*)RootNode;
}

XmlNode*
EFIAPI
GetIdentityCurrentListNodeFromPacketNode(
IN CONST XmlNode* PacketNode)
{
  ASSERT(PacketNode != NULL);
  return FindFirstChildNodeByName(PacketNode, IDENTITY_CURRENT_LIST_ELEMENT_NAME);

}

XmlNode *
EFIAPI
New_IdentityCurrentPacketNodeList(VOID)
{
  EFI_STATUS Status;
  XmlNode *Root = NULL;
  XmlNode *Temp = NULL;


  Status = CreateXmlTree(IDENTITY_CURRENT_XML_TEMPLATE, sizeof(IDENTITY_CURRENT_XML_TEMPLATE) - 1, &Root);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR;
  }

  // Add IdentifiersList Node
  Status = AddNode(Root, IDENTITY_CURRENT_LIST_ELEMENT_NAME, NULL, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to add node for Certificates. %r\n", __FUNCTION__, Status));
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
 * Add the current Identity Version
 *
 * @param IdentityCurrentPacketNode
 * @param Version
 */
EFI_STATUS
EFIAPI
AddVersionNode(
  IN CONST XmlNode *IdentityCurrentPacketNode,
  IN CONST CHAR8   *Version)
{
  EFI_STATUS Status;
  if ((IdentityCurrentPacketNode == NULL) || (Version == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }
  //Make sure its our expected node
  if (AsciiStrnCmp(IdentityCurrentPacketNode->Name, IDENTITY_CURRENT_PACKET_ELEMENT_NAME, sizeof(IDENTITY_CURRENT_PACKET_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - IdentityCurrentPacketPacketNode is not the Identity current packet Element\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  //Add Dfci Version Node
  Status = AddNode((XmlNode*)IdentityCurrentPacketNode, IDENTITY_CURRENT_VERSION_ELEMENT_NAME, Version, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Version node %r\n", __FUNCTION__, Status));
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
SetIdentityCurrentCertificate (
  IN CONST XmlNode *ParentCertificatesListNode,
  IN CONST CHAR8   *Signer,
  IN CONST CHAR8   *Value) {

  XmlNode   *Temp = NULL;
  XmlNode   *Certificate = NULL;
  EFI_STATUS Status;

  if ((ParentCertificatesListNode == NULL) || (Signer == NULL) || (Value == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (AsciiStrnCmp(ParentCertificatesListNode->Name, IDENTITY_CURRENT_LIST_ELEMENT_NAME, sizeof(IDENTITY_CURRENT_LIST_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - Parent Identifier Node is not an Identity Node List\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  //Make the <Certificate>
  Status = AddNode((XmlNode*)ParentCertificatesListNode, IDENTITY_CURRENT_ELEMENT_NAME, NULL, &Certificate);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Identity node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  //Make the <Id>
  Status = AddNode((XmlNode*)Certificate, IDENTITY_CURRENT_ID_ELEMENT_NAME, Signer, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Id node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  Status = AddNode((XmlNode*)Certificate, IDENTITY_CURRENT_VALUE_ELEMENT_NAME, Value, &Temp);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create Value node %r\n", __FUNCTION__, Status));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


