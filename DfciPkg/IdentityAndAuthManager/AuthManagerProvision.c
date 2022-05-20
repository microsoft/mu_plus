/**@file
AuthManagerProvision.c

Processes new Identity Packets

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "IdentityAndAuthManager.h"
#include <Guid/EventGroup.h>
#include <Guid/DfciPacketHeader.h>
#include <Guid/DfciIdentityAndAuthManagerVariables.h>
#include <Guid/ZeroGuid.h>
#include <Private/DfciGlobalPrivate.h>
#include <Library/BaseLib.h>
#include <Library/DfciDeviceIdSupportLib.h>
#include <Settings/DfciSettings.h>
#include <Settings/DfciPrivateSettings.h>

/**
 Convert the Identity values used in the Provisioning Variable
 to the Identity values used by the Authentication Manager
**/
DFCI_IDENTITY_ID
VarIdentityToDfciIdentity (
  IN UINT8  VarIdentity
  )
{
  if (VarIdentity == DFCI_SIGNER_PROVISION_IDENTITY_ZTD) {
    return DFCI_IDENTITY_SIGNER_ZTD;
  }

  if (VarIdentity == DFCI_SIGNER_PROVISION_IDENTITY_OWNER) {
    return DFCI_IDENTITY_SIGNER_OWNER;
  }

  if (VarIdentity == DFCI_SIGNER_PROVISION_IDENTITY_USER) {
    return DFCI_IDENTITY_SIGNER_USER;
  }

  if (VarIdentity == DFCI_SIGNER_PROVISION_IDENTITY_USER1) {
    return DFCI_IDENTITY_SIGNER_USER1;
  }

  if (VarIdentity == DFCI_SIGNER_PROVISION_IDENTITY_USER2) {
    return DFCI_IDENTITY_SIGNER_USER2;
  }

  return DFCI_IDENTITY_INVALID;
}

UINT8
DfciIdentityToVarIdentity (
  IN DFCI_IDENTITY_ID  DfciIdentity
  )
{
  if (DfciIdentity == DFCI_IDENTITY_SIGNER_ZTD ) {
    return DFCI_SIGNER_PROVISION_IDENTITY_ZTD;
  }

  if (DfciIdentity == DFCI_IDENTITY_SIGNER_OWNER) {
    return DFCI_SIGNER_PROVISION_IDENTITY_OWNER;
  }

  if (DfciIdentity == DFCI_IDENTITY_SIGNER_USER) {
    return DFCI_SIGNER_PROVISION_IDENTITY_USER;
  }

  if (DfciIdentity == DFCI_IDENTITY_SIGNER_USER1) {
    return DFCI_SIGNER_PROVISION_IDENTITY_USER1;
  }

  if (DfciIdentity == DFCI_IDENTITY_SIGNER_USER2) {
    return DFCI_SIGNER_PROVISION_IDENTITY_USER2;
  }

  return DFCI_SIGNER_PROVISION_IDENTITY_INVALID;
}

DFCI_SETTING_ID_STRING
DfciIdentityToSettingId (
  IN DFCI_IDENTITY_ID  Identity
  )
{
  if (Identity == DFCI_IDENTITY_SIGNER_ZTD ) {
    return DFCI_PRIVATE_SETTING_ID__ZTD_KEY;
  }

  if (Identity == DFCI_IDENTITY_SIGNER_USER) {
    return DFCI_PRIVATE_SETTING_ID__USER_KEY;
  }

  if (Identity == DFCI_IDENTITY_SIGNER_OWNER) {
    return DFCI_PRIVATE_SETTING_ID__OWNER_KEY;
  }

  if (Identity == DFCI_IDENTITY_SIGNER_USER1) {
    return DFCI_PRIVATE_SETTING_ID__USER1_KEY;
  }

  if (Identity == DFCI_IDENTITY_SIGNER_USER2) {
    return DFCI_PRIVATE_SETTING_ID__USER2_KEY;
  }

  return NULL;
}

/**
Write the provisioning response variable with parameter info
**/
EFI_STATUS
EFIAPI
SetIdentityResponse (
  IN  CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
  IN DFCI_INTERNAL_PACKET               *Data
  )
{
  EFI_STATUS                        Status;
  DFCI_SIGNER_PROVISION_RESULT_VAR  Var;

  if (Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Don't want to write a status when we didn't have any data
  //
  if (Data->State == DFCI_PACKET_STATE_UNINITIALIZED) {
    return EFI_SUCCESS;
  }

  // If user confirmation pending..don't write status as this should be run again once user input is enabled
  if (Data->State == DFCI_PACKET_STATE_DATA_DELAYED_PROCESSING) {
    return EFI_SUCCESS;
  }

  Var.Header.Hdr.Signature = DFCI_IDENTITY_RESULT_VAR_SIGNATURE;
  Var.Header.Version       = DFCI_IDENTITY_RESULT_VERSION;
  Var.Identity             = DfciIdentityToVarIdentity (Data->DfciIdentity);
  DEBUG ((DEBUG_INFO, "%a - Set Result Var Identity 0x%X.  DFCI Identity 0x%X\n", __FUNCTION__, Var.Identity, Data->DfciIdentity));
  Var.StatusCode = (UINT64)(Data->StatusCode);
  Var.SessionId  = Data->SessionId;

  Status = gRT->SetVariable (
                  (CHAR16 *)Data->ResultName,
                  &gDfciAuthProvisionVarNamespace,
                  DFCI_IDENTITY_VAR_ATTRIBUTES,
                  sizeof (DFCI_SIGNER_PROVISION_RESULT_VAR),
                  &Var
                  );
  return Status;
}

/**
 * Perform basic checks on packet
 *
 * @param Data
 * @param SettingsPermissionProtocol
 *
 * @return EFI_STATUS
 */
EFI_STATUS
ValidateAndAuthenticatePendingProvisionData (
  IN       DFCI_INTERNAL_PACKET               *Data,
  IN CONST DFCI_SETTING_PERMISSIONS_PROTOCOL  *SettingsPermissionProtocol
  )
{
  EFI_STATUS                Status;
  DFCI_PERMISSION_MASK      PermMask            = 0;
  DFCI_PERMISSION_MASK      ZtdUnenrollPermMask = 0;
  DFCI_IDENTITY_PROPERTIES  Properties;
  WIN_CERTIFICATE           *TestSignature = NULL;
  WIN_CERTIFICATE           *Signature     = NULL;
  UINTN                     SignedDataLength;

  SignedDataLength = Data->SignedDataLength;

  Data->DfciIdentity = VarIdentityToDfciIdentity (*Data->VarIdentity);  // Set the Identity

  // Check the Identity to make sure it's supported
  if (Data->DfciIdentity == DFCI_IDENTITY_INVALID) {
    DEBUG ((DEBUG_ERROR, "%a - Identity is not supported 0x%X\n", __FUNCTION__, Data->DfciIdentity));
    Data->StatusCode = EFI_UNSUPPORTED;
    Data->State      = DFCI_PACKET_STATE_DATA_INVALID;
    return Data->StatusCode;
  }

  // Lets check that the auth packet is either Owner Identity or that an Owner already exists.
  // Can't provision User Key without Owner key already provisioned.
  if ((*Data->VarIdentity != DFCI_SIGNER_PROVISION_IDENTITY_OWNER) &&
      (!(Provisioned () & DFCI_IDENTITY_SIGNER_OWNER)))
  {
    DEBUG ((DEBUG_ERROR, "[AM] - Can't provision User Auth Packet when Owner auth isn't already provisioned.\n"));
    Data->StatusCode = EFI_UNSUPPORTED;
    Data->State      = DFCI_PACKET_STATE_DATA_NO_OWNER;
    return Data->StatusCode;
  }

  // Lets check that if its an unenroll packet, that identity must be enrolled already.
  if ((Data->PayloadSize == 0) && ((Data->DfciIdentity & Provisioned ()) == 0)) {
    DEBUG ((DEBUG_ERROR, "[AM] %a - Can't un-enroll a device that isn't enrolled in DFCI (no owner).\n", __FUNCTION__));
    Data->StatusCode = EFI_UNSUPPORTED;
    Data->State      = DFCI_PACKET_STATE_DATA_NO_OWNER;
    return Data->StatusCode;
  }

  // Lets check the test signature
  // - this is to confirm new Cert Data (Trusted Cert) isn't in bad format (user/tool error) which would
  //   cause future validation errors and possible "brick"
  // - This is not present when this is a unenroll request (no New Trusted Cert)
  //
  if (Data->PayloadSize > 0) {
    if (Data->PacketSize <= (SignedDataLength + sizeof (WIN_CERTIFICATE))) {
      // Invalid....Where the signature data???
      DEBUG ((DEBUG_ERROR, "[AM] %a - Variable isn't big enough to hold any signature data\n", __FUNCTION__));
      Data->StatusCode = EFI_COMPROMISED_DATA;
      Data->State      = DFCI_PACKET_STATE_DATA_INVALID;
      return Data->StatusCode;
    }

    // Now we can check if we have a WIN_CERT
    TestSignature = (WIN_CERTIFICATE *)PKT_FIELD_FROM_OFFSET (Data->Packet, SignedDataLength);
    // check Test Signature length
    if ((TestSignature->dwLength + SignedDataLength) > Data->PacketSize) {
      // Invalid....Where the signature data???
      DEBUG ((DEBUG_ERROR, "[AM] %a - Variable isn't big enough to hold the declared test signature data\n", __FUNCTION__));
      Data->StatusCode = EFI_COMPROMISED_DATA;
      Data->State      = DFCI_PACKET_STATE_DATA_INVALID;
      return Data->StatusCode;
    }

    // Check the test signature
    Status = VerifySignature (Data->Payload, Data->PayloadSize, TestSignature, Data->Payload, Data->PayloadSize);
    if (EFI_ERROR (Status)) {
      // Test Signature Fails Validation
      DEBUG ((DEBUG_ERROR, "[AM] %a - Test Signature Failed Validation.  %r\n", __FUNCTION__, Status));
      Data->StatusCode = EFI_CRC_ERROR;  // special return code for this case.  Probably should create a new status code
      Data->State      = DFCI_PACKET_STATE_DATA_INVALID;
      return Data->StatusCode;
    }

    DEBUG ((DEBUG_INFO, "[AM] Test Signature passed Validation.\n"));
    SignedDataLength += TestSignature->dwLength;  // Update the SignedDataLength based on valid Signature Length
  }

  // Check Signed Data length vs variable Length
  DEBUG ((DEBUG_INFO, "[AM] %a - SignedDataLength = 0x%X\n", __FUNCTION__, SignedDataLength));
  if ((SignedDataLength + sizeof (WIN_CERTIFICATE_UEFI_GUID)) >= Data->PacketSize) {
    // Where is the cert data?
    DEBUG ((DEBUG_ERROR, "[AM] %a - Variable isn't big enough to hold the declared var signature data\n", __FUNCTION__));
    Data->StatusCode = EFI_COMPROMISED_DATA;
    Data->State      = DFCI_PACKET_STATE_DATA_INVALID;
    return Data->StatusCode;
  }

  // Get Permissions for this provisioned data
  Status = SettingsPermissionProtocol->GetPermission (SettingsPermissionProtocol, DfciIdentityToSettingId (Data->DfciIdentity), &PermMask);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to get Permission for Identity 0x%X.  Status = %r\n", __FUNCTION__, Data->DfciIdentity, Status));
    Data->StatusCode = Status;
    Data->State      = DFCI_PACKET_STATE_DATA_INVALID;
    return Data->StatusCode;
  }

  DEBUG ((DEBUG_INFO, "%a - Permission for 0x%2.2x, %a, is 0x%X\n", __FUNCTION__, Data->DfciIdentity, DfciIdentityToSettingId (Data->DfciIdentity), PermMask));

  Signature = (WIN_CERTIFICATE *)PKT_FIELD_FROM_OFFSET (Data->Packet, SignedDataLength);

  // Check to make sure Signature is contained within Var Data
  if (Data->PacketSize != (Signature->dwLength + SignedDataLength)) {
    // Var Length isn't correct
    DEBUG ((DEBUG_ERROR, "[AM] %a - Variable Size (0x%X) doesn't match calculated size (0x%X)\n", __FUNCTION__, Data->PacketSize, (Signature->dwLength + SignedDataLength)));
    Data->StatusCode = EFI_COMPROMISED_DATA;
    Data->State      = DFCI_PACKET_STATE_DATA_INVALID;
    return Data->StatusCode;
  }

  // ALL WIN_CERT Support and Verification is handled in the Auth Protocol

  // Now lets ask the auth manager to verify
  Status = AuthWithSignedData (
             &mAuthProtocol,
             (UINT8 *)Data->Packet, // signed data ptr
             SignedDataLength,      // signed data length
             Signature,             // Win Cert ptr
             &(Data->AuthToken)
             );

  if (!EFI_ERROR (Status)) {
    // Success.  now get Identity
    Status = GetIdentityProperties (&mAuthProtocol, &(Data->AuthToken), &Properties);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "%a - Auth Passed but Identity failed. Should never happen. %r\n", Status));
      Data->StatusCode = EFI_ABORTED;
      Data->State      = DFCI_PACKET_STATE_DATA_AUTH_FAILED;
      return Data->StatusCode;
    }

    // Handle UnEnroll via ZTD signed differently.
    if ((Data->PayloadSize == 0) && (Properties.Identity == DFCI_IDENTITY_SIGNER_ZTD)) {
      // Get Permissions Dfci.ZtdUnenroll.Enable
      Status = SettingsPermissionProtocol->GetPermission (SettingsPermissionProtocol, DFCI_PRIVATE_SETTING_ID__ZTD_UNENROLL, &ZtdUnenrollPermMask);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a - Failed to get Permission for Identity 0x%X.  Status = %r\n", __FUNCTION__, Data->DfciIdentity, Status));
        Data->StatusCode = Status;
        Data->State      = DFCI_PACKET_STATE_DATA_INVALID;
        return Data->StatusCode;
      }

      // If the cert being unenrolled is allowed by ZtdUnenroll, allow the unenroll.
      if ((Data->DfciIdentity & ZtdUnenrollPermMask) != 0) {
        // Permission is good.  Apply
        DEBUG ((DEBUG_INFO, "%a - Permission by Ztd Unenroll is good. Applying without requiring user interaction.\n", __FUNCTION__));
        Data->UserConfirmationRequired = FALSE;
        Data->StatusCode               = EFI_SUCCESS;
        Data->State                    = DFCI_PACKET_STATE_DATA_AUTHENTICATED;
        return Data->StatusCode;
      }
    } else {
      if ((Properties.Identity & PermMask) != 0) {
        // Permission is good.  Apply
        DEBUG ((DEBUG_INFO, "%a - Permission is good. Applying without requiring user interaction.\n", __FUNCTION__));
        Data->UserConfirmationRequired = FALSE;
        Data->StatusCode               = EFI_SUCCESS;
        Data->State                    = DFCI_PACKET_STATE_DATA_AUTHENTICATED;
        return Data->StatusCode;
      }
    }

    // Auth was good but Permission wasn't
    DEBUG ((DEBUG_INFO, "%a - Auth Good but Permission not set for this identity\n", __FUNCTION__));
  }

  // Auth wasn't good enough
  DEBUG ((DEBUG_INFO, "%a - Crypto Supplied Auth wasn't enough.\n", __FUNCTION__));
  if ((PermMask & DFCI_IDENTITY_LOCAL) != 0) {
    DEBUG ((DEBUG_INFO, "%a - Local User Auth allowed.  Will prompt for User approval.\n", __FUNCTION__));
    Data->UserConfirmationRequired = TRUE;
    Data->StatusCode               = EFI_SUCCESS;
    Data->State                    = DFCI_PACKET_STATE_DATA_AUTHENTICATED;
    return Data->StatusCode;
  }

  // UNKNOWN ERROR
  // FAIL - Unsupported Identity
  DEBUG ((DEBUG_INFO, "%a - Unsupported Key Provision\n", __FUNCTION__));
  Data->StatusCode = EFI_ACCESS_DENIED;
  Data->State      = DFCI_PACKET_STATE_DATA_AUTH_FAILED;
  return Data->StatusCode;
}

/**
Function sets the new data into the Internal Cert Store and save to NV ram
**/
EFI_STATUS
EFIAPI
ApplyProvisionData (
  IN DFCI_INTERNAL_PACKET  *Data
  )
{
  UINT8       Index        = CERT_INVALID_INDEX;
  UINT8       *NewCertData = NULL;
  EFI_STATUS  Status;

  if (Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->State != DFCI_PACKET_STATE_DATA_USER_APPROVED) {
    DEBUG ((DEBUG_ERROR, "ApplyProvisionData called with data in wrong state 0x%x\n", Data->State));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "Applying Provision Data for Identity %d\n", Data->DfciIdentity));

  // Special case for when a user is unenrolling in DFCI - which is done by removing owner key
  if ((Data->PayloadSize == 0) && (Data->DfciIdentity == DFCI_IDENTITY_SIGNER_OWNER)) {
    Status = ClearDFCI (&(Data->AuthToken));

    Data->ResetRequired = TRUE;  // After clear force reboot...even in error case
    Data->StatusCode    = Status;
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[AM] - Failed to Clear DFCI.  System in bad state. %r\n", Status));
      Data->State = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR;
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    Data->State = DFCI_PACKET_STATE_DATA_COMPLETE;
    return Status;
  }

  Index = DfciIdentityToCertIndex (Data->DfciIdentity);
  if (Index == CERT_INVALID_INDEX) {
    DEBUG ((DEBUG_INFO, "Invalid Cert Index\n"));
    Data->State      = DFCI_PACKET_STATE_DATA_INVALID;
    Data->StatusCode = EFI_UNSUPPORTED;
    return Data->StatusCode;
  }

  // Only allocate new memory if this request has new cert data
  if (Data->PayloadSize > 0) {
    // allocate new data just in case of error we will not delete old yet
    NewCertData = AllocatePool (Data->PayloadSize);
    if (NewCertData == NULL) {
      Data->State      = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR;
      Data->StatusCode = EFI_OUT_OF_RESOURCES;
      return EFI_OUT_OF_RESOURCES;
    }
  }

  // Remove old if present
  if (mInternalCertStore.Certs[Index].Cert != NULL) {
    FreePool ((VOID *)mInternalCertStore.Certs[Index].Cert);
    mInternalCertStore.Certs[Index].Cert     = NULL;
    mInternalCertStore.Certs[Index].CertSize = 0;

    mInternalCertStore.PopulatedIdentities &= ~(Data->DfciIdentity); // unset the PopulatedIdentities
    // Destroy any auth handle that is using the old Identity
  }

  // Dont try to copy if it was a delete operation
  if (Data->Payload > 0) {
    mInternalCertStore.Certs[Index].Cert     = NewCertData;
    mInternalCertStore.Certs[Index].CertSize = Data->PayloadSize;

    CopyMem ((VOID *)mInternalCertStore.Certs[Index].Cert, Data->Payload, mInternalCertStore.Certs[Index].CertSize);
    mInternalCertStore.PopulatedIdentities |= (Data->DfciIdentity);  // Set the populatedIdentities
  }

  // Data will be saved after all identities have been set
  Data->LKGDirty = TRUE;

  Data->StatusCode = EFI_SUCCESS;
  Data->State      = DFCI_PACKET_STATE_DATA_COMPLETE;
  return EFI_SUCCESS;
}

/**
Delete the variable for NV space
**/
VOID
DeleteProvisionVariable (
  IN DFCI_INTERNAL_PACKET  *Data
  )
{
  if ((Data == NULL) || (Data->State == DFCI_PACKET_STATE_UNINITIALIZED)) {
    return;
  }

  if (Data->State == DFCI_PACKET_STATE_DATA_DELAYED_PROCESSING) {
    // don't delete the variable since we should come and try again later
    return;
  }

  gRT->SetVariable ((CHAR16 *)Data->MailboxName, &gDfciAuthProvisionVarNamespace, 0, 0, NULL);
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
ValidateIdentityPacket (
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

  if (((UINT8 *)Data->VarIdentity <= Data->Packet->Hdr.Pkt) ||
      ((UINT8 *)Data->VarIdentity >= EndData))
  {
    DEBUG ((DEBUG_ERROR, "%a - VarIdentity outside Pkt. %p <= %p <= %p.\n", __FUNCTION__, Data->Packet->Hdr.Pkt, Data->VarIdentity, EndData));
    return EFI_COMPROMISED_DATA;
  }

  if ((Data->Packet->Version >= DFCI_IDENTITY_VAR_VERSION) && (Data->Version == 0)) {
    if (((UINT8 *)Data->Version <= Data->Packet->Hdr.Pkt) ||
        ((UINT8 *)Data->Version >= EndData))
    {
      DEBUG ((DEBUG_ERROR, "%a - Version outside Pkt. %p <= %p <= %p.\n", __FUNCTION__, Data->Packet->Hdr.Pkt, Data->Version, EndData));
      return EFI_COMPROMISED_DATA;
    }
  }

  if ((Data->Packet->Version >= DFCI_IDENTITY_VAR_VERSION) && (Data->LSV == 0)) {
    if (((UINT8 *)Data->LSV <= Data->Packet->Hdr.Pkt) ||
        ((UINT8 *)Data->LSV >= EndData))
    {
      DEBUG ((DEBUG_ERROR, "%a - Lsv outside Pkt. %p <= %p <= %p.\n", __FUNCTION__, Data->Packet->Hdr.Pkt, Data->Version, EndData));
      return EFI_COMPROMISED_DATA;
    }
  }

  if ((Data->PayloadSize != 0) || (Data->Payload != NULL)) {
    if (((UINT8 *)Data->Payload <= Data->Packet->Hdr.Pkt) ||
        ((UINT8 *)Data->Payload+Data->PayloadSize > EndData))
    {
      DEBUG ((DEBUG_ERROR, "%a - Payload outside Pkt. %p <= %p <= %p < %p.\n", __FUNCTION__, Data->Packet->Hdr.Pkt, Data->Payload, Data->Payload+Data->PayloadSize, EndData));
      return EFI_COMPROMISED_DATA;
    }
  }

  return EFI_SUCCESS;
}

/**
 *  Apply an Identity packet
 *
 * @param[in]  This:           Apply Packet Protocol
 * @param[in]  Data:           Pointer to buffer containing packet
 *
 * @retval EFI_SUCCESS -       Packet processed normally
 * @retval Error -             Severe error processing packet
 */
EFI_STATUS
EFIAPI
ApplyNewIdentityPacket (
  IN CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
  IN       DFCI_INTERNAL_PACKET        *Data
  )
{
  EFI_STATUS  Status;

  if ((This != &mApplyIdentityProtocol) || (Data == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a - Bad parameters received.\n", __FUNCTION__));
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (Data->State != DFCI_PACKET_STATE_DATA_PRESENT) {
    DEBUG ((DEBUG_ERROR, "%a - Error detected by caller.\n", __FUNCTION__));
    Status = EFI_ABORTED;
    goto CLEANUP;
  }

  // 1 - Validate the internal packet contents are valid
  Status = ValidateIdentityPacket (Data);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Invalid packet.\n", __FUNCTION__));
    Data->State      = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR; // Code error. this shouldn't happen.
    Data->StatusCode = EFI_ABORTED;
    goto CLEANUP;
  }

  DEBUG ((DEBUG_INFO, "%a - Session ID = 0x%X\n", __FUNCTION__, Data->SessionId));

  //
  // 2 - Validate mailbox data.
  //
  Status = ValidateAndAuthenticatePendingProvisionData (
             Data,
             mDfciSettingsPermissionProtocol
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ValidateAndAuthenticatePendingProvisionData failed %r\n", Status));
    goto CLEANUP;
  }

  //
  // 3 - Check if delayed processing is required.
  //
  // If handling this provisioning request cannot be completed at this time,
  // let the DFciManager know to try again at end of DXE.
  //
  // There are two reasons to wait for the UI to become present:
  //     1 Unenroll of the Owner
  //     2 UserConfirmation is required

  if (!DfciUiIsUiAvailable ()) {
    // If User Confirmation is required
    if (Data->UserConfirmationRequired) {
      //     UserConfirmation is
      Data->State = DFCI_PACKET_STATE_DATA_DELAYED_PROCESSING;
      goto CLEANUP;
    }
  }

  //
  // 4 - Handle User Input
  //
  // If user confirmation is required, get the answer from the user.
  //
  if (Data->UserConfirmationRequired == FALSE) {
    DEBUG ((DEBUG_VERBOSE, "USER APPROVAL NOT NECESSARY\n"));
    Data->State = DFCI_PACKET_STATE_DATA_USER_APPROVED;
  } else {
    Status = LocalGetAnswerFromUser (
               (UINT8 *)Data->Payload,
               Data->PayloadSize,
               &Data->AuthToken
               );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "DfciUiGetAnswerFromUser failed %r\n", Status));
      if (Status == EFI_NOT_READY) {
        Data->State      = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR;
        Data->StatusCode = EFI_NOT_READY;
      } else {
        Data->State      = DFCI_PACKET_STATE_DATA_USER_REJECTED;
        Data->StatusCode = EFI_ABORTED;
      }

      goto CLEANUP;
    } else {
      Data->ResetRequired = TRUE;
      Data->State         = DFCI_PACKET_STATE_DATA_USER_APPROVED;
    }
  }

  if (Data->State != DFCI_PACKET_STATE_DATA_USER_APPROVED) {
    DEBUG ((DEBUG_ERROR, "DfciUiGetAnswerFromUser - User Rejected Change\n"));
    goto CLEANUP;
  }

  //
  // 5 - Apply the change
  //
  Status = ApplyProvisionData (Data);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ApplyProvisionData failed %r\n", Status));
    goto CLEANUP;
  }

  //
  // 6 - Notify Permissions of Identity Change
  //
  Status = mDfciSettingsPermissionProtocol->IdentityChange (
                                              mDfciSettingsPermissionProtocol,
                                              &(Data->AuthToken),
                                              Data->DfciIdentity,
                                              (Data->PayloadSize != 0)
                                              ); // Send TRUE for Enroll
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: IdentityChange notification failed. Status = %r\n", __FUNCTION__, Status));
    Data->StatusCode = Status;
    Data->State      = DFCI_PACKET_STATE_DATA_INVALID;
    goto CLEANUP;
  }

  // Dispose of all mappings for the Identity that changed
  Status = DisposeAllIdentityMappings (Data->DfciIdentity);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[AM] - Failed to dispose of identities for Id 0x%X.  Status = %r\n", Data->DfciIdentity, Status));
    // continue on.
  }

CLEANUP:
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SetProvisionResponse failed %r\n", Status));
  }

  if (Data->AuthToken != DFCI_AUTH_TOKEN_INVALID) {
    DisposeAuthToken (&mAuthProtocol, &(Data->AuthToken));
    Data->AuthToken = DFCI_AUTH_TOKEN_INVALID;
  }

  return Status;
}

/**
 *  Last Known Good handler
 *
 *  Applying identities does NOT change the internal variable, just the internal memory.
 *  After applying Identities, and LKG_COMMIT or LKG_DISCARD must be called
 *
 * @param[in] This:            Apply Packet Protocol
 * @param[in] Data             Internal Packet
 * @param[in] Operation
 *                        DISCARD   discards the in memory changes, and restores from NV STORE
 *                        COMMIT    Saves the current settings to NV Store
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
LKG_Handler (
  IN  CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
  IN        DFCI_INTERNAL_PACKET        *Data,
  IN        UINT8                       Operation
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  DeleteProvisionVariable (Data);

  if ((This != &mApplyIdentityProtocol) || (Data == NULL)) {
    DEBUG ((DEBUG_ERROR, "[AM] - Invalid parameters to LKG Handler.\n"));
    Status = EFI_INVALID_PARAMETER;
  } else {
    switch (Operation) {
      case DFCI_LKG_RESTORE:
        if (Data->LKGDirty) {
          Status = LoadProvisionedData ();
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_ERROR, "[AM] - LKG Unable to load provisioned data. Code=%r.\n", Status));
          } else {
            DEBUG ((DEBUG_INFO, "[AM] - LKG Identities restored.\n"));
          }

          Data->LKGDirty = FALSE;
        }

        break;

      case DFCI_LKG_COMMIT:
        if (Data->LKGDirty) {
          Status = SaveProvisionedData ();
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_ERROR, "[AM] - Unable to save provisioned data. Code=%r.\n", Status));
            if (EFI_ERROR (LoadProvisionedData ())) {
              DEBUG ((DEBUG_ERROR, "[AM] - Unable to restore current provisioned data after save failed.\n"));
            }
          } else {
            DEBUG ((DEBUG_INFO, "[AM] - LKG Identities committed.\n"));
            PopulateCurrentIdentities (TRUE);
          }

          Data->LKGDirty = FALSE;
        }

        break;

      default:
        DEBUG ((DEBUG_ERROR, "[AM] - Invalid operation to LKG Handler(%d) in state (%d).\n", Operation, Data->LKGDirty));
        Status = EFI_INVALID_PARAMETER;
        break;
    }

    if (EFI_ERROR (Status)) {
      Data->StatusCode = Status;
      Data->State      = DFCI_PACKET_STATE_DATA_SYSTEM_ERROR;
    }
  }

  return Status;
}

/**
Clear all DFCI from the System.

This requires an Auth token that has permission to change the owner key and/or permission for recovery.

All settings need a DFCI reset (only reset the settings that are DFCI only)
All Permissions need a DFCI Reset (clear all permissions and internal data)
All Auth needs a DFCI reset (Clear all keys and internal data)

**/
EFI_STATUS
EFIAPI
ClearDFCI (
  IN CONST DFCI_AUTH_TOKEN  *AuthToken
  )
{
  DFCI_SETTING_ACCESS_PROTOCOL  *SettingsAccess = NULL;
  EFI_STATUS                    Status          = EFI_SUCCESS;

  if (*AuthToken == DFCI_AUTH_TOKEN_INVALID) {
    DEBUG ((DEBUG_ERROR, "[AM] - %a - ClearDFCI requires valid auth token\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto CLEANUP;
  }

  //
  // Check to make sure we have necessary protocols
  //
  if (mDfciSettingsPermissionProtocol == NULL) {
    DEBUG ((DEBUG_ERROR, "[AM] - %a - requires Settings Permission Protocol\n", __FUNCTION__));
    Status = EFI_NOT_READY;
    goto CLEANUP;
  }

  //
  // Get SettingsAccess
  //
  Status = gBS->LocateProtocol (&gDfciSettingAccessProtocolGuid, NULL, (VOID **)&SettingsAccess);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[AM] - %a - requires Settings Access Protocol (Status = %r)\n", __FUNCTION__, Status));
    goto CLEANUP;
  }

  // Must Reset Settings (including settings internal data)
  Status = SettingsAccess->Reset (SettingsAccess, AuthToken);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[AM] - %a - FAILED to clear Settings.  Status = %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
    goto CLEANUP;
  }

  DEBUG ((DEBUG_INFO, "[AM] Settings Cleared\n"));

  // Must clear permissions (including internal data)
  Status = mDfciSettingsPermissionProtocol->ResetPermissions (mDfciSettingsPermissionProtocol, AuthToken);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[AM] - %a - FAILED to Reset Permissions. Status = %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
    goto CLEANUP;
  }

  DEBUG ((DEBUG_INFO, "[AM] Permissions Reset\n"));

  // Must delete keys (including internal data)
  Status = InitializeProvisionedData ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[AM] - %a - FAILED to Reset All Auth. Status = %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
    goto CLEANUP;
  }

  DEBUG ((DEBUG_INFO, "[AM] All Stored Authentication Keys Reset\n"));

  // Dispose all Key based Identity Mappings in the system
  Status = DisposeAllIdentityMappings (DFCI_IDENTITY_MASK_KEYS);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[AM] - %a - FAILED to dispose all existing key based auth tokens. Status = %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
    goto CLEANUP;
  }

CLEANUP:
  return Status;
}
