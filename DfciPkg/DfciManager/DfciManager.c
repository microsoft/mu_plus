/** @file
DfciManager.c

This module implements the Dfci Package Deployment.

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

#include "DfciManager.h"

typedef
EFI_STATUS
(EFIAPI *DECODE_PACKET) (
    IN DFCI_INTERNAL_PACKET  *Data
);

typedef
EFI_STATUS
(EFIAPI *ID_SUPPORT) (
    IN CHAR8   **Name,
    IN UINTN    *Size
);

typedef struct {
   DFCI_APPLY_PACKET_PROTOCOL  *ApplyProtocol;
   DFCI_INTERNAL_PACKET        *Data;
   EFI_STATUS                   DecodeStatus;
} DFCI_MANAGER_DATA;

#define MGR_IDENTITY     (0)
#define MGR_PERMISSIONS  (1)
#define MGR_SETTINGS     (2)
#define MGR_IDENTITY2    (3)
#define MGR_PERMISSIONS2 (4)
#define MGR_SETTINGS2    (5)
#define MGR_MAX          (6)

static DFCI_APPLY_PACKET_PROTOCOL *mApplyIdentityProtocol;
static DFCI_APPLY_PACKET_PROTOCOL *mApplyPermissionsProtocol;
static DFCI_APPLY_PACKET_PROTOCOL *mApplySettingsProtocol;
static EFI_EVENT                   mEndOfDxeEvent;
static BOOLEAN                     mProcessingAtEndOfDxe = FALSE;
static BOOLEAN                     mRebootRequired = FALSE;
static DFCI_MANAGER_DATA           mManagerData[MGR_MAX] = {{NULL, NULL, EFI_SUCCESS},
                                                            {NULL, NULL, EFI_SUCCESS},
                                                            {NULL, NULL, EFI_SUCCESS},
                                                            {NULL, NULL, EFI_SUCCESS},
                                                            {NULL, NULL, EFI_SUCCESS},
                                                            {NULL, NULL, EFI_SUCCESS}};

/**
 * Process Mailboxes
 *
 * Called in mainline, or at one of the callbacks (EndOfDxe, or SettingAccess).
 *
 * @param
 *
 * @return EFI_STATUS
 */
EFI_STATUS
ProcessMailBoxes (
);

/**
 * RunProcessMailBoxes - Set the TPL to TPL_APPLICATION and run ProcessMailboxes
 *
 * @param
 *
 * @return VOID
 */
VOID
RunProcessMailBoxes (
  ) {
    EFI_TPL OldTpl;


    OldTpl = gBS->RaiseTPL(TPL_NOTIFY);
    gBS->RestoreTPL(TPL_APPLICATION);

    ProcessMailBoxes();

    gBS->RaiseTPL(OldTpl);
    return;
}

/**
 * Event callback for End Of Dxe.
 * This is needed when processing a provisioning request that requires
 * user confirmation.
 *
 * @param Event
 * @param Context
 *
 * @return VOID EFIAPI
 */
VOID
EFIAPI
EndOfDxeCallback (
    IN EFI_EVENT Event,
    IN VOID* Context
  ) {


    PERF_FUNCTION_BEGIN (PERF_VERBOSITY_STANDARD);

    DEBUG((DEBUG_INFO, "[DM] %a: ProcessMailboxes at EndOfDxe\n", __FUNCTION__));

    //
    // Check if the UI components we need are available. If not, bail.
    //
    if (DfciUiIsUiAvailable() == FALSE) {
        DEBUG((DEBUG_ERROR, "[DM] %a - Callback trigggered. UI not available\n", __FUNCTION__));
        ASSERT(FALSE);
        return;
    }

    //
    // Try again to process provisioning input.
    //
    mProcessingAtEndOfDxe = TRUE;
    RunProcessMailBoxes ();

    gBS->CloseEvent(Event);

    PERF_FUNCTION_END (PERF_VERBOSITY_STANDARD);
}

/**
 * Event callback for End Of Dxe.
 * This is needed when a privisioning request that requires
 * user confirmation.
 *
 * @param Event
 * @param Context
 *
 * @return VOID EFIAPI
 */
VOID
EFIAPI
SettingAccessCallback (
    IN EFI_EVENT Event,
    IN VOID* Context
  ) {


    PERF_FUNCTION_BEGIN (PERF_VERBOSITY_STANDARD);
    DEBUG((DEBUG_INFO, "[DM] %a: ProcessMailboxes at SettingsAccess\n", __FUNCTION__));
    //
    // Try again to process provisioning input.
    //
    RunProcessMailBoxes ();

    gBS->CloseEvent(Event);

    PERF_FUNCTION_END (PERF_VERBOSITY_STANDARD);
}

/**
 * Check Dfci Target V2 packet target
 *
 * Allow Wildcards
 *
 * @param Data
 * @param IdSupport
 * @param Name
 * @param NameSize
 *
 * @return VOID
 */
EFI_STATUS
CheckTarget (
    IN DFCI_INTERNAL_PACKET    *Data,
    IN ID_SUPPORT               IdSupport,
    IN CHAR8                   *Name,
    IN UINTN                    NameSize
  ) {
    EFI_STATUS  Status;
    CHAR8      *Temp;
    UINTN       TempSize;


    if (*Name == '\0') {
        Data->DfciWildcard = TRUE;
    } else {
        Status = IdSupport (&Temp, &TempSize);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "[DM] Unable to get IdSupport value. Code -%r\n", Status));
            Data->StatusCode = EFI_ABORTED;
            Data->State = DFCI_PACKET_STATE_DATA_NOT_CORRECT_TARGET;
            return Data->StatusCode;
        }
        if ((TempSize != NameSize) ||
            (0 != CompareMem(Name, Temp, TempSize))) {
            DEBUG((DEBUG_ERROR, "[DM] Target failed  %a - %a\n",  Name, Temp));
            Data->StatusCode = EFI_ABORTED;
            Data->State = DFCI_PACKET_STATE_DATA_NOT_CORRECT_TARGET;
            return Data->StatusCode;
        }
        FreePool(Temp);
    }

    return EFI_SUCCESS;
}

/**
 * Decode the bulk of the packet
 *
 * This function decodes the incoming V2 DFCI Packet for internal use
 *
 * @param InternalPacket
 *
 */
EFI_STATUS
EFIAPI
DecodePacket (
    IN DFCI_INTERNAL_PACKET    *Data
  ) {
    DFCI_PACKET_HEADER  *Packet;
    EFI_STATUS           Status;


    if ((Data->PacketSize == 0) ||
        (Data->Packet == NULL)) {
        Data->StatusCode = EFI_INVALID_PARAMETER;
        Data->State = DFCI_PACKET_STATE_DATA_INVALID;
        return Data->StatusCode;
    }

    //Check incomming size
    if (Data->PacketSize > MAX_ALLOWABLE_DFCI_APPLY_VAR_SIZE) {
      DEBUG((DEBUG_ERROR, "[DM] %a: %s Incomming Apply var is too big (%d bytes)\n", __FUNCTION__, Data->MailboxName, Data->PacketSize));
      Data->State = DFCI_PACKET_STATE_DATA_INVALID;
      Data->StatusCode = EFI_BAD_BUFFER_SIZE;
      return Data->StatusCode;
    }

    Data->State = DFCI_PACKET_STATE_DATA_PRESENT;
    DEBUG((DEBUG_INFO, "[DM] %a: %s Variable Size: 0x%X\n", __FUNCTION__, Data->MailboxName, Data->PacketSize));

    if (Data->PacketSize < sizeof(DFCI_PACKET_HEADER)) {
        DEBUG((DEBUG_ERROR, "[DM] Apply VarSize too small. Size: 0x%X MinSize: 0x%X\n", Data->PacketSize, sizeof(DFCI_SIGNER_PROVISION_APPLY_VAR)));
        Data->StatusCode = EFI_COMPROMISED_DATA;
        Data->State = DFCI_PACKET_STATE_DATA_INVALID;
        return Data->StatusCode;
    }

    if (Data->Packet->Signature != Data->Expected.Signature) {
        DEBUG((DEBUG_ERROR, "[DM] Var Signature not valid. Sig=%x, Exp=%x\n", Data->Packet->Signature, Data->Expected.Signature));
        Data->StatusCode = EFI_COMPROMISED_DATA;
        Data->State = DFCI_PACKET_STATE_DATA_INVALID;
        return Data->StatusCode;
    }

    if (Data->Packet->Version != Data->Expected.Version) {
        DEBUG((DEBUG_ERROR, "[DM] Var Version not current. Sig=%x, Exp=%x\n", Data->Packet->Version, Data->Expected.Version));
        Data->StatusCode = EFI_INCOMPATIBLE_VERSION;
        Data->State = DFCI_PACKET_STATE_DATA_INVALID;
        return Data->StatusCode;
    }

    Packet = (DFCI_PACKET_HEADER *) Data->Packet;

    if ((Packet->SystemMfgOffset     >= Packet->SystemProductOffset) ||
        (Packet->SystemProductOffset >= Packet->SystemSerialOffset)  ||
        (Packet->SystemSerialOffset  >= Packet->PayloadOffset)       ||
        (Packet->SystemMfgOffset     < sizeof(DFCI_PACKET_HEADER))   ||
        (Packet->SystemProductOffset < sizeof(DFCI_PACKET_HEADER))   ||
        (Packet->SystemSerialOffset  < sizeof(DFCI_PACKET_HEADER))) {
        DEBUG((DEBUG_ERROR, "[DM] Targeting String Structure invalid.\n"));
        Data->StatusCode = EFI_INCOMPATIBLE_VERSION;
        Data->State = DFCI_PACKET_STATE_DATA_INVALID;
        return Data->StatusCode;
    }

    Data->SignedDataLength = Packet->PayloadOffset + Packet->PayloadSize;
    Data->SessionId = Packet->SessionId;
    Data->PayloadSize = Packet->PayloadSize;
    if (Data->PayloadSize != 0) {
        Data->Payload = PKT_FIELD_FROM_OFFSET(Packet, Packet->PayloadOffset);
    }

    Packet->SessionId = 0;      // Packet Session ID must be zero for signature.

    if (Data->PacketSize < Data->SignedDataLength + sizeof(WIN_CERTIFICATE)) {
        DEBUG((DEBUG_ERROR, "[DM] Identity VarSize too small. Size: 0x%X MinSize: 0x%X\n", Data->PacketSize, Data->SignedDataLength + sizeof(WIN_CERTIFICATE)));
        Data->StatusCode = EFI_COMPROMISED_DATA;
        Data->State = DFCI_PACKET_STATE_DATA_INVALID;
        return Data->StatusCode;
    }

    Data->Manufacturer =  PKT_FIELD_FROM_OFFSET(Packet,Packet->SystemMfgOffset);
    Data->ManufacturerSize = Packet->SystemProductOffset - Packet->SystemMfgOffset;
    Data->ProductName =  PKT_FIELD_FROM_OFFSET(Packet,Packet->SystemProductOffset);
    Data->ProductNameSize = Packet->SystemSerialOffset - Packet->SystemProductOffset;
    Data->SerialNumber =  PKT_FIELD_FROM_OFFSET(Packet,Packet->SystemSerialOffset);
    Data->SerialNumberSize = Packet->PayloadOffset - Packet->SystemSerialOffset;

    Data->Signature = (WIN_CERTIFICATE*) PKT_FIELD_FROM_OFFSET(Packet,Data->SignedDataLength);

    if (Data->PacketSize < Data->SignedDataLength + Data->Signature->dwLength) {
        DEBUG((DEBUG_ERROR, "[DM] %a: Signature Data not expected size (0x%X) (0x%X)\n", __FUNCTION__, Data->PacketSize, Data->SignedDataLength + Data->Signature->dwLength));
        Data->State = DFCI_PACKET_STATE_DATA_INVALID;
        Data->StatusCode = EFI_BAD_BUFFER_SIZE;
        return Data->StatusCode;
    }

    Status = CheckTarget (Data, DfciIdSupportGetManufacturer, Data->Manufacturer, Data->ManufacturerSize);
    if (!EFI_ERROR(Status)) {
        CheckTarget(Data, DfciIdSupportGetProductName, Data->ProductName, Data->ProductNameSize);
    }

    if (!EFI_ERROR(Status)) {
        CheckTarget (Data, DfciIdSupportGetSerialNumber, Data->SerialNumber, Data->SerialNumberSize);
    }

    return Status;
}

/**
 * Decode Identity Packet
 *
 * This function decodes the incoming V2 DFCI Packet for internal use
 *
 * @param InternalPacket
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
DecodeIdentityPacket (
    IN DFCI_INTERNAL_PACKET    *Data
  ) {
    DFCI_SIGNER_PROVISION_APPLY_VAR *IdentityPacket;
    EFI_STATUS                       Status;


    Status = DecodePacket(Data);

    if (!EFI_ERROR(Status)) {
        IdentityPacket = (DFCI_SIGNER_PROVISION_APPLY_VAR *) Data->Packet;
        Data->VarIdentity = &IdentityPacket->Header.Identity;
        Data->Version = &IdentityPacket->Version;
        Data->LSV = &IdentityPacket->LSV;

        if (Data->PayloadSize == 0) {
            DEBUG((DEBUG_INFO, "[DM] %a: Delaying UnEnroll until after permissions and settings\n", __FUNCTION__));
            Data->Unenroll = TRUE;
        }
    }

    return Status;;
}

/**
 * Queue ProcessMailbox to run at EndOfDxe
 *
 * Halt process of mailboxes until EndOfDxe when User Auth is required.
 *
 * @param
 *
 * @return EFI_STATUS
 */
EFI_STATUS
QueueMailboxAtEndOfDxe () {
    EFI_STATUS Status;


    if (mProcessingAtEndOfDxe) {
        DEBUG((DEBUG_INFO, "Queue for EndOfDxe satified\n"));
        Status = EFI_SUCCESS;
    } else {
        Status = EFI_MEDIA_CHANGED;
        DEBUG((DEBUG_INFO, "Delaying Processing until EndOfDxe\n"));
    }

    return Status;
}

/**
 * Queue ProcessMailbox to run after SetingAccess published
 *
 * Halt process of mailboxes until SettingAccess protocol is published.  Since UnEnrol calls SettingAccess,
 * it must wait for Setting Access.
 *
 * @param
 *
 * @return EFI_STATUS
 */
EFI_STATUS
QueueMailboxAtSettingAccess () {
    EFI_STATUS Status;
    EFI_EVENT  Event;
    VOID      *NotUsed;

    Status = gBS->LocateProtocol (&gDfciSettingAccessProtocolGuid,
                                  NULL,
                                  (VOID **)&NotUsed
                                 );
    if (EFI_ERROR(Status)) {
        Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL,
                                   TPL_CALLBACK,
                                   SettingAccessCallback,
                                   NULL,
                                   &Event
                                  );
        if (EFI_ERROR (Status)) {
            DEBUG((DEBUG_INFO, "[DM] %a: Failed to create SettingAccess registration event (%r).\r\n", __FUNCTION__, Status));
        } else {
            Status = gBS->RegisterProtocolNotify (&gDfciSettingAccessProtocolGuid,
                                                   Event,
                                                  &NotUsed
                                                 );
            if (EFI_ERROR (Status)) {
                DEBUG((DEBUG_INFO,  "[DM] %a: Failed to register for Setting Access notifications (%r).\r\n", __FUNCTION__, Status));
            } else {
                Status = EFI_MEDIA_CHANGED;
            }
        }
    }

    return Status;
}

/**
 * Apply the packet
 *
 *
 * @param Data
 * @param ApplyProtocol
 *
 * @return EFI_STATUS
 */
EFI_STATUS
ProcessApplyPacket (
    IN DFCI_INTERNAL_PACKET    *Data,
    DFCI_APPLY_PACKET_PROTOCOL *ApplyProtocol
) {
    EFI_STATUS Status;


    DEBUG((DEBUG_INFO, "[DM] Dfci Manager - Processing Apply Packet for %s.\n", Data->MailboxName));
    Status = ApplyProtocol->ApplyPacket(ApplyProtocol, Data);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "[DM] %a: Error applying packet for variable %s - %r\n", __FUNCTION__, Data->MailboxName, Status));
    } else {
        // Only Identity packets can set DFCI_PACKET_STATE_DATA_DELAYED_PROCESSING
        if (Data->State == DFCI_PACKET_STATE_DATA_DELAYED_PROCESSING) {
            Data->State = DFCI_PACKET_STATE_DATA_PRESENT;
            Status = QueueMailboxAtEndOfDxe();
        }

        if (Data->ResetRequired) {
            mRebootRequired = TRUE;
        }
    }
    return Status;
}

/**
 * Initialize Packet
 *
 * Initialize static packet information.
 *
 *
 * @param VariableName
 * @param ResultName
 * @param NameSpace
 * @param HdrSignature
 * @param HdrVersion
 * @param Decoder
 * @param ApplyProtocol
 * @param ManagerData
 *
 * @return EFI_STATUS
 */
EFI_STATUS
InitializePacket (IN     CHAR16                     *VariableName,
                  IN     CHAR16                     *ResultName,
                  IN     EFI_GUID                   *NameSpace,
                  IN     UINT32                      HdrSignature,
                  IN     UINT8                       HdrVersion,
                  IN     DECODE_PACKET               Decoder,
                  IN     DFCI_APPLY_PACKET_PROTOCOL *ApplyProtocol,
                  IN OUT DFCI_MANAGER_DATA          *MgrData
  ) {
    EFI_STATUS  Status;


    if (MgrData == NULL) {
      DEBUG((DEBUG_ERROR, "[DM] %a: ManagerData not provided\n", __FUNCTION__));
      return EFI_INVALID_PARAMETER;
    }

    MgrData->Data = AllocateZeroPool (sizeof(DFCI_INTERNAL_PACKET));
    if (MgrData->Data == NULL) {
        DEBUG((DEBUG_INFO, "[DM] Dfci Manager out of resources.\n"));
        return EFI_OUT_OF_RESOURCES;
    }

    MgrData->Data->MailboxName = VariableName;
    MgrData->Data->ResultName = ResultName;
    MgrData->Data->NameSpace = NameSpace;
    MgrData->Data->Expected.Signature = HdrSignature;
    MgrData->Data->Expected.Version = HdrVersion;
    MgrData->ApplyProtocol = ApplyProtocol;

    //Get the Mailbox variable
    Status = GetVariable2(MgrData->Data->MailboxName,
                          MgrData->Data->NameSpace,
                         &MgrData->Data->Packet,
                         &MgrData->Data->PacketSize);

    if (EFI_ERROR(Status)) {
        MgrData->Data->StatusCode = Status;
        MgrData->DecodeStatus = Status;
        if (Status == EFI_NOT_FOUND) {
            DEBUG((DEBUG_INFO, "[DM] Dfci Manager - No Pending Data for %s.\n", MgrData->Data->MailboxName));
        } else {
            DEBUG((DEBUG_ERROR, "[DM] %a: Error getting variable %s - %r\n", __FUNCTION__, MgrData->Data->MailboxName, Status));
        }
        MgrData->Data->Packet = NULL;
    } else {
        // Decode the packet
        Status = Decoder(MgrData->Data);
        MgrData->DecodeStatus = Status;

        DEBUG((DEBUG_INFO, "[DM] Dfci Manager - Processing queued for %s - %r\n", MgrData->Data->MailboxName, Status));
    }

    return EFI_SUCCESS;
}

/**
 * Allocate Manager Data
 *
 * @param
 *
 * @return EFI_STATUS
 */
EFI_STATUS
AllocateManagerData (VOID) {
    EFI_STATUS  Status;


    Status = InitializePacket (DFCI_IDENTITY_APPLY_VAR_NAME,
                               DFCI_IDENTITY_RESULT_VAR_NAME,
                              &gDfciAuthProvisionVarNamespace,
                               DFCI_IDENTITY_APPLY_VAR_SIGNATURE,
                               DFCI_IDENTITY_VAR_VERSION,
                               DecodeIdentityPacket,
                               mApplyIdentityProtocol,
                              &mManagerData[MGR_IDENTITY]);

    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = InitializePacket(DFCI_PERMISSION_POLICY_APPLY_VAR_NAME,
                               DFCI_PERMISSION_POLICY_RESULT_VAR_NAME,
                              &gDfciPermissionManagerVarNamespace,
                               DFCI_PERMISSION_POLICY_APPLY_VAR_SIGNATURE,
                               DFCI_PERMISSION_POLICY_VAR_VERSION,
                               DecodePacket,
                               mApplyPermissionsProtocol,
                              &mManagerData[MGR_PERMISSIONS]);

    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = InitializePacket (DFCI_IDENTITY2_APPLY_VAR_NAME,
                               DFCI_IDENTITY2_RESULT_VAR_NAME,
                              &gDfciAuthProvisionVarNamespace,
                               DFCI_IDENTITY_APPLY_VAR_SIGNATURE,
                               DFCI_IDENTITY_VAR_VERSION,
                               DecodeIdentityPacket,
                               mApplyIdentityProtocol,
                              &mManagerData[MGR_IDENTITY2]);

    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = InitializePacket (DFCI_PERMISSION2_POLICY_APPLY_VAR_NAME,
                               DFCI_PERMISSION2_POLICY_RESULT_VAR_NAME,
                              &gDfciPermissionManagerVarNamespace,
                               DFCI_PERMISSION_POLICY_APPLY_VAR_SIGNATURE,
                               DFCI_PERMISSION_POLICY_VAR_VERSION,
                               DecodePacket,
                               mApplyPermissionsProtocol,
                              &mManagerData[MGR_PERMISSIONS2]);

    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = InitializePacket (DFCI_SETTINGS_APPLY_INPUT_VAR_NAME,
                               DFCI_SETTINGS_APPLY_OUTPUT_VAR_NAME,
                              &gDfciSettingsManagerVarNamespace,
                               DFCI_SECURED_SETTINGS_APPLY_VAR_SIGNATURE,
                               DFCI_SECURED_SETTINGS_VAR_VERSION,
                               DecodePacket,
                               mApplySettingsProtocol,
                              &mManagerData[MGR_SETTINGS]);

    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = InitializePacket (DFCI_SETTINGS2_APPLY_INPUT_VAR_NAME,
                               DFCI_SETTINGS2_APPLY_OUTPUT_VAR_NAME,
                              &gDfciSettingsManagerVarNamespace,
                               DFCI_SECURED_SETTINGS_APPLY_VAR_SIGNATURE,
                               DFCI_SECURED_SETTINGS_VAR_VERSION,
                               DecodePacket,
                               mApplySettingsProtocol,
                              &mManagerData[MGR_SETTINGS2]);
    return Status;
}

/**
 * Free Manager Data
 *
 * Normal - just discard varialbe data - will be retrieved again
 * Complete - Discard all ManagerData
 *
 * @param
 *
 * @return EFI_STATUS
 */
EFI_STATUS
FreeManagerData (VOID) {


    // Free Internal Data structures
    for (UINTN i=0; i < MGR_MAX; i++) {
        if (mManagerData[i].Data != NULL) {
            if (mManagerData[i].Data->Packet != NULL) {
                FreePool (mManagerData[i].Data->Packet);
                mManagerData[i].Data->Packet = NULL;
            }

            FreePool(mManagerData[i].Data);
            mManagerData[i].Data = NULL;
        }
    }
    return EFI_SUCCESS;
}

/**
 * Process Packet
 *
 * Get the MailBox packet.  If not present, Done.
 *
 *
 * @param MgrData
 *
 * @return EFI_STATUS
 */
EFI_STATUS
ProcessPacket (IN DFCI_MANAGER_DATA *MgrData
  ) {
    DFCI_INTERNAL_PACKET  *Data;
    EFI_STATUS             Status;


    Data = MgrData->Data;

    if (Data->Packet == NULL) {
        DEBUG((DEBUG_INFO, "[DM] Process Packet - No pending Data for %s.\n", Data->MailboxName));
        return EFI_SUCCESS;
    }

    DEBUG((DEBUG_INFO, "[DM] Process Packet - Processing pending Data for %s.\n", Data->MailboxName));

    // Get the results from the packet decode
    Status = MgrData->DecodeStatus;

    // Apply the packet, unless it is an Identity Unenroll packet.
    if (!EFI_ERROR(Status)) {
        if (!Data->Unenroll) {
            Status = ProcessApplyPacket (Data, MgrData->ApplyProtocol);
        }
    }

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "[DM] Process Packet failed for %s. Code=%r\n", Data->MailboxName, Status));
    }

    return Status;
}

/**
 * Process UnEnroll Packet
 *
 * Process the UnEnroll Packet.
 *
 * @param MgrData
 *
 * @return EFI_STATUS
 */
EFI_STATUS
ProcessUnEnrollPacket (IN DFCI_MANAGER_DATA *MgrData
  ) {
    DFCI_INTERNAL_PACKET  *Data;
    EFI_STATUS             Status;


    Status = ProcessPacket (MgrData);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "[DM] %a: Error processing unenroll. Code=%r\n", __FUNCTION__, Status));
    } else {
        Data = MgrData->Data;
        if (Data->Packet != NULL) {
            if (Data->Unenroll) {
                Status = QueueMailboxAtSettingAccess ();
                DEBUG((DEBUG_INFO, "[DM] QueueMailboxAtSettingsAccess - code=%r\n",Status));
                if (Status == EFI_MEDIA_CHANGED) {
                    return Status;
                }

                if (!EFI_ERROR(Status)) {
                    Status = ProcessApplyPacket (Data, mApplyIdentityProtocol);
                    DEBUG((DEBUG_INFO, "[DM] Applied Packet, code=%r, state=%d\n", Data->StatusCode, Data->State));
                    if (Status == EFI_MEDIA_CHANGED) {
                        return Status;
                    } else if (EFI_ERROR(Status)) {
                        DEBUG((DEBUG_ERROR, "[DM] %a: Error applying results for variable %s - %r\n", __FUNCTION__, Data->ResultName, Status));
                    }
                }

                mRebootRequired = TRUE;
            } else {
                DEBUG((DEBUG_INFO, "[DM] Invalid internal state. Should never have Enroll here.\n"));
                ASSERT(FALSE);
            }
        }
    }
    return Status;
}

/**
 * Complete the packet processing by sending the Lkg operation and result code
 *
 * @param Data
 * @param ApplyStatus
 *
 * @return EFI_STATUS
 */
EFI_STATUS
CompletePacket(
    IN  DFCI_MANAGER_DATA  *MgrData,
    IN  EFI_STATUS          ApplyStatus
  ) {
    DFCI_INTERNAL_PACKET  *Data;
    UINT8                  LkgOperation;
    EFI_STATUS             Status;


    Data = MgrData->Data;

    // If no packet processed, no packet to complete
    if (Data->Packet == NULL) {
        return EFI_SUCCESS;
    }

    if (EFI_ERROR(ApplyStatus)) {
        LkgOperation = DFCI_LKG_RESTORE;
    } else {
        LkgOperation = DFCI_LKG_COMMIT;
    }

    // For aborted operations that have not been applied, supply an aborted state and error code
    if ((LkgOperation == DFCI_LKG_RESTORE) && (Data->State == DFCI_PACKET_STATE_UNINITIALIZED)) {
        Data->State = DFCI_PACKET_STATE_ABORTED;
        Data->StatusCode = ApplyStatus;
    }

    DEBUG((DEBUG_INFO, "[DM] Dfci Manager - CompletePacket for %s, Lkg=%d,State=%d, Code=%r.\n", Data->MailboxName, LkgOperation, Data->State, Data->StatusCode));

    Status = MgrData->ApplyProtocol->Lkg(MgrData->ApplyProtocol, Data, LkgOperation);
    if (!EFI_ERROR(Status)) {

        Status = MgrData->ApplyProtocol->ApplyResult(MgrData->ApplyProtocol, Data);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "[DM] %a: Error applying results for variable %s - %r\n", __FUNCTION__, Data->ResultName, Status));
        }

    } else {
        DEBUG((DEBUG_ERROR, "[DM] %a: Error completing Lkg for packet variable %s - %r\n", __FUNCTION__, Data->ResultName, Status));
    }

    return Status;
}

/**
 * Process DFCI Mailboxes
 *
 * Process Mailboxes is called in mainline entry, or on a deferred callback at END_OF_DXE
 *
 * @param
 *
 * @return EFI_STATUS
 */
EFI_STATUS
ProcessMailBoxes (
) {
    EFI_STATUS         LkgStatus;
    DFCI_MANAGER_DATA *MgrData;
    EFI_STATUS         Status;


    DEBUG((DEBUG_INFO, "[DM] %a: ProcessMailboxes Entry\n", __FUNCTION__));

    // Process all packets in this order.  If any identity packet state is set to
    // DFCI_PACKET_STATE_DATA_DELAYED_PROCESSING, ProcessPacket will register an EndOfDxe
    // handler and return EFI_MEDIA_CHANGED.
    // This error will cause all subsequent packets to be skipped until EndOfDxe.

    LkgStatus = EFI_SUCCESS;
    MgrData = &mManagerData[MGR_IDENTITY];
    Status = ProcessPacket (MgrData);

    if (EFI_ERROR(Status)) {
        if (Status == EFI_MEDIA_CHANGED) {
            goto EARLY_EXIT;
        } else {
            LkgStatus = Status;
            // For an error case, all subsequest identity and permissions operations are void
            goto COMPLETE_AS_FAILED;
        }
    }

    Status = ProcessPacket(&mManagerData[MGR_PERMISSIONS]);

    if (EFI_ERROR(Status)) {
        LkgStatus = Status;
        // For an error case, all subsequest identity and permissions operations are void
        goto COMPLETE_AS_FAILED;
    }

    MgrData = &mManagerData[MGR_IDENTITY2];
    Status = ProcessPacket (MgrData);

    if (EFI_ERROR(Status)) {
        if (Status == EFI_MEDIA_CHANGED) {
            goto EARLY_EXIT;
        } else {
            LkgStatus = Status;
            // For an error case, all subsequest identity and permissions operations are void
            goto COMPLETE_AS_FAILED;
        }
    }

    Status = ProcessPacket(&mManagerData[MGR_PERMISSIONS2]);

    if (EFI_ERROR(Status)) {
        LkgStatus = Status;
        // For an error case, all subsequest identity and permissions operations are void
        goto COMPLETE_AS_FAILED;
    }

COMPLETE_AS_FAILED:

    // CompletePacket is a no-op for no mailbox entry.  So, any mailbox that has
    // an operation will be sent an LKG.  If both mailboxes of a pair were processed,
    // the first commit will commit both, and the subsequent commit will be a no-op.
    CompletePacket (&mManagerData[MGR_PERMISSIONS2], LkgStatus);
    CompletePacket (&mManagerData[MGR_PERMISSIONS], LkgStatus);

    MgrData = &mManagerData[MGR_IDENTITY];
    if (!MgrData->Data->Unenroll) {
        CompletePacket(MgrData, LkgStatus);
    }

    MgrData = &mManagerData[MGR_IDENTITY2];
    if (!MgrData->Data->Unenroll) {
        CompletePacket(MgrData, LkgStatus);
    }

    // Settings are completely processed - no delays added, no "LKG_RESTORE" possible
    ProcessPacket (&mManagerData[MGR_SETTINGS]);

    // Actual LKG ignored for Settings
    CompletePacket(&mManagerData[MGR_SETTINGS], EFI_SUCCESS);

    ProcessPacket (&mManagerData[MGR_SETTINGS2]);

    // Actual LKG ignored for Settings
    CompletePacket(&mManagerData[MGR_SETTINGS2], EFI_SUCCESS);

    // Now, re-process the identity mailboxes for possible un-enroll operations

    MgrData = &mManagerData[MGR_IDENTITY2];
    Status = ProcessUnEnrollPacket (MgrData);
    if (Status == EFI_MEDIA_CHANGED) {
        goto EARLY_EXIT;
    }

    CompletePacket(MgrData, Status);

    MgrData = &mManagerData[MGR_IDENTITY];
    Status = ProcessUnEnrollPacket (MgrData);
    if (Status == EFI_MEDIA_CHANGED) {
        goto EARLY_EXIT;
    }

    CompletePacket(MgrData, Status);

    DEBUG((DEBUG_INFO, "[DM] ProcessMailboxes Final Exit\n"));

    FreeManagerData ();

    if (mRebootRequired) {
        gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
    }

    return Status;

EARLY_EXIT:

    DEBUG((DEBUG_INFO, "[DM] ProcessMailboxes Early Exit\n"));

    return Status;
}

/**
  Entry to DfciManager.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.
  @return Status         From internal routine or boot object

 **/
EFI_STATUS
EFIAPI
DfciManagerEntry(
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
    ) {
    EFI_STATUS  Status = EFI_DEVICE_ERROR;


    PERF_FUNCTION_BEGIN (PERF_VERBOSITY_STANDARD);

    Status = gBS->LocateProtocol(&gDfciApplyIdentityProtocolGuid, NULL, &mApplyIdentityProtocol);
    if (EFI_ERROR(Status) || NULL == mApplyIdentityProtocol) {
        DEBUG((DEBUG_ERROR, "[DM] %a: Cannot find Apply Identity Protocol.\n", __FUNCTION__));
        ASSERT(FALSE);
        goto ERROR_EXIT;
    }

    Status = gBS->LocateProtocol(&gDfciApplyPermissionsProtocolGuid, NULL, &mApplyPermissionsProtocol);
    if (EFI_ERROR(Status) || NULL == mApplyPermissionsProtocol) {
        DEBUG((DEBUG_ERROR, "[DM] %a: Cannot find Apply Permission Protocol.\n", __FUNCTION__));
        ASSERT(FALSE);
        goto ERROR_EXIT;
    }

    Status = gBS->LocateProtocol(&gDfciApplySettingsProtocolGuid, NULL, &mApplySettingsProtocol);
    if (EFI_ERROR(Status) || NULL == mApplySettingsProtocol) {
        DEBUG((DEBUG_ERROR, "[DM] %a: Cannot find Apply Settings Protocol.\n", __FUNCTION__));
        ASSERT(FALSE);
        goto ERROR_EXIT;
    }

    Status = AllocateManagerData ();

    if (EFI_ERROR(Status)) {
        goto ERROR_EXIT;
    }

    //
    // Request notification of EndOfDxe.
    //
    // NOTE: If this fails, the provisioning instance data object is left in the
    //       DELAYED_PROCESSING state.
    //
    Status = gBS->CreateEventEx(EVT_NOTIFY_SIGNAL,
                                TPL_CALLBACK,
                                EndOfDxeCallback,
                                NULL,
                                &gEfiEndOfDxeEventGroupGuid,
                                &mEndOfDxeEvent );

    if (EFI_ERROR( Status )) {
      DEBUG(( DEBUG_ERROR, "[DM] %a: EndOfDxe callback registration failed! %r\n", __FUNCTION__, Status ));
      goto ERROR_EXIT;
    } else {
        Status = ProcessMailBoxes ();
        DEBUG((DEBUG_INFO, "[DM] %a: Processing mailbox complete. Code = %r.\n", __FUNCTION__, Status));

        if (Status != EFI_MEDIA_CHANGED) {
            gBS->CloseEvent(mEndOfDxeEvent);
        }
        Status = EFI_SUCCESS;
    }

    if (EFI_ERROR(Status)) {
        goto ERROR_EXIT;
    }

    PERF_FUNCTION_END (PERF_VERBOSITY_STANDARD);

    return EFI_SUCCESS;

ERROR_EXIT:

    DEBUG((DEBUG_ERROR, "[DM] %a: Exiting with error. Code = %r\n",Status));

    FreeManagerData ();

    PERF_FUNCTION_END (PERF_VERBOSITY_STANDARD);

    return EFI_SUCCESS;
}


