/**
/** @file -- TpmEventLogAudit.c
Audit test code to collect tpm event log for offline processing or validation
File provides common XML output format for tpm event log.

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

#ifndef TPM_EVENT_LOG_XML_H
#define TPM_EVENT_LOG_XML_H

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
#include <Library/Tpm2CommandLib.h>
#include <IndustryStandard/UefiTcgPlatform.h>

/**
<Events>
  <HeaderEvent>  
    <PcrIndex></PcrIndex>
    <EventType></EventType>
    <EventSize></EventSize>
    <EventData></EventData>
  </HeaderEvent>
  <Event>
    <PcrIndex></PcrIndex>
    <EventType></EventType>
    <EventSize></EventSize>  
    <EventData></EventData>
    <DigestCount></DigestCount>
    <Digests>
      <Digest HashAlgo=""></Digest>
      ...
    </Digests>
  </Event>
  <Event>  
    ...  
  </Event>
...
</Events>
**/

#define LIST_ELEMENT_NAME                   "Events"
#define HEADER_ENTRY_ELEMENT_NAME           "HeaderEvent"
#define EVENT_ENTRY_ELEMENT_NAME            "Event"
#define EVENT_PCR_ELEMENT_NAME              "PcrIndex"
#define EVENT_TYPE_ELEMENT_NAME             "EventType"
#define EVENT_SIZE_ELEMENT_NAME             "EventSize"
#define EVENT_DATA_ELEMENT_NAME             "EventData"
#define EVENT_DIGEST_COUNT_ELEMENT_NAME     "DigestCount"
#define EVENT_DIGESTS_ELEMENT_NAME          "Digests"
#define EVENT_HASH_ALGO_ATTRIBUTE_NAME      "HashAlgo"
#define EVENT_DIGEST_ELEMENT_NAME           "Digest"


/**
Creates a new XmlNode list following the List
format.

Return NULL if error occurs. Otherwise return a pointer to xml doc root element
of Events.

List must be freed using FreeNodeList

Result will contain
LIST Xml Template

**/
XmlNode *
EFIAPI
New_EventsNodeList();

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
  IN TPML_DIGEST_VALUES       *Digest
);

#endif  // TPM_EVENT_LOG_XML_H