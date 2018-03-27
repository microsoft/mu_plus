/** @file 
Audit test code to collect tpm event log for offline processing or validation

Copyright (c) 2017, Microsoft Corporation

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


#include "TpmEventLogXml.h"

#define LIST_XML_TEMPLATE "<?xml version=\"1.0\" encoding=\"utf-8\"?><Events></Events>"
#define DIGEST_XML_TEMPLATE "<Digests></Digests>"

#define MAX_STRING_LENGTH (0xFFFF)


//Helper functions
 
 
/**
Creates a new XmlNode list following the List
format.

Return NULL if error occurs. Otherwise return a pointer to xml doc root element
of a Events.

List must be freed using FreeNodeList

Result will contain
LIST Xml Template

**/
XmlNode *
EFIAPI
New_EventsNodeList()
{
  EFI_STATUS Status;
  XmlNode *Root = NULL;

  Status = CreateXmlTree(LIST_XML_TEMPLATE, sizeof(LIST_XML_TEMPLATE) - 1, &Root);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed.  Status %r\n", __FUNCTION__, Status));
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


#define NUM_OF_PAGES (16)
/**
Creates a new XmlNode for an event and adds it to the list

Return NULL if error occurs. Otherwise return a pointer Event Node.

List must be freed using FreeNodeList

return pointer will be the Event element node
EVENT_XML_TEMPLATE

**/
XmlNode *
EFIAPI
New_NodeInList(
  IN CONST XmlNode            *RootNode,
  IN UINTN                    PcrIndex,
  IN UINTN                    EventType,
  IN UINTN                    EventSize,
  IN UINT8                    *EventBuffer,
  IN UINTN                    DigestCount,
  IN TPML_DIGEST_VALUES       *Digest)
{
  XmlNode*                  NewEventNode = NULL;
  XmlNode*                  TempNode = NULL;
  XmlNode*                  DigestNode = NULL;
  CHAR8                     *AsciiString = NULL;
  EFI_STATUS                Status; 
  UINTN                     i;
  UINTN                     DigestIndex;
  TPMI_ALG_HASH             HashAlgo;
  UINTN                     DigestSize;
  UINT8                     *DigestBuffer;

  AsciiString = AllocatePages(NUM_OF_PAGES);  //allocate 64kb
  if (AsciiString == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to allocate 64kb for string conversion\n", __FUNCTION__));
    return NULL;
  }

  //1 - confirm good root node
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

  if (AsciiStrnCmp(RootNode->Name, LIST_ELEMENT_NAME, sizeof(LIST_ELEMENT_NAME)) != 0) 
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is not Event List\n", __FUNCTION__));
    return NULL;
  }

  //RootNode is good. 

  if (DigestCount > 0) 
  {
  //Create the event node with no parent
  Status = AddNode(NULL, EVENT_ENTRY_ELEMENT_NAME, NULL, &NewEventNode);
  }
  else
  {
    //Create the header node with no parent
    Status = AddNode(NULL, HEADER_ENTRY_ELEMENT_NAME, NULL, &NewEventNode);
  }

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddNode for Event Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  //Create the PcrIndex element
  *AsciiString = '\0';
  AsciiValueToString(AsciiString, 0, (INT64)PcrIndex, 30); 
  Status = AddNode(NewEventNode, EVENT_PCR_ELEMENT_NAME, AsciiString, &TempNode);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddNode for PcrIndex Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  //Create the EventType element
  *AsciiString = '\0';
  AsciiValueToString(AsciiString, 0, (INT64)EventType, 30); 
  Status = AddNode(NewEventNode, EVENT_TYPE_ELEMENT_NAME, AsciiString, &TempNode);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddNode for EventType Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  //Create the EventSize element
  *AsciiString = '\0';
  AsciiValueToString(AsciiString, 0, (INT64)EventSize, 30); 
  Status = AddNode(NewEventNode, EVENT_SIZE_ELEMENT_NAME, AsciiString, &TempNode);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddNode for EventSize Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  *AsciiString = '\0';
  //convert the data into hex bytes
  if (EventSize * 2 > MAX_STRING_LENGTH)
  {
    DEBUG((DEBUG_ERROR, "%a - Data Size Too Large for String conversion 0x%X\n", __FUNCTION__, EventSize * 2));
    goto ERROR_EXIT;
  }

  for (i = 0; i < EventSize; i++)
  {
     AsciiValueToString((AsciiString + (i * 2)), (RADIX_HEX | PREFIX_ZERO), (INT64)EventBuffer[i], 2); 
  }
    
  Status = AddNode(NewEventNode, EVENT_DATA_ELEMENT_NAME, AsciiString, &TempNode);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddNode for EventData Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  if (DigestCount > 0)
  {
    //Create the Event DigestCount element
    *AsciiString = '\0';
    AsciiValueToString(AsciiString, 0, (INT64)DigestCount, 30); 
    Status = AddNode(NewEventNode, EVENT_DIGEST_COUNT_ELEMENT_NAME, AsciiString, &TempNode);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - AddNode for DigestCount Failed.  Status %r\n", __FUNCTION__, Status));
      goto ERROR_EXIT;
    }

    Status = CreateXmlTree(DIGEST_XML_TEMPLATE, sizeof(DIGEST_XML_TEMPLATE) - 1, &DigestNode);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed.  Status %r\n", __FUNCTION__, Status));
      goto ERROR_EXIT;
    }

    HashAlgo = Digest->digests[0].hashAlg;
    DigestBuffer = (UINT8 *)Digest->digests[0].digest.sha1;
    for (DigestIndex = 0; DigestIndex < DigestCount; DigestIndex++) {
      DigestSize = GetHashSizeFromAlgo (HashAlgo);
      *AsciiString = '\0';
      //convert the data into hex bytes
      if (DigestSize * 2 > MAX_STRING_LENGTH)
      {
        DEBUG((DEBUG_ERROR, "%a - Data Size Too Large for String conversion 0x%X\n", __FUNCTION__, DigestSize * 2));
        goto ERROR_EXIT;
      }

      for (i = 0; i < DigestSize; i++)
      {
         AsciiValueToString((AsciiString + (i * 2)), (RADIX_HEX | PREFIX_ZERO), (INT64)DigestBuffer[i], 2); 
      }

      Status = AddNode(DigestNode, EVENT_DIGEST_ELEMENT_NAME, AsciiString, &TempNode);
      if (EFI_ERROR(Status))
      {
        DEBUG((DEBUG_ERROR, "%a - AddNode for Digest Failed.  Status %r\n", __FUNCTION__, Status));
        goto ERROR_EXIT;
      }

      //Add the HashAlgo attribute to Digest node
      *AsciiString = '\0';
      AsciiSPrint(AsciiString, MAX_STRING_LENGTH + 1, "%d", HashAlgo);
      Status = AddAttributeToNode(TempNode, EVENT_HASH_ALGO_ATTRIBUTE_NAME, AsciiString);
      if (EFI_ERROR(Status))
      {
        DEBUG((DEBUG_ERROR, "%a - AddNodeAttribute for HashAlgo Failed.  Status %r\n", __FUNCTION__, Status));
        goto ERROR_EXIT;
      }

      //
      // Prepare next
      //
      CopyMem (&HashAlgo, DigestBuffer + DigestSize, sizeof(TPMI_ALG_HASH));
      DigestBuffer = DigestBuffer + DigestSize + sizeof(TPMI_ALG_HASH);
    }

    //Add the DigestNode List to the end of the NewEventNode
    Status = AddChildTree((XmlNode*)NewEventNode, DigestNode);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Can't add DigestNode list to NewEventNode.  Status %r\n", __FUNCTION__, Status));
      goto ERROR_EXIT;
    }
  }
  else {
    DEBUG((DEBUG_INFO, "Header node\n"));
  }

  //Add the NewEventNode to the end of the RootNode children
  Status = AddChildTree((XmlNode*)RootNode, NewEventNode);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Can't add new event to list.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  return NewEventNode;

ERROR_EXIT:
  if (NewEventNode != NULL)  { FreeXmlTree(&NewEventNode);  }
  if (TempNode != NULL)      { FreeXmlTree(&TempNode);  }
  if (DigestNode != NULL)    { FreeXmlTree(&DigestNode);  }
  if (AsciiString != NULL)   { FreePages(AsciiString, NUM_OF_PAGES); }

  return NULL;
}

