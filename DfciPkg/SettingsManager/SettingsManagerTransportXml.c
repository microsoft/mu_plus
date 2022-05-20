/**@file
SettingsManagerTransportXml.c

This file supports the tool input path for setting settings.
Settings are set using XML.  That xml is written to a variable and then passed to UEFI to be applied.
This code supports that.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SettingsManager.h"

/**
Function to authenticate the data and get an identity based on the xml payload and signature
**/
EFI_STATUS
EFIAPI
ValidateAndAuthenticateSettings (
  IN       DFCI_INTERNAL_PACKET  *Data
  )
{
  EFI_STATUS  Status;

  if (Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "%a - Session ID = 0x%X\n", __FUNCTION__, Data->SessionId));

  // Lets check for device specific targeting using Serial Number
  Status = CheckAuthAndGetToken (Data->Packet->Hdr.Pkt, Data->SignedDataLength, Data->Signature, &Data->AuthToken);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to Authenticate Settings %r\n", __FUNCTION__, Status));
    Data->State      = DFCI_PACKET_STATE_DATA_AUTH_FAILED; // Auth Error
    Data->StatusCode = EFI_SECURITY_VIOLATION;
    Status           = Data->StatusCode;
    return Data->StatusCode;
  }

  Data->State = DFCI_PACKET_STATE_DATA_AUTHENTICATED; // authenticated
  Status      = EFI_SUCCESS;

  return Status;
}

//
// Apply all settings from XML to their associated setting providers
//
EFI_STATUS
EFIAPI
ApplySettings (
  IN DFCI_INTERNAL_PACKET        *Data,
  IN DFCI_SETTING_INTERNAL_DATA  *InternalData
  )
{
  XmlNode  *InputRootNode     = NULL;                     // The root xml node for the Input list.
  XmlNode  *InputPacketNode   = NULL;                     // The SettingsPacket node in the Input list
  XmlNode  *InputSettingsNode = NULL;                     // The Settings node for the Input list.
  XmlNode  *InputTempNode     = NULL;                     // Temp node ptr to use when moving thru the Input list

  XmlNode  *ResultRootNode     = NULL;                    // The root xml node in the result list
  XmlNode  *ResultPacketNode   = NULL;                    // The ResultsPacket node in the result list
  XmlNode  *ResultSettingsNode = NULL;                    // The Settings Node in the result list

  LIST_ENTRY          *Link = NULL;
  EFI_STATUS          Status;
  UINTN               StrLen = 0;
  DFCI_SETTING_FLAGS  Flags  = 0;
  EFI_TIME            ApplyTime;
  UINTN               Version = 0;
  UINTN               Lsv     = 0;

  if (Data == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - NULL pointer received.\n", __FUNCTION__));
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (Data->State != DFCI_PACKET_STATE_DATA_AUTHENTICATED) {
    DEBUG ((DEBUG_ERROR, "%a - Wrong start state (0x%X)\n", __FUNCTION__, Data->State));
    Data->State      = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR; // Code error. this shouldn't happen.
    Data->StatusCode = EFI_ABORTED;
    return Data->StatusCode;
  }

  StrLen = AsciiStrnLenS ((CHAR8 *)Data->Payload, Data->PayloadSize);
  DEBUG ((DEBUG_INFO, "%a - StrLen = 0x%X PayloadSize = 0x%X\n", __FUNCTION__, StrLen, Data->PayloadSize));

  //
  // Create Node List from input
  //
  Status = CreateXmlTree ((CHAR8 *)Data->Payload, StrLen, &InputRootNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Couldn't create a node list from the payload xml  %r\n", __FUNCTION__, Status));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status      = EFI_NO_MAPPING;
    goto EXIT;
  }

  // print the list
  DEBUG ((DEBUG_INFO, "PRINTING INPUT XML - Start\n"));
  DebugPrintXmlTree (InputRootNode, 0);
  DEBUG ((DEBUG_INFO, "PRINTING INPUT XML - End\n"));

  //
  // Create Node List for output
  //
  Status = gRT->GetTime (&ApplyTime, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to get time. %r\n", __FUNCTION__, Status));
    Data->State = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR;
    Status      = EFI_ABORTED;
    goto EXIT;
  }

  ResultRootNode = New_ResultPacketNodeList (&ApplyTime);
  if (ResultRootNode == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Couldn't create a node list from the result xml.\n", __FUNCTION__));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status      = EFI_ABORTED;
    goto EXIT;
  }

  // Get Input SettingsPacket Node
  InputPacketNode = GetSettingsPacketNode (InputRootNode);
  if (InputPacketNode == NULL) {
    DEBUG ((DEBUG_INFO, "Failed to Get Input SettingsPacket Node\n"));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status      = EFI_NO_MAPPING;
    goto EXIT;
  }

  // Get Output ResultsPacket Node
  ResultPacketNode = GetResultsPacketNode (ResultRootNode);
  if (ResultPacketNode == NULL) {
    DEBUG ((DEBUG_INFO, "Failed to Get Output ResultsPacket Node\n"));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status      = EFI_NO_MAPPING;
    goto EXIT;
  }

  //
  // Get input version
  //
  InputTempNode = FindFirstChildNodeByName (InputPacketNode, SETTINGS_VERSION_ELEMENT_NAME);
  if (InputTempNode == NULL) {
    DEBUG ((DEBUG_INFO, "Failed to Get Version Node\n"));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status      = EFI_NO_MAPPING;
    goto EXIT;
  }

  DEBUG ((DEBUG_INFO, "Incoming Version: %a\n", InputTempNode->Value));
  Version = AsciiStrDecimalToUintn (InputTempNode->Value);

  if (Version > 0xFFFFFFFF) {
    DEBUG ((DEBUG_INFO, "Version Value invalid.  0x%x\n", Version));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status      = EFI_NO_MAPPING;
    goto EXIT;
  }

  // check against lsv
  if (InternalData->LSV > (UINT32)Version) {
    DEBUG ((DEBUG_INFO, "Setting Version Less Than System LSV(%ld)\n", InternalData->LSV));
    Data->State = DFCI_PACKET_STATE_VERSION_ERROR;
    Status      = EFI_ACCESS_DENIED;
    goto EXIT;
  }

  //
  // Get Incoming LSV
  //
  InputTempNode = FindFirstChildNodeByName (InputPacketNode, SETTINGS_LSV_ELEMENT_NAME);
  if (InputTempNode == NULL) {
    DEBUG ((DEBUG_INFO, "Failed to Get LSV Node\n"));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status      = EFI_NO_MAPPING;
    goto EXIT;
  }

  DEBUG ((DEBUG_INFO, "Incoming LSV: %a\n", InputTempNode->Value));
  Lsv = AsciiStrDecimalToUintn (InputTempNode->Value);

  if (Lsv > 0xFFFFFFFF) {
    DEBUG ((DEBUG_INFO, "Lowest Supported Version Value invalid.  0x%x\n", Lsv));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status      = EFI_NO_MAPPING;
    goto EXIT;
  }

  if (Lsv > Version) {
    DEBUG ((DEBUG_ERROR, "%a - LSV (%a) can't be larger than current version\n", __FUNCTION__, InputTempNode->Value));
    Data->State = DFCI_PACKET_STATE_DATA_INVALID;
    Status      = EFI_NO_MAPPING;
    goto EXIT;
  }

  // set the new version
  if (InternalData->CurrentVersion != (UINT32)Version) {
    InternalData->CurrentVersion = (UINT32)Version;
    InternalData->Modified       = TRUE;
  }

  // If new LSV is larger set it
  if ((UINT32)Lsv > InternalData->LSV) {
    DEBUG ((DEBUG_INFO, "%a - Setting New LSV (0x%X)\n", __FUNCTION__, Lsv));
    InternalData->LSV      = (UINT32)Lsv;
    InternalData->Modified = TRUE;
  }

  // Get the Xml Node for the SettingsList
  InputSettingsNode = GetSettingsListNodeFromPacketNode (InputPacketNode);

  if (InputSettingsNode == NULL) {
    DEBUG ((DEBUG_INFO, "Failed to Get Input Settings List Node\n"));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status      = EFI_NO_MAPPING;
    goto EXIT;
  }

  ResultSettingsNode = GetSettingsListNodeFromPacketNode (ResultPacketNode);

  if (ResultSettingsNode == NULL) {
    DEBUG ((DEBUG_INFO, "Failed to Get Result Settings List Node\n"));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status      = EFI_ABORTED; // internal xml..should never fail
    goto EXIT;
  }

  // All verified.   Now lets walk thru the Settings and try to apply each one.
  for (Link = InputSettingsNode->ChildrenListHead.ForwardLink; Link != &(InputSettingsNode->ChildrenListHead); Link = Link->ForwardLink) {
    XmlNode                 *NodeThis = NULL;
    DFCI_SETTING_ID_STRING  Id        = NULL;
    CONST CHAR8             *Value    = NULL;
    CHAR8                   StatusString[25]; // 0xFFFFFFFFFFFFFFFF\n
    CHAR8                   FlagString[25];
    Flags = 0;

    NodeThis = (XmlNode *)Link;   // Link is first member so just cast it.  this is the <Setting> node
    Status   = GetInputSettings (NodeThis, &Id, &Value);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to GetInputSettings.  Bad XML Data. %r\n", Status));
      Data->State = DFCI_PACKET_STATE_BAD_XML;
      Status      = EFI_NO_MAPPING;
      goto EXIT;
    }

    // Now we have an Id and Value
    Status = SetSettingFromAscii (Id, Value, &Data->AuthToken, &Flags);
    DEBUG ((DEBUG_INFO, "%a - Set %a = %a. Result = %r\n", __FUNCTION__, Id, Value, Status));

    // Record Status result
    ZeroMem (StatusString, sizeof (StatusString));
    ZeroMem (FlagString, sizeof (FlagString));
    StatusString[0] = '0';
    StatusString[1] = 'x';
    FlagString[0]   = '0';
    FlagString[1]   = 'x';

    AsciiValueToStringS (&(StatusString[2]), sizeof (StatusString)-2, RADIX_HEX, (INT64)Status, 18);
    AsciiValueToStringS (&(FlagString[2]), sizeof (FlagString)-2, RADIX_HEX, (INT64)Flags, 18);
    Status = SetOutputSettingsStatus (ResultSettingsNode, Id, &(StatusString[0]), &(FlagString[0]));
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to SetOutputSettingStatus.  %r\n", Status));
      Data->State = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR;
      Status      = EFI_DEVICE_ERROR;
      goto EXIT;
    }

    if (Flags & DFCI_SETTING_FLAGS_OUT_REBOOT_REQUIRED) {
      Data->ResetRequired = TRUE;
    }

    // all done.
  } // end for loop

  Data->State = DFCI_PACKET_STATE_DATA_APPLIED;

  // PRINT OUT XML HERE
  DEBUG ((DEBUG_INFO, "PRINTING OUT XML - Start\n"));
  DebugPrintXmlTree (ResultRootNode, 0);
  DEBUG ((DEBUG_INFO, "PRINTING OUTPUT XML - End\n"));

  // convert result xml node list to string
  Status = XmlTreeToString (ResultRootNode, TRUE, &Data->ResultXmlSize, &Data->ResultXml);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to convert Result XML to String.  Status = %r\n", Status));
    Status = EFI_ABORTED;
    goto EXIT;
  }

  // make sure its a good size
  if (Data->ResultXmlSize > MAX_ALLOWABLE_DFCI_RESULT_VAR_SIZE) {
    DEBUG ((DEBUG_ERROR, "Size of result XML doc is too large (0x%X).\n", Data->ResultXmlSize));
    Status = EFI_ABORTED;
    goto EXIT;
  }

  StrLen = AsciiStrSize (Data->ResultXml);
  if (Data->ResultXmlSize != StrLen) {
    DEBUG ((DEBUG_ERROR, "ResultXmlSize is not the correct size\n"));
  }

  DEBUG ((DEBUG_INFO, "%a - ResultXmlSize = 0x%X  ResultXml String Length = 0x%X\n", __FUNCTION__, Data->ResultXmlSize, StrLen));
  Status = EFI_SUCCESS;

EXIT:
  if (InputRootNode) {
    FreeXmlTree (&InputRootNode);
  }

  if (ResultRootNode) {
    FreeXmlTree (&ResultRootNode);
  }

  Data->StatusCode = Status;
  return Status;
}

//
// Create the Setting Result var
//
VOID
EFIAPI
UpdateSettingsResult (
  IN DFCI_INTERNAL_PACKET        *Data,
  IN DFCI_SETTING_INTERNAL_DATA  *InternalData
  )
{
  DFCI_SECURED_SETTINGS_RESULT_VAR  *ResultVar = NULL;
  UINTN                             VarSize    = 0;
  EFI_STATUS                        Status;

  if (Data == NULL) {
    return;
  }

  if (Data->State == DFCI_PACKET_STATE_UNINITIALIZED) {
    return;
  }

  VarSize   = Data->ResultXmlSize + sizeof (DFCI_SECURED_SETTINGS_RESULT_VAR);
  ResultVar = AllocatePool (VarSize);
  if (ResultVar == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to allocate memory for Var\n", __FUNCTION__));
    return;
  }

  ResultVar->Header.Hdr.Signature = DFCI_SECURED_SETTINGS_RESULT_VAR_SIGNATURE;
  ResultVar->Header.Version       = DFCI_SECURED_SETTINGS_RESULTS_VERSION;
  ResultVar->Status               = Data->StatusCode;
  ResultVar->SessionId            = Data->SessionId;
  ResultVar->PayloadSize          = (UINT16)Data->ResultXmlSize;
  if (Data->ResultXml != NULL) {
    CopyMem (ResultVar->Payload, Data->ResultXml, Data->ResultXmlSize);
  }

  // save var to var store
  Status = gRT->SetVariable ((CHAR16 *)Data->ResultName, &gDfciSettingsManagerVarNamespace, DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES, VarSize, ResultVar);
  DEBUG ((DEBUG_INFO, "%a - Writing Variable %s for Results %r\n", __FUNCTION__, Data->ResultName, Status));

  if (ResultVar) {
    FreePool (ResultVar);
  }

  if (!EFI_ERROR (Data->StatusCode)) {
    Status = SMID_SaveToFlash (InternalData);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Writing New Internal Data to Flash Error %r\n", __FUNCTION__, Status));
      ASSERT_EFI_ERROR (Status);
    }
  }
}

//
// Clean up the incoming variable
//
VOID
EFIAPI
FreeSettings (
  IN DFCI_INTERNAL_PACKET  *Data
  )
{
  EFI_STATUS  Status;

  if (Data == NULL) {
    return;
  }

  if (Data->State != DFCI_PACKET_STATE_UNINITIALIZED) {
    // delete the variable
    Status = gRT->SetVariable ((CHAR16 *)Data->MailboxName, &gDfciSettingsManagerVarNamespace, 0, 0, NULL);
    DEBUG ((DEBUG_INFO, "Delete Xml Settings Apply Input variable %r\n", Status));
  }
}

//
// Free locally allocated memory
//  -- this function only gets called when system is not resetting.
//
VOID
EFIAPI
FreeInstanceMemory (
  IN DFCI_INTERNAL_PACKET        *Data,
  IN DFCI_SETTING_INTERNAL_DATA  **InternalData
  )
{
  if (Data == NULL) {
    return;
  }

  if (Data->ResultXml != NULL) {
    FreePool (Data->ResultXml);
    Data->ResultXml = NULL;
  }

  if (*InternalData != NULL) {
    FreePool (*InternalData);
    *InternalData = NULL;
  }
}

/**
 * Validate that all secure information points within the
 * signed data.
 *
 * @param Data
 *
 * @return EFI_STATUS
 */
EFI_STATUS
ValidateSettingsPacket (
  IN       DFCI_INTERNAL_PACKET  *Data
  )
{
  UINT8  *EndData;

  if (Data->PacketSize > MAX_ALLOWABLE_DFCI_APPLY_VAR_SIZE) {
    DEBUG ((DEBUG_ERROR, "%a - MAX_ALLOWABLE_DFCI_APPLY_VAR_SIZE.\n", __FUNCTION__));
    return EFI_COMPROMISED_DATA;
  }

  if (Data->SignedDataLength >= Data->PacketSize) {
    DEBUG ((DEBUG_ERROR, "%a - Signed Data too large. %d >= %d.\n", __FUNCTION__, Data->SignedDataLength, Data->PacketSize));
    return EFI_COMPROMISED_DATA;
  }

  EndData = &Data->Packet->Hdr.Pkt[Data->SignedDataLength];

  if ((UINT8 *)Data->Signature != EndData) {
    DEBUG ((DEBUG_ERROR, "%a - Addr of Signature not at EndData. %p != %p.\n", __FUNCTION__, Data->Signature, EndData));
    return EFI_COMPROMISED_DATA;
  }

  if (((UINT8 *)Data->Payload <= Data->Packet->Hdr.Pkt) ||
      ((UINT8 *)Data->Payload+Data->PayloadSize > EndData))
  {
    DEBUG ((DEBUG_ERROR, "%a - Payload outside Pkt. %p <= %p <= %p < %p.\n", __FUNCTION__, Data->Packet->Hdr.Pkt, Data->Payload, Data->Payload+Data->PayloadSize, EndData));
    return EFI_COMPROMISED_DATA;
  }

  return EFI_SUCCESS;
}

/**
 * Apply New Settings
 *
 *
 * @param This
 * @param Data
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
ApplyNewSettingsPacket (
  IN CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
  IN       DFCI_INTERNAL_PACKET        *Data
  )
{
  EFI_STATUS                  Status;
  DFCI_SETTING_INTERNAL_DATA  *InternalData = NULL;

  if ((This != &mApplySettingsProtocol) || (Data == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a - Bad parameters received.\n", __FUNCTION__));
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (Data->State != DFCI_PACKET_STATE_DATA_PRESENT) {
    DEBUG ((DEBUG_ERROR, "%a - Error detected by caller.\n", __FUNCTION__));
    Status = EFI_ABORTED;
    goto CLEANUP;
  }

  // Validate the internal packet contents are valid
  Status = ValidateSettingsPacket (Data);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Invalid packet.\n", __FUNCTION__));
    Data->State      = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR; // Code error. this shouldn't happen.
    Data->StatusCode = EFI_ABORTED;
    goto CLEANUP;
  }

  // Load current internal data info
  Status = SMID_LoadFromFlash (&InternalData);
  if (EFI_ERROR (Status)) {
    if (Status != EFI_NOT_FOUND) {
      DEBUG ((DEBUG_ERROR, "%a - Failed to load Settings Manager Internal Data. %r\n", __FUNCTION__, Status));
    }

    // If load failed - init store
    Status = SMID_InitInternalData (&InternalData);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Couldn't Init Settings Internal Data %r\n", __FUNCTION__, Status));
      InternalData = NULL;
      ASSERT (InternalData != NULL);
      goto CLEANUP;
    }
  }

  Status = ValidateAndAuthenticateSettings (Data);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Input Settings failed Authentication\n"));
    goto CLEANUP;
  }

  Status = ApplySettings (Data, InternalData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Input Settings Apply Error\n"));
    goto CLEANUP;
  }

CLEANUP:
  if (InternalData != NULL) {
    UpdateSettingsResult (Data, InternalData);
  }

  FreeSettings (Data);
  AuthTokenDispose (&Data->AuthToken);
  FreeInstanceMemory (Data, &InternalData);

  return Status;
}
