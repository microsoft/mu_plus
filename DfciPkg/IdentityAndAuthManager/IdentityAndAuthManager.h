

#ifndef IDENTITY_AND_AUTH_MANAGER_H
#define IDENTITY_AND_AUTH_MANAGER_H

#include <PiDxe.h>
#include <DfciSystemSettingTypes.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/DfciUiSupportLib.h>
#include <Library/DfciDeviceIdSupportLib.h>
#include <Library/BaseCryptLib.h>
#include <Protocol/DfciPkcs7.h>
#include <Protocol/DfciSettingPermissions.h>
#include <Protocol/DfciSettingAccess.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/DfciIdentityAndAuthManagerVariables.h>
#include <Guid/DfciInternalVariableGuid.h>
#include <Protocol/DfciAuthentication.h>


//
// Internal structure to be used to hold cert details
// of provisioned data
//
typedef struct {
  UINT8   *Cert;
  UINTN   CertSize;
}INTERNAL_CERT_DETAILS;

#define MAX_NUMBER_OF_CERTS         (4)
//
// Because of how nv is stored it is hard to add a new
// cert index later.  Therefore create two open slots
// for future enhancements.
//
#define CERT_USER_INDEX             (0)
#define CERT_USER1_INDEX            (1)
#define CERT_USER2_INDEX            (2)
#define CERT_OWNER_INDEX            (3)
#define CERT_INVALID_INDEX          (0xFF)


#define DFCI_IDENTITY_MASK_LOCAL_PW             (DFCI_IDENTITY_LOCAL)
#define DFCI_IDENTITY_MASK_KEYS                 (DFCI_IDENTITY_SIGNER_USER |DFCI_IDENTITY_SIGNER_USER1 | DFCI_IDENTITY_SIGNER_USER2| DFCI_IDENTITY_SIGNER_OWNER)


typedef struct {
  DFCI_IDENTITY_MASK        PopulatedIdentities;  //bitmask with all identities
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

typedef enum {
    AUTH_MAN_PROV_STATE_UNINITIALIZED = 0x00,
    AUTH_MAN_PROV_STATE_DATA_PRESENT = 0x01,
    AUTH_MAN_PROV_STATE_DATA_AUTHENTICATED = 0x02,
    AUTH_MAN_PROV_STATE_DATA_USER_APPROVED = 0x03,
    AUTH_MAN_PROV_STATE_DATA_COMPLETE = 0x0F,   //Complete
    AUTH_MAN_PROV_STATE_DATA_NO_OWNER = 0xF9,  //Can't provision User Auth before having valid Owner Auth
    AUTH_MAN_PROV_STATE_DATA_NOT_CORRECT_TARGET = 0xFA,  //SN target doesn't match device
    AUTH_MAN_PROV_STATE_DATA_DELAYED_PROCESSING = 0xFB,  //needs delayed processing for ui or other reasons
    AUTH_MAN_PROV_STATE_DATA_USER_REJECTED = 0xFC,
    AUTH_MAN_PROV_STATE_DATA_INVALID = 0xFD,   //Need to delete var because of error condition
    AUTH_MAN_PROV_STATE_DATA_AUTH_FAILED = 0xFE,
    AUTH_MAN_PROV_STATE_DATA_AUTH_SYSTEM_ERROR = 0xFF
}AUTH_MAN_PROV_STATE;

//Internal object def to handle incoming request
typedef struct {
    DFCI_SIGNER_PROVISION_APPLY_VAR *Var;
    UINTN                            VarSize;
    UINT32                           SessionId;
    AUTH_MAN_PROV_STATE              State;
    EFI_STATUS                       StatusCode;
    UINT8                            VarIdentity;
    DFCI_IDENTITY_ID                 Identity;
    BOOLEAN                          UserConfirmationRequired;
    BOOLEAN                          RebootRequired;
    DFCI_AUTH_TOKEN                  AuthToken;
    DFCI_DEVICE_ID_ELEMENTS         *DeviceId;
    UINT16                           TrustedCertOffset;
    UINT16                           TrustedCertSize;
    UINT8                           *TrustedCert;
} AUTH_MAN_PROV_INSTANCE_DATA;

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
  IN  CONST UINT8                          *SignedData,
  IN  UINTN                                SignedDataLength,
  IN  CONST WIN_CERTIFICATE                *Signature,
  IN OUT DFCI_AUTH_TOKEN                     *IdentityToken
);

EFI_STATUS
EFIAPI
AuthWithPW(
IN  CONST DFCI_AUTHENTICATION_PROTOCOL     *This,
IN  CONST CHAR16                         *Password OPTIONAL,
IN  UINTN                                PasswordLength,
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
  IN CONST UINT8                            *RecoveryResponse,
  IN       UINTN                            Size
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
  IN       DFCI_IDENTITY_ID                   Identity,
  IN       UINT8*                           Cert,
  IN       UINTN                            CertSize,
  OUT      DFCI_CERT_STRINGS                  *CertInfo
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
Function to verify PKCS1 signed data against a provisioned cert
**/
EFI_STATUS
EFIAPI
VerifyUsingPkcs1
(
  IN CONST WIN_CERTIFICATE_EFI_PKCS1_15   *WinCert,
  IN CONST UINT8                          *TrustedCertData,
  IN       UINTN                          TrustedCertDataSize,
  IN CONST UINT8                          *SignedData,
  IN       UINTN                          SignedDataLength
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
CHAR16*
EFIAPI
GetSubjectName(
  IN CONST UINT8    *TrustedCert,
  UINTN    CertLength,
  UINTN    MaxStringLength
  );

CHAR16*
EFIAPI
GetIssuerName(
  IN CONST UINT8    *TrustedCert,
  UINTN    CertLength,
  UINTN    MaxStringLength
  );

CHAR16*
EFIAPI
GetSha1Thumbprint(
  IN CONST UINT8    *TrustedCert,
  UINTN    CertLength);

#endif // IDENTITY_AND_AUTH_MANAGER_H