/**@file
DfciSettingPermissionProvisionXml.c

Thsi file supports the tool input path for setting permissions.
Permissions are set using XML.  That xml is written to a variable and then passed to UEFI to be applied.
This code supports that.

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

#include "DfciSettingPermission.h"

/**
Function to authenticate the data and get an identity based on the xml payload and signature
**/
EFI_STATUS
EFIAPI
ValidateAndAuthenticatePermissions(
    IN DFCI_INTERNAL_PACKET *Data)
{
  UINTN                     SignedDataLength = 0;
  UINTN                     SigLen = 0;
  WIN_CERTIFICATE          *SignaturePtr;
  EFI_STATUS                Status;


  DEBUG((DEBUG_INFO, "%a - SignedDataLength = 0x%X\n", __FUNCTION__, SignedDataLength));

  SignaturePtr = (WIN_CERTIFICATE *) PKT_FIELD_FROM_OFFSET(Data->Packet ,Data->SignedDataLength);
  SigLen = Data->PacketSize - Data->SignedDataLength;  //find out the max size of sig data based on var size and start of sig data.
  if (SigLen != SignaturePtr->dwLength)
  {
    DEBUG((DEBUG_ERROR, "%a - Signature Data not expected size (0x%X) (0x%X)\n", __FUNCTION__, SigLen, SignaturePtr->dwLength));
    Data->State = DFCI_PACKET_STATE_DATA_INVALID;
    Data->StatusCode = EFI_BAD_BUFFER_SIZE;
    return Data->StatusCode;
  }
  DEBUG((DEBUG_INFO, "%a - Session ID = 0x%X\n", __FUNCTION__, Data->SessionId));

  Status = mAuthenticationProtocol->AuthWithSignedData(mAuthenticationProtocol, (UINT8*)Data->Packet, Data->SignedDataLength, SignaturePtr, &Data->AuthToken);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to Authenticate Permissions Packet %r\n", __FUNCTION__, Status));
    Data->State = DFCI_PACKET_STATE_DATA_AUTH_FAILED;  //Auth Error
    Data->StatusCode = EFI_SECURITY_VIOLATION;
    Status = Data->StatusCode;
    return Data->StatusCode;
  }

  Data->State = DFCI_PACKET_STATE_DATA_AUTHENTICATED; //authenticated
  Status = EFI_SUCCESS;

  return Status;
}

//
// Apply all Permissions from XML to their associated setting providers
//
EFI_STATUS
EFIAPI
ApplyPermissionsInXml(
  IN DFCI_INTERNAL_PACKET *Data)
{
  XmlNode                   *InputRootNode = NULL;        //The root xml node for the Input list.
  XmlNode                   *InputPacketNode = NULL;      //The PermissionPacket node in the Input list
  XmlNode                   *InputPermissionsListNode = NULL;    //The Permissions node for the Input list.
  XmlNode                   *InputTempNode = NULL;        //Temp node ptr to use when moving thru the Input list

  XmlNode                   *ResultRootNode = NULL;       //The root xml node in the result list
  XmlNode                   *ResultPacketNode = NULL;     //The ResultsPacket node in the result list
  XmlNode                   *ResultPermissionsNode = NULL;   //The Permissions Node in the result list

  LIST_ENTRY                *Link = NULL;
  EFI_STATUS                 Status;
  UINTN                      StrLen = 0;
  UINTN                      Version = 0;
  UINTN                      Lsv = 0;
  UINTN                      NewLsv = 0;
  DFCI_IDENTITY_PROPERTIES   IdProps;
  BOOLEAN                    AppendToExistingPermission = FALSE;
  DFCI_PERMISSION_MASK       PMask = 0;
  DFCI_PERMISSION_STORE     *Store;
  BOOLEAN                    NewStore;
  DFCI_PERMISSION_MASK       DMask;
  EFI_TIME                   ApplyTime;

  NewStore = FALSE;
  Store = mPermStore;
  if (Data == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - NULL pointer received.\n", __FUNCTION__));
    ASSERT(FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (Data->State != DFCI_PACKET_STATE_DATA_AUTHENTICATED)
  {
    DEBUG((DEBUG_ERROR, "%a - Wrong start state (0x%X)\n", __FUNCTION__, Data->State));
    Data->State = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR;  // Code error. this shouldn't happen.
    Data->StatusCode = EFI_ABORTED;
    return Data->StatusCode;
  }

  //Check the auth.  Permission Updates can only be done by the Owner or Delegated Identities
  Status = mAuthenticationProtocol->GetIdentityProperties(mAuthenticationProtocol, &Data->AuthToken, &IdProps);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get Id properties using the Auth Token %r\n", __FUNCTION__, Status));
    Data->State = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR;  // Code error. this shouldn't happen.
    Data->StatusCode = EFI_ABORTED;
    return Data->StatusCode;
  }

  StrLen = AsciiStrnLenS(Data->Payload, Data->PayloadSize);
  DEBUG((DEBUG_INFO, "%a - StrLen = 0x%X PayloadSize = 0x%X\n", __FUNCTION__, StrLen, Data->PayloadSize));

  //
  // Create Node List from input
  //
  Status = CreateXmlTree(Data->Payload, StrLen, &InputRootNode);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Couldn't create a node list from the payload xml  %r\n", __FUNCTION__, Status));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  //print the list
  DEBUG((DEBUG_INFO, "PRINTING PERMISSION INPUT XML - Start\n"));
  DebugPrintXmlTree(InputRootNode, 0);
  DEBUG((DEBUG_INFO, "PRINTING PERMISSION INPUT XML - End\n"));

  //
  // Create Node List for output
  //
  Status = gRT->GetTime(&ApplyTime, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get time. %r\n", __FUNCTION__, Status));
    Data->State = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR;
    Status = EFI_ABORTED;
    goto EXIT;
  }

  ResultRootNode = New_ResultPermissionPacketNodeList(&ApplyTime);
  if(ResultRootNode == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Couldn't create a node list from the result xml.\n", __FUNCTION__));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status = EFI_ABORTED;
    goto EXIT;
  }

  //Get Input SettingsPacket Node
  InputPacketNode = GetPermissionPacketNode(InputRootNode);
  if (InputPacketNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get Input PermissionsPacket Node\n"));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  //Get Output GetResultsPermissionPacketNode
  ResultPacketNode = GetResultsPermissionPacketNode(ResultRootNode);
  if (ResultPacketNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get Output ResultsPermissionPacket Node\n"));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  //
  //Get input version
  //
  InputTempNode = FindFirstChildNodeByName(InputPacketNode, PERMISSIONS_VERSION_ELEMENT_NAME);
  if (InputTempNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get Version Node\n"));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }
  DEBUG((DEBUG_INFO, "Incomming Version: %a\n", InputTempNode->Value));
  Version = AsciiStrDecimalToUintn(InputTempNode->Value);

  if (Version > 0xFFFFFFFF)
  {
    DEBUG((DEBUG_INFO, "Version Value invalid.  0x%x\n", Version));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  //
  // Compare against save LSV
  //
  if (Version < Store->Lsv)
  {
    DEBUG((DEBUG_INFO, "%a - Incomming Permission Packet Has Lower Version (0x%X) than allowed LSV (0x%X). Can't apply\n", __FUNCTION__, Version, Store->Lsv));
    Data->State = DFCI_PACKET_STATE_VERSION_ERROR;
    Status = EFI_ACCESS_DENIED;
    goto EXIT;
  }

  //
  //Get Incomming LSV
  //
  InputTempNode = FindFirstChildNodeByName(InputPacketNode, PERMISSIONS_LSV_ELEMENT_NAME);
  if (InputTempNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get LSV Node\n"));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }
  DEBUG((DEBUG_INFO, "Incomming LSV: %a\n", InputTempNode->Value));
  Lsv = AsciiStrDecimalToUintn(InputTempNode->Value);

  if (Lsv > 0xFFFFFFFF)
  {
    DEBUG((DEBUG_INFO, "Lowest Supported Version Value invalid.  0x%x\n", Lsv));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  if (Lsv > Version)
  {
    DEBUG((DEBUG_ERROR, "%a - LSV (%a) can't be larger than current version\n", __FUNCTION__, InputTempNode->Value));
    Data->State = DFCI_PACKET_STATE_DATA_INVALID;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  NewLsv = MAX(Store->Lsv, Lsv);

  // Get the Xml Node for the PermissionsList
  InputPermissionsListNode = GetPermissionsListNodeFromPacketNode(InputPacketNode);

  if (InputPermissionsListNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get Input Permissions List Node\n"));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  ResultPermissionsNode = GetPermissionsListNodeFromPacketNode(ResultPacketNode);

  if (ResultPermissionsNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get Result Permissions List Node\n"));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status= EFI_ABORTED;  //internal xml..should never fail
    goto EXIT;
  }

  //if request is to replace then initialize a new perm store
  Status = PermissionListEntriesAppend(InputPermissionsListNode, &AppendToExistingPermission);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to get Required Append Attribute in Permission XML.  Bad XML Data. %r\n", Status));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  if (AppendToExistingPermission && (Data->V1Mode)) // Early V1 allows APPEND=TRUE.  This is depracated for V2
  {
    DEBUG((DEBUG_ERROR, "Append=TRUE specified for V2 processing.  Bad XML Data.\n"));
    Data->State = DFCI_PACKET_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  if (!AppendToExistingPermission)
  {
    // If not doing append, delete all permissions created by this Identity
    Status = DeletePermissionEntries (Store, IdProps.Identity);

    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to delete permission entries. Code = %r\n", __FUNCTION__, Status));
      Data->State = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR;
      Status = EFI_ABORTED;
      goto EXIT;
    }
  }

  Store->Lsv = (UINT32)NewLsv;  //Update LSV
  Store->Version = (UINT32)Version;  //Update Version

  //Handle Default Mask if set
  DMask = Store->DefaultDMask;
  PMask = Store->DefaultPMask;
  Status = GetPermissionsListDefaultPMask(InputPermissionsListNode, &PMask, &DMask);
  if (EFI_ERROR(Status))
  {
    if (Status == EFI_NOT_FOUND)
    {
      //this is ok.  New Permission Xml doesn't have default
      DEBUG((DEBUG_INFO, "%a - New Permissions doesn't define a default\n", __FUNCTION__));
    }
    else
    {
      DEBUG((DEBUG_INFO, "%a - Error while trying to get default entry %r\n", __FUNCTION__, Status));
      Data->State = DFCI_PACKET_STATE_BAD_XML;
      Status = EFI_NO_MAPPING;
      goto EXIT;
    }
  }
  else
  {  //have a good mask value
    Store->DefaultPMask = PMask;
    Store->DefaultDMask = DMask;
  }

  //All verified.   Now lets walk thru the Permission Entries and add them to our Permission List.  
  for (Link = InputPermissionsListNode->ChildrenListHead.ForwardLink; Link != &(InputPermissionsListNode->ChildrenListHead); Link = Link->ForwardLink)
  {
    XmlNode *NodeThis = NULL;
    DFCI_SETTING_ID_STRING Id = 0;
    DFCI_PERMISSION_MASK   Mask = 0;
    DFCI_PERMISSION_ENTRY *Entry = NULL;
    CHAR8                  StatusString[25];   //0xFFFFFFFFFFFFFFFF\n
   
    NodeThis = (XmlNode*)Link;   //Link is first member so just cast it.  this is the <Setting> node
    Status = GetInputPermission(NodeThis, &Id, &Mask, &DMask);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "Failed to Get Input Permission.  Bad XML Data. %r\n", Status));
      Data->State = DFCI_PACKET_STATE_BAD_XML;
      Status = EFI_NO_MAPPING;
      goto EXIT;
    }

    DEBUG((DEBUG_INFO, "%a - Setting Permission for ID %a to 0x%X\n", __FUNCTION__, Id, Mask));

    //Check if it already exists
    Status = EFI_SUCCESS;
    Entry = FindPermissionEntry(Store, Id, NULL, NULL);
    if (Entry)
    {

      if (IdProps.Identity & Entry->DMask)
      {
        Entry->PMask = Mask;
        if (DMask != DFCI_IDENTITY_NOT_SPECIFIED)   // If not specified, don't change
        {
          Entry->DMask = DMask;
        }
      }
      else
      {
        Status = EFI_ACCESS_DENIED;
        DEBUG((DEBUG_ERROR,"%a - failed to update permission. Access Denied. Id=%x, DMask=%x\n", __FUNCTION__, IdProps.Identity, Entry->DMask));
      }
    }
    else
    {
      if (IdProps.Identity & Store->DefaultDMask)
      {
        if (DMask == DFCI_IDENTITY_NOT_SPECIFIED)  // If not specified, use default
        {
          DMask = Store->DefaultDMask;
        }
        //Doesn't exist.  Add new
        Status = AddPermissionEntry(Store, Id, Mask, DMask);
        if (EFI_ERROR(Status))
        {
          DEBUG((DEBUG_ERROR, "%a - Failed to Add Entry to Perm Store %r\n", __FUNCTION__, Status));
          Data->State = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR;
          Status = EFI_ABORTED;
          goto EXIT;
        }
      }
      else
      {
        {
          Status = EFI_ACCESS_DENIED;
          DEBUG((DEBUG_ERROR,"%a - failed to add permission. Access Denied. Id=%x, Perm=%s\n", __FUNCTION__, IdProps.Identity, Store->DefaultDMask));
        }
      }
    }

    ZeroMem(StatusString, sizeof(StatusString));
    StatusString[0] = '0';
    StatusString[1] = 'x';

    AsciiValueToStringS(&(StatusString[2]), sizeof(StatusString)-2, RADIX_HEX, (INT64)Status, 18);
    Status = SetOutputPermissionStatus(ResultPermissionsNode, Id, &(StatusString[0]));
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "Failed to SetOutputPermissionStatus.  %r\n", Status));
      Data->State = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR;
      Status = EFI_DEVICE_ERROR;
      goto EXIT;
    }

  } //end for loop

  Data->State = DFCI_PACKET_STATE_DATA_APPLIED;

  //PRINT OUT XML HERE
  DEBUG((DEBUG_INFO, "PRINTING OUT PERMISSIONS RESULT XML - Start\n"));
  DebugPrintXmlTree(ResultRootNode, 0);
  DEBUG((DEBUG_INFO, "PRINTING OUTPUT PERMISSIONS RESULT XML - End\n"));

  //convert result xml node list to string
  Status = XmlTreeToString(ResultRootNode, TRUE, &Data->ResultXmlSize, &Data->ResultXml);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to convert Result XML to String.  Status = %r\n", Status));
    Status = EFI_ABORTED;
    goto EXIT;
  }

  //make sure its a good size
  if (Data->ResultXmlSize > MAX_ALLOWABLE_DFCI_RESULT_VAR_SIZE)
  {
    DEBUG((DEBUG_ERROR, "Size of result XML doc is too large (0x%X).\n", Data->ResultXmlSize));
    Status = EFI_ABORTED;
    goto EXIT;
  }

  StrLen = AsciiStrSize(Data->ResultXml);
  if (Data->ResultXmlSize != StrLen)
  {
    DEBUG((DEBUG_ERROR, "ResultXmlSize is not the correct size\n"));
  }
  DEBUG((DEBUG_INFO, "%a - ResultXmlSize = 0x%X  ResultXml String Length = 0x%X\n", __FUNCTION__, Data->ResultXmlSize, StrLen));

  //PRINT OUT PERMISSION STORE HERE
  DEBUG((DEBUG_INFO, "PRINTING OUT Permission Store\n"));
  DebugPrintPermissionStore(Store);
  if (NewStore)
  {
    FreePermissionStore(mPermStore);
    mPermStore = Store;
  }
  Status = EFI_SUCCESS;

EXIT:

  if (EFI_ERROR(Status))
  {
    if (NewStore)
    {
      FreePermissionStore(Store);
    }
    else
    {
      LoadFromFlash(&mPermStore);
    }
  }
  if (InputRootNode)
  {
    FreeXmlTree(&InputRootNode);
  }

  if (ResultRootNode)
  {
    FreeXmlTree(&ResultRootNode);
  }

  Data->StatusCode = Status;
  return Status;
}

//
// Create the Permission Result var
//
EFI_STATUS
EFIAPI
SetPermissionsResponse(
  IN  CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
  IN        DFCI_INTERNAL_PACKET        *Data
  ) {
  UINT8                                 HeaderVersion;
  DFCI_PERMISSION_POLICY_RESULT_VAR    *ResultVar = NULL;
  EFI_STATUS                            Status;
  UINTN                                 VarSize;

  if (Data == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->State == DFCI_PACKET_STATE_UNINITIALIZED)
  {
    return EFI_INVALID_PARAMETER;
  }

  // THe V1 and V1 Result var are identical for the first V1 length in bytes
  if (Data->Expected.Version == DFCI_PERMISSION_POLICY_VAR_VERSION)
  {
    // V2 Result VAR
    VarSize = sizeof(DFCI_PERMISSION_POLICY_RESULT_VAR) + Data->ResultXmlSize;
    HeaderVersion = DFCI_PERMISSION_POLICY_RESULT_VERSION;
  }
  else
  {
    // V1 Result VAR
    VarSize = sizeof(DFCI_PERMISSION_POLICY_RESULT_VAR_V1);
    HeaderVersion = DFCI_PERMISSION_POLICY_RESULT_VERSION_V1;
  }

  ResultVar = AllocatePool(VarSize);
  if (ResultVar == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to allocate memory for Var\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }
  ResultVar->Header.Signature = DFCI_PERMISSION_POLICY_RESULT_VAR_SIGNATURE;
  ResultVar->Header.Version = HeaderVersion;
  ResultVar->Status = Data->StatusCode;
  ResultVar->SessionId = Data->SessionId;

  if (Data->Expected.Version == DFCI_PERMISSION_POLICY_VAR_VERSION)
  {
    ResultVar->PayloadSize = (UINT16) Data->ResultXmlSize;
    CopyMem (&ResultVar->Payload, (UINT8 *) Data->ResultXml, Data->ResultXmlSize);
  }

  //save var to var store
  Status = gRT->SetVariable((CHAR16 *)Data->ResultName, &gDfciPermissionManagerVarNamespace, DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES, VarSize, ResultVar);
  DEBUG((DEBUG_INFO, "%a - Writing Variable for Results %r\n", __FUNCTION__, Status));

  if (ResultVar)
  {
    FreePool(ResultVar);
  }
  return Status;
}

//
// Clean up the incomming variable
//
VOID
EFIAPI
FreeNvVarsForIncommingPermissions(
  IN DFCI_INTERNAL_PACKET *Data)
{
  EFI_STATUS Status;
  if (Data == NULL)
  {
    return;
  }

  if (Data->State != DFCI_PACKET_STATE_UNINITIALIZED)
  {
    //delete the variable
    Status = gRT->SetVariable((CHAR16 *)Data->MailboxName, &gDfciPermissionManagerVarNamespace, 0, 0, NULL);
    DEBUG((DEBUG_INFO, "Delete Permission Apply Input variable %r\n", Status));
  }
}

/**
 *  Last Known Good handler
 *
 *  Applying identities does NOT change the internal variable, just the internal memory.
 *  After applying Identities, and LKG_COMMIT or LKG_DISCARD must be called
 *
 * @param[in] This:            Apply Packet Protocol
 * @param[in] Operation
 *                        DISCARD   discards the in memory changes, and retores from NV STORE
 *                        COMMIT    Saves the current settings to NV Store
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
LKG_Handler (
    IN  CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
    IN        DFCI_INTERNAL_PACKET        *Data,
    IN        UINT8                        Operation
  ) {
  EFI_STATUS    Status;


  Status = EFI_SUCCESS;

  FreeNvVarsForIncommingPermissions(Data);

  if ((This != &mApplyPermissionsProtocol) || (Data == NULL))
  {
    DEBUG((DEBUG_ERROR, "[PM] - Invalid parameters to LKG Handler.\n"));
    Status = EFI_INVALID_PARAMETER;
  } else {
    switch (Operation)
    {
      case DFCI_LKG_RESTORE:
        if (Data->LKGDirty)
        {

          Status = LoadFromFlash(&mPermStore);
          if (EFI_ERROR(Status))
          {
            DEBUG((DEBUG_ERROR, "[PM] - Unable to load provisioned data. Code=%r.\n",Status));
          } else {
            DEBUG((DEBUG_INFO, "[PM] - Lkg Permissions Restored.\n"));
          }
          Data->LKGDirty = FALSE;
        }
        break;

      case DFCI_LKG_COMMIT:
        if (Data->LKGDirty)
        {

          Status = SaveToFlash(mPermStore); //save it
          if (EFI_ERROR(Status))
          {
            DEBUG((DEBUG_ERROR, "[PM] - Unable to save permission data. Code=%r.\n",Status));
            if (EFI_ERROR(LoadFromFlash(&mPermStore)))
            {
              DEBUG((DEBUG_ERROR, "[PM] - Unable to restore current provisioned data after save failed.\n"));
            }
          } else {
            DEBUG((DEBUG_INFO, "[PM] - Lkg Permissions Committed.\n"));
            PopulateCurrentPermissions(TRUE);
          }
          Data->LKGDirty = FALSE;
        }
        break;

      default:
        DEBUG((DEBUG_ERROR, "[PM] - Invalid operation to LKG Handler(%d) in state (%d).\n",Operation,Data->LKGDirty));
        Status = EFI_INVALID_PARAMETER;
        break;
    }
    if (EFI_ERROR(Status))
    {
      Data->StatusCode = Status;
      Data->State = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR;
    }
  }
  return Status;
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
ValidatePermissionsPacket (
    IN       DFCI_INTERNAL_PACKET       *Data)
{
    UINT8   *EndData;


    if (Data->PacketSize > MAX_ALLOWABLE_DFCI_APPLY_VAR_SIZE)
    {
        DEBUG((DEBUG_ERROR, "%a - MAX_ALLOWABLE_DFCI_APPLY_VAR_SIZE.\n", __FUNCTION__));
        return EFI_COMPROMISED_DATA;
    }

    if (Data->SignedDataLength >= Data->PacketSize)
    {
        DEBUG((DEBUG_ERROR, "%a - Signed Data too large. %d >= %d.\n", __FUNCTION__,Data->SignedDataLength,Data->PacketSize));
        return EFI_COMPROMISED_DATA;
    }

    EndData = &Data->Packet->Pkt[Data->SignedDataLength];

    if ((UINT8 *) Data->Signature != EndData)
    {
        DEBUG((DEBUG_ERROR, "%a - Addr of Signatue not at EndData. %p != %p.\n", __FUNCTION__,Data->Signature, EndData));
        return EFI_COMPROMISED_DATA;
    }

    if (((UINT8 *)Data->Payload <= Data->Packet->Pkt) ||
        ((UINT8 *)Data->Payload+Data->PayloadSize > EndData))
    {
        DEBUG((DEBUG_ERROR, "%a - Payload outside Pkt. %p <= %p <= %p < %p.\n", __FUNCTION__,Data->Packet->Pkt, Data->Payload,Data->Payload+Data->PayloadSize, EndData));
        return EFI_COMPROMISED_DATA;
    }

    return EFI_SUCCESS;
}

/**
  Main Entry point into the Xml Provisioning code. 
  This will check the incomming variables, authenticate them, and apply permission settings.
**/

EFI_STATUS
EFIAPI
ApplyNewPermissionsPacket (
    IN CONST DFCI_APPLY_PACKET_PROTOCOL *This,
    IN       DFCI_INTERNAL_PACKET       *Data
  ) {

  EFI_STATUS Status;


  if ((This != &mApplyPermissionsProtocol) || (Data == NULL) || (mAuthenticationProtocol == NULL))
  {
    DEBUG((DEBUG_ERROR, "%a - Internal error processing apply packet.\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  if (Data->State != DFCI_PACKET_STATE_DATA_PRESENT)
  {
    DEBUG((DEBUG_ERROR, "%a - Error detected by caller.\n", __FUNCTION__));
    Status = EFI_ABORTED;
    goto CLEANUP;
  }

  // Validate the internal packet contents are valid
  Status = ValidatePermissionsPacket (Data);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Invalid packet.\n", __FUNCTION__));
    Data->State = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR;  // Code error. this shouldn't happen.
    Data->StatusCode = EFI_ABORTED;
    goto CLEANUP;
  }

  Status = ValidateAndAuthenticatePermissions(Data);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Input Permission failed Authentication\n"));
    goto CLEANUP;
  }

  Status = ApplyPermissionsInXml(Data);
  if (EFI_ERROR(Status))
  {  
    DEBUG((DEBUG_ERROR, "Input Permissions Apply Error\n"));
    goto CLEANUP;
  }
  
  mPermStore->Modified = TRUE;
  Data->LKGDirty = TRUE;

CLEANUP:    
  if (Data->AuthToken != DFCI_AUTH_TOKEN_INVALID)
  {
    mAuthenticationProtocol->DisposeAuthToken(mAuthenticationProtocol, &Data->AuthToken);
  }

  return Status;
}
