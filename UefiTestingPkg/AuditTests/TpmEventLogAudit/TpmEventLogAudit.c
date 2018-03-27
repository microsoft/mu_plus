/** @file -- TpmEventLogAudit.c
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

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Protocol/Tcg2Protocol.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include "TpmEventLogXml.h"


/**
  Add Header Event to XML List

  @param[in]  EventHdr     TCG PCR event structure.
**/
EFI_STATUS
AddHeaderEvent (
  IN XmlNode                   *RootNode,
  IN TCG_PCR_EVENT_HDR         *EventHdr
  )
{
  if (New_NodeInList(RootNode, (UINTN)EventHdr->PCRIndex, (UINTN)EventHdr->EventType, EventHdr->EventSize, (UINT8 *)(EventHdr + 1), 0, NULL) == NULL) 
  {
    DEBUG((DEBUG_ERROR, "Failed to create new Header Node.  Event Type: %s PcrIndex: %g\n", EventHdr->EventType, EventHdr->PCRIndex));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


/**
  This function get size of TCG_EfiSpecIDEventStruct.
  NOTE: Copied from Tcg2Dxe driver in UDK.

  @param[in]  TcgEfiSpecIdEventStruct     A pointer to TCG_EfiSpecIDEventStruct.
**/
UINTN
GetTcgEfiSpecIdEventStructSize (
  IN TCG_EfiSpecIDEventStruct   *TcgEfiSpecIdEventStruct
  )
{
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  UINT8                            *VendorInfoSize;
  UINT32                           NumberOfAlgorithms;

  CopyMem (&NumberOfAlgorithms, TcgEfiSpecIdEventStruct + 1, sizeof(NumberOfAlgorithms));

  DigestSize = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct + sizeof(*TcgEfiSpecIdEventStruct) + sizeof(NumberOfAlgorithms));
  VendorInfoSize = (UINT8 *)&DigestSize[NumberOfAlgorithms];
  return sizeof(TCG_EfiSpecIDEventStruct) + sizeof(UINT32) + (NumberOfAlgorithms * sizeof(TCG_EfiSpecIdEventAlgorithmSize)) + sizeof(UINT8) + (*VendorInfoSize);
}


/**
  Add Event to Xml List

  @param[in]  TcgPcrEvent2     TCG PCR event 2 structure.
**/
EFI_STATUS
AddEvent (
  IN XmlNode               *RootNode,
  IN TCG_PCR_EVENT2        *TcgPcrEvent2
  )
{
  UINT32                    DigestIndex;
  UINT32                    DigestCount;
  TPMI_ALG_HASH             HashAlgo;
  UINT32                    DigestSize;
  UINT8                     *DigestBuffer;
  UINT32                    EventSize;
  UINT8                     *EventBuffer;

  DigestCount = TcgPcrEvent2->Digest.count;
  HashAlgo = TcgPcrEvent2->Digest.digests[0].hashAlg;
  DigestBuffer = (UINT8 *)&TcgPcrEvent2->Digest.digests[0].digest;
  for (DigestIndex = 0; DigestIndex < DigestCount; DigestIndex++) {
    DigestSize = GetHashSizeFromAlgo (HashAlgo);
    //
    // Prepare next
    //
    CopyMem (&HashAlgo, DigestBuffer + DigestSize, sizeof(TPMI_ALG_HASH));
    DigestBuffer = DigestBuffer + DigestSize + sizeof(TPMI_ALG_HASH);
  }
  DigestBuffer = DigestBuffer - sizeof(TPMI_ALG_HASH);

  CopyMem (&EventSize, DigestBuffer, sizeof(TcgPcrEvent2->EventSize));
  EventBuffer = DigestBuffer + sizeof(TcgPcrEvent2->EventSize);

  if (New_NodeInList(RootNode, (UINTN)TcgPcrEvent2->PCRIndex, (UINTN)TcgPcrEvent2->EventType, EventSize, EventBuffer, TcgPcrEvent2->Digest.count, &TcgPcrEvent2->Digest) == NULL) 
  {
    DEBUG((DEBUG_ERROR, "Failed to create new Event Node.  Event Type: %s PcrIndex: %g\n", TcgPcrEvent2->EventType, TcgPcrEvent2->PCRIndex));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


/**
  This function returns size of TCG PCR event 2.
  NOTE: Copied from Tcg2Dxe driver in UDK.
  
  @param[in]  TcgPcrEvent2     TCG PCR event 2 structure.

  @return size of TCG PCR event 2.
**/
UINTN
GetPcrEvent2Size (
  IN TCG_PCR_EVENT2        *TcgPcrEvent2
  )
{
  UINT32                    DigestIndex;
  UINT32                    DigestCount;
  TPMI_ALG_HASH             HashAlgo;
  UINT32                    DigestSize;
  UINT8                     *DigestBuffer;
  UINT32                    EventSize;
  UINT8                     *EventBuffer;

  DigestCount = TcgPcrEvent2->Digest.count;
  HashAlgo = TcgPcrEvent2->Digest.digests[0].hashAlg;
  DigestBuffer = (UINT8 *)&TcgPcrEvent2->Digest.digests[0].digest;
  for (DigestIndex = 0; DigestIndex < DigestCount; DigestIndex++) {
    DigestSize = GetHashSizeFromAlgo (HashAlgo);
    //
    // Prepare next
    //
    CopyMem (&HashAlgo, DigestBuffer + DigestSize, sizeof(TPMI_ALG_HASH));
    DigestBuffer = DigestBuffer + DigestSize + sizeof(TPMI_ALG_HASH);
  }
  DigestBuffer = DigestBuffer - sizeof(TPMI_ALG_HASH);

  CopyMem (&EventSize, DigestBuffer, sizeof(TcgPcrEvent2->EventSize));
  EventBuffer = DigestBuffer + sizeof(TcgPcrEvent2->EventSize);

  return (UINTN)EventBuffer + EventSize - (UINTN)TcgPcrEvent2;
}


/**
  This function dump event log.
  NOTE: Copied from Tcg2Dxe driver in UDK.

  @param[in]  EventLogFormat     The type of the event log for which the information is requested.
  @param[in]  EventLogLocation   A pointer to the memory address of the event log.
  @param[in]  EventLogLastEntry  If the Event Log contains more than one entry, this is a pointer to the
                                 address of the start of the last entry in the event log in memory.
  @param[in]  FinalEventsTable   A pointer to the memory address of the final event table.
**/
EFI_STATUS
DumpEventLog (
  IN EFI_TCG2_EVENT_LOG_FORMAT   EventLogFormat,
  IN EFI_PHYSICAL_ADDRESS        EventLogLocation,
  IN EFI_PHYSICAL_ADDRESS        EventLogLastEntry,
  IN EFI_TCG2_FINAL_EVENTS_TABLE *FinalEventsTable
  )
{
  TCG_PCR_EVENT_HDR         *EventHdr;
  TCG_PCR_EVENT2            *TcgPcrEvent2;
  TCG_EfiSpecIDEventStruct  *TcgEfiSpecIdEventStruct;
  UINTN                     NumberOfEvents;
  XmlNode                   *List;
  CHAR16                    LogFileName[] = L"TpmEventLogAudit_manifest.xml";
  SHELL_FILE_HANDLE         FileHandle;
  UINTN                     StringSize = 0;
  CHAR8*                    XmlString = NULL;
  EFI_STATUS                Status;

  switch (EventLogFormat) {
  case EFI_TCG2_EVENT_LOG_FORMAT_TCG_2:
    List = New_EventsNodeList();
    if (List == NULL)
    {
      Status = EFI_DEVICE_ERROR;
      DEBUG((DEBUG_ERROR, "Failed to allocate an XML list\n"));
      goto Exit;
    }

    EventHdr = (TCG_PCR_EVENT_HDR *)(UINTN)EventLogLocation;
    Status = AddHeaderEvent (List, EventHdr);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "AddHeaderEvent failed.  %r\n", Status));
      goto Exit;
    }

    TcgEfiSpecIdEventStruct = (TCG_EfiSpecIDEventStruct *)(EventHdr + 1);
    TcgPcrEvent2 = (TCG_PCR_EVENT2 *)((UINTN)TcgEfiSpecIdEventStruct + GetTcgEfiSpecIdEventStructSize (TcgEfiSpecIdEventStruct));
    while ((UINTN)TcgPcrEvent2 <= EventLogLastEntry) {
      Status = AddEvent (List, TcgPcrEvent2);
      if (EFI_ERROR(Status))
      {
        DEBUG((DEBUG_ERROR, "AddEvent failed.  %r\n", Status));
        goto Exit;
      }
      TcgPcrEvent2 = (TCG_PCR_EVENT2 *)((UINTN)TcgPcrEvent2 + GetPcrEvent2Size(TcgPcrEvent2));
    }

    if (FinalEventsTable == NULL) {
      DEBUG((DEBUG_ERROR, "FinalEventsTable: NOT FOUND.\n"));
    } else {
      TcgPcrEvent2 = (TCG_PCR_EVENT2 *)(UINTN)(FinalEventsTable + 1);
      for (NumberOfEvents = 0; NumberOfEvents < FinalEventsTable->NumberOfEvents; NumberOfEvents++) {
        Status = AddEvent (List, TcgPcrEvent2);
        if (EFI_ERROR(Status))
        {
          DEBUG((DEBUG_ERROR, "AddEvent failed.  %r\n", Status));
          goto Exit;
        }
        TcgPcrEvent2 = (TCG_PCR_EVENT2 *)((UINTN)TcgPcrEvent2 + GetPcrEvent2Size (TcgPcrEvent2));
      }
    }

    //Write XML
    Status = XmlTreeToString(List, FALSE, &StringSize, &XmlString);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "XmlNodeListToString failed.  %r\n", Status));
      goto Exit;
    }

    if (StringSize == 0)
    {
      Status = EFI_OUT_OF_RESOURCES;
      DEBUG((DEBUG_ERROR, "StringSize equal 0.\n"));
      goto Exit;
    }

    //
    // subtract 1 from string size to avoid writing the NULL terminator
    //
    StringSize--;

    Status = ShellOpenFileByName(LogFileName, &FileHandle, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "Failed to open %s file for create. Status = %r\n", LogFileName, Status));
      goto Exit;
    }
    else
    {
      // Workaround start - delete the file if it exists and then reopen it to fix an issue where file data may be corrupted at the end
      ShellDeleteFile(&FileHandle); 
      Status = ShellOpenFileByName(LogFileName, &FileHandle, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);
      if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Failed to open %s file for create. Status = %r\n", LogFileName, Status));
        goto Exit;
      }
      // Workaround end

      ShellPrintEx(-1, -1, L"Writing XML to file %s\n", LogFileName);
      ShellWriteFile(FileHandle, &StringSize, XmlString);
      ShellCloseFile(&FileHandle);
    }

    //success
    Status = EFI_SUCCESS;

  Exit:
    if (List != NULL)      { FreeXmlTree(&List); }
    if (XmlString != NULL) { FreePool(XmlString); }

    break;

  default:
    Status = EFI_UNSUPPORTED;
    break;
  }

  return Status & 0x7FFFFFFFFFFFFF;;
}


/**
  Test entry point.

  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.

  @retval EFI_SUCCESS            Command completed successfully.

**/
EFI_STATUS
EFIAPI
UefiTestApp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  EFI_TCG2_PROTOCOL           *Tcg2Protocol;
  EFI_PHYSICAL_ADDRESS        EventLogLocation, EventLogLastEntry;
  BOOLEAN                     EventLogTruncated;
  EFI_TCG2_EVENT_LOG_FORMAT   RequestedFormat = EFI_TCG2_EVENT_LOG_FORMAT_TCG_2;

  //
  // Initialize the shell lib (we must be in non-auto-init...)
  //  NOTE: This may not be necessary, but whatever.
  //
  Status = ShellInitialize();
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed to init Shell.  %r\n", Status));
    return Status;
  }

  //
  // Let's locate the protocol.
  //
  Status = gBS->LocateProtocol( &gEfiTcg2ProtocolGuid, NULL, &Tcg2Protocol );

  //
  // Handle errors and move on.
  //
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed to located the TCG2 Protocol.  %r\n", Status));
  }
  else
  {
    //
    // Get the log.
    //
    Status = Tcg2Protocol->GetEventLog( Tcg2Protocol,
                                        RequestedFormat,
                                        &EventLogLocation,
                                        &EventLogLastEntry,
                                        &EventLogTruncated );
    if (EFI_ERROR( Status ))
    {
      DEBUG((DEBUG_ERROR, "Failed to retrieve the event log.  %r\n", Status));
    }
    else
    {
      Status = DumpEventLog( RequestedFormat, EventLogLocation, EventLogLastEntry, NULL );
    }
  }

  return Status;
}


