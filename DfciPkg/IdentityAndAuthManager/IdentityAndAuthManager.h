/**@file
IdentityAndAuthManager.h

Identity and Auth Manager defines

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

#ifndef IDENTITY_AND_AUTH_MANAGER_H
#define IDENTITY_AND_AUTH_MANAGER_H

#include <PiDxe.h>
#include <DfciSystemSettingTypes.h>

#include <Guid/ImageAuthentication.h>
#include <Guid/DfciPacketHeader.h>
#include <Guid/DfciIdentityAndAuthManagerVariables.h>
#include <Guid/DfciInternalVariableGuid.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/DfciUiSupportLib.h>
#include <Library/DfciDeviceIdSupportLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/ZeroTouchSettingsLib.h>

#include <Protocol/MuPkcs7.h>
#include <Protocol/DfciSettingPermissions.h>
#include <Protocol/DfciSettingAccess.h>
#include <Protocol/DfciApplyPacket.h>
#include <Protocol/DfciAuthentication.h>


extern DFCI_APPLY_PACKET_PROTOCOL mApplyIdentityProtocol;
//
// Internal structure to be used to hold cert details
// of provisioned data
//
typedef struct {
  UINT8   *Cert;
  UINTN   CertSize;
}INTERNAL_CERT_DETAILS;

#define MAX_NUMBER_OF_CERTS_V1      (4)
#define MAX_NUMBER_OF_CERTS         (7)
#define CERT_STRING_SIZE            (200)
#define SHA1_FINGERPRINT_DIGEST_SIZE           (SHA1_DIGEST_SIZE)
#define SHA1_FINGERPRINT_DIGEST_STRING_SIZE    (SHA1_DIGEST_SIZE * 2)       // 2 characters per byte
#define SHA1_FINGERPRINT_DIGEST_STRING_SIZE_UI ((SHA1_DIGEST_SIZE * 3) - 1) // 3 characters per byte (except last)

//
// Because of how nv is stored it is hard to add a new
// cert index later.  Therefore create two open slots
// for future enhancements.
//
#define CERT_USER_INDEX             (0)
#define CERT_USER1_INDEX            (1)
#define CERT_USER2_INDEX            (2)
#define CERT_OWNER_INDEX            (3)
#define CERT_ZTD_INDEX              (4)
#define CERT_RSVD1_INDEX            (5)
#define CERT_RSVD2_INDEX            (6)
#define CERT_INVALID_INDEX          (0xFF)

#define MAX_SUBJECT_ISSUER_LENGTH 300

#define DFCI_IDENTITY_MASK_LOCAL_PW             (DFCI_IDENTITY_LOCAL)
#define DFCI_IDENTITY_MASK_KEYS                 (DFCI_IDENTITY_SIGNER_USER | DFCI_IDENTITY_SIGNER_USER1 | DFCI_IDENTITY_SIGNER_USER2 | DFCI_IDENTITY_SIGNER_OWNER | DFCI_IDENTITY_SIGNER_ZTD )
#define DFCI_IDENTITY_MASK_USER_KEYS            (DFCI_IDENTITY_SIGNER_USER | DFCI_IDENTITY_SIGNER_USER1 | DFCI_IDENTITY_SIGNER_USER2)

typedef struct {
  UINT32                  Version;
  UINT32                  Lsv;
  DFCI_IDENTITY_MASK      PopulatedIdentities;  //bitmask with all identities
  INTERNAL_CERT_DETAILS   Certs[MAX_NUMBER_OF_CERTS];
} INTERNAL_CERT_STORE;

#define DFCI_AUTH_TO_ID_LIST_ENTRY_SIGNATURE SIGNATURE_32('M','S','A','I')
//
// Internal structure to be used for the link list.
//
typedef struct {
  UINTN                     Signature;
  LIST_ENTRY                Link;
  DFCI_AUTH_TOKEN           AuthToken;
  DFCI_IDENTITY_PROPERTIES  *Identity;
} DFCI_AUTH_TO_ID_LIST_ENTRY;

extern LIST_ENTRY                    mAuthHandlesToIdentity;   //list
extern INTERNAL_CERT_STORE           mInternalCertStore;       //Internal Cert Store
extern DFCI_AUTHENTICATION_PROTOCOL  mAuthProtocol;            //Auth Protocol
extern DFCI_SETTING_PERMISSIONS_PROTOCOL *mDfciSettingsPermissionProtocol; //Permission Protocol

//****//  EXTERNAL FUNCTIONS - Protocol implementation //****//

EFI_STATUS
EFIAPI
GetIdentityProperties(
IN  CONST DFCI_AUTHENTICATION_PROTOCOL     *This,
IN  CONST DFCI_AUTH_TOKEN                  *IdentityToken,
IN OUT DFCI_IDENTITY_PROPERTIES            *Properties
);

EFI_STATUS
EFIAPI
DisposeAuthToken(
IN  CONST DFCI_AUTHENTICATION_PROTOCOL        *This,
IN OUT DFCI_AUTH_TOKEN                        *IdentityToken
);

EFI_STATUS
EFIAPI
AuthWithSignedData(
  IN  CONST DFCI_AUTHENTICATION_PROTOCOL     *This,
  IN  CONST UINT8                            *SignedData,
  IN  UINTN                                   SignedDataLength,
  IN  CONST WIN_CERTIFICATE                  *Signature,
  IN OUT DFCI_AUTH_TOKEN                     *IdentityToken
);

EFI_STATUS
EFIAPI
AuthWithPW(
IN  CONST DFCI_AUTHENTICATION_PROTOCOL     *This,
IN  CONST CHAR16                           *Password OPTIONAL,
IN  UINTN                                   PasswordLength,
OUT DFCI_AUTH_TOKEN                        *IdentityToken
);

EFI_STATUS
EFIAPI
GetRecoveryPacket(
  IN CONST DFCI_AUTHENTICATION_PROTOCOL       *This,
  IN       DFCI_IDENTITY_ID                   Identity,
  OUT      DFCI_AUTH_RECOVERY_PACKET          **Packet
  );

EFI_STATUS
EFIAPI
SetRecoveryResponse(
  IN CONST DFCI_AUTHENTICATION_PROTOCOL       *This,
  IN CONST UINT8                              *RecoveryResponse,
  IN       UINTN                               Size
  );

EFI_STATUS
EFIAPI
GetEnrolledIdentities(
  IN CONST DFCI_AUTHENTICATION_PROTOCOL       *This,
  OUT      DFCI_IDENTITY_MASK                 *EnrolledIdentities
  );

EFI_STATUS
EFIAPI
GetCertInfo(
  IN CONST DFCI_AUTHENTICATION_PROTOCOL       *This,
  IN       DFCI_IDENTITY_ID                    Identity,
  IN CONST UINT8                              *Cert          OPTIONAL,
  IN       UINTN                               CertSize,
  IN       DFCI_CERT_REQUEST                   CertRequest,
  IN       DFCI_CERT_FORMAT                    CertFormat,
  OUT      VOID                              **Value,
  OUT      UINTN                              *ValueSize     OPTIONAL
  );

EFI_STATUS
EFIAPI
ApplyNewIdentityPacket (
    IN CONST DFCI_APPLY_PACKET_PROTOCOL *This,
    IN       DFCI_INTERNAL_PACKET       *ApplyPacket
  );

EFI_STATUS
EFIAPI
SetIdentityResponse(
  IN  CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
  IN        DFCI_INTERNAL_PACKET        *Data
  );

EFI_STATUS
EFIAPI
LKG_Handler(
    IN  CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
    IN        DFCI_INTERNAL_PACKET        *ApplyPacket,
    IN        UINT8                        Operation
  );

//*****// INTERNAL FUNCTIONS //*******//

/**
Add an Auth Token to Id Properties Entry

@param  Token                Add new token to id map
@param  Properties           Identity Properties ptr

**/
EFI_STATUS
AddAuthHandleMapping(
  IN DFCI_AUTH_TOKEN          *Token,
  IN DFCI_IDENTITY_PROPERTIES *Properties
  );

/**
Function used to create an Auth Token
and add it to the map for a given ID.

If error occurs it will return invalid auth token.
**/
DFCI_AUTH_TOKEN
EFIAPI
CreateAuthTokenWithMapping(DFCI_IDENTITY_ID Id);

/**
Function to dispose any existing identity mappings that
are for an Id in the Identity Mask.

Success for all disposed
Error for error condition
**/
EFI_STATUS
EFIAPI
DisposeAllIdentityMappings(DFCI_IDENTITY_MASK Mask);

/*
Check to see what identities are provisioned and if provisioned return a bitmask
that convers the provisioned identities.
*/
DFCI_IDENTITY_MASK
Provisioned();

/**
Function to initialize the provisioned NV data to defaults.

This will delete any existing variable and recreate it using the default values.
*/
EFI_STATUS
EFIAPI
InitializeProvisionedData();

/*
Get the CertData and Size for a given provisioned Cert

@param CertData  Double Ptr.  On success return it will point to
                 static const buffer shared within the module for the cert data
@param CertSize  Will be filled in with the size of the Cert
@param KeyMask   Certificate data being requested

@retval  SUCCESS on found otherwise error
*/
EFI_STATUS
GetProvisionedCertDataAndSize(
  OUT UINT8   **CertData,
  OUT UINTN   *CertSize,
  IN  DFCI_IDENTITY_ID Identity);

/**
Provisioned Data entry point.
This function should load or initialize the variable and the Internal Cert Store on
every boot.  This also verifies the contents are valid in flash.
**/
EFI_STATUS
EFIAPI
PopulateInternalCertStore();

/**
Check for incoming provisioning request for User Cert or Owner Cert
Provision request could be Provision or Change/Delete.

**/
VOID
CheckForNewProvisionInput();

/**
Internal function to map external identites to the cert index used
internally to store the certificate.
If the identity is invalid a invalid index will be returned.
**/
UINT8
DfciIdentityToCertIndex(DFCI_IDENTITY_ID IdentityId);

/**
  Internal function to map cert Index to the DFCI IDENTITY If the
  identity is invalid a invalid index will be returned.
**/
DFCI_IDENTITY_ID
CertIndexToDfciIdentity(UINT8 Identity);

EFI_STATUS
SaveProvisionedData();

VOID
FreeCertStore();

EFI_STATUS
LoadProvisionedData();

VOID
DebugPrintCertStore(
  IN CONST INTERNAL_CERT_STORE* Store);

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
  IN CONST DFCI_AUTH_TOKEN *AuthToken
  );

/**
Function supports verifying the data in the SignedData hasn't
been tampered with since it was signed, creating the signature, by an
key that is verified using the TrustedCert
**/
EFI_STATUS
EFIAPI
VerifySignature(
  IN  CONST UINT8                          *SignedData,
  IN  UINTN                                SignedDataSize,
  IN  CONST WIN_CERTIFICATE                *Signature,
  IN  CONST UINT8                          *TrustedCert,
  IN  UINTN                                TrustedCertSize
  );


/******************************** Functions to support extracting CERT Data *********************/
EFI_STATUS
EFIAPI
GetSubjectName8(
  IN CONST UINT8   *TrustedCert,
  IN       UINTN    CertLength,
  IN       UINTN    MaxStringLength,
  OUT      CHAR8  **Value,
  OUT      UINTN   *ValueSize  OPTIONAL
  );

EFI_STATUS
EFIAPI
GetSubjectName16(
  IN CONST UINT8    *TrustedCert,
  IN       UINTN     CertLength,
  IN       UINTN     MaxStringLength,
  OUT      CHAR16  **Value,
  OUT      UINTN    *ValueSize  OPTIONAL
  );

EFI_STATUS
EFIAPI
GetIssuerName8 (
  IN CONST UINT8   *TrustedCert,
  IN       UINTN    CertLength,
  IN       UINTN    MaxStringLength,
  OUT      CHAR8  **Value,
  OUT      UINTN   *ValueSize  OPTIONAL
  );

EFI_STATUS
EFIAPI
GetIssuerName16 (
  IN CONST UINT8    *TrustedCert,
  IN       UINTN     CertLength,
  IN       UINTN     MaxStringLength,
  OUT      CHAR16  **Value,
  OUT      UINTN    *ValueSize  OPTIONAL
  );

EFI_STATUS
EFIAPI
GetSha1Thumbprint(
  IN CONST UINT8   *TrustedCert,
  IN       UINTN    CertLength,
  OUT      UINT8    (*CertDigest)[SHA1_FINGERPRINT_DIGEST_SIZE]);

EFI_STATUS
EFIAPI
GetSha1Thumbprint8 (
  IN CONST UINT8   *TrustedCert,
  IN       UINTN    CertLength,
  IN       BOOLEAN  UiFormat,
  OUT      CHAR8  **Value,
  OUT      UINTN   *ValueSize  OPTIONAL
);

EFI_STATUS
EFIAPI
GetSha1Thumbprint16(
  IN CONST UINT8   *TrustedCert,
  IN       UINTN    CertLength,
  IN       BOOLEAN  UiFormat,
  OUT      CHAR16 **Value,
  OUT      UINTN   *ValueSize  OPTIONAL
);

/**
 * Populate current identities.  Due to this being new, every boot
 * needs
 *
 * @param Force        TRUE - A change may have occurred. Rebuild current XML
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
PopulateCurrentIdentities(BOOLEAN  Force);

/**
  This routine is called by DFCI to prompt a local user to confirm certificate
  provisioning operations.

  @param  TrustedCert            Supplies a pointer to a trusted certificate.
  @param  TrustedCertSize        Supplies the size in bytes of the trusted
                                 certificate.
  @param  AuthToken              Supplies a pointer that will receive an auth-
                                 entication token.

  @return EFI_NOT_READY          Indicates that UI components are not available.
  @return EFI_ACCESS_DENIED      The user rejected the operation.
  @return EFI_SUCCESS            The user approved the operation.

**/
EFI_STATUS
EFIAPI
LocalGetAnswerFromUser (
  UINT8* TrustedCert,
  UINTN  TrustedCertSize,
  OUT DFCI_AUTH_TOKEN* AuthToken
  );


#endif // IDENTITY_AND_AUTH_MANAGER_H