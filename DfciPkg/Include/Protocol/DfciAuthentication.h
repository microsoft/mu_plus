/** @file
  Defines the Authentication protocol used for authenticating an identity.

  This protocol allows modules to get an Identity token using the authentication methods
  It also allows a module to evaluate the properties of the identity token

Copyright (c) 2015, Microsoft Corporation. All rights reserved.<BR>

**/
#ifndef __DFCI_AUTHENTICATION_PROTOCOL_H__
#define __DFCI_AUTHENTICATION_PROTOCOL_H__

typedef struct _DFCI_AUTHENTICATION_PROTOCOL DFCI_AUTHENTICATION_PROTOCOL;

typedef struct {
  DFCI_IDENTITY_ID Identity;
} DFCI_IDENTITY_PROPERTIES;

typedef UINT8 DFCI_IDENTITY_MASK;  //compatible type as enum DFCI_IDENTITY_ID

typedef struct {
  CHAR16* SubjectString;
  CHAR16* IssuerString;
  CHAR16* ThumbprintString;
} DFCI_CERT_STRINGS;

typedef struct {
  DFCI_IDENTITY_ID Identity;
  UINT64           DataLength;
  UINT8            Data[];
} DFCI_AUTH_RECOVERY_PACKET;

//Required size of the response byte array
#define RECOVERY_RESPONSE_SIZE (10)   


/////////////////////*** AUTH FUNCTIONS ***////////////////////////
typedef
EFI_STATUS
(EFIAPI *DFCI_AUTHENTICATE_WITH_PASSWORD) (
  IN  CONST DFCI_AUTHENTICATION_PROTOCOL  *This,
  IN  CONST CHAR16                        *Password OPTIONAL,
  IN  UINTN                                PasswordLength,
  OUT DFCI_AUTH_TOKEN                     *IdentityToken
  );

typedef
EFI_STATUS
(EFIAPI *DFCI_AUTHENTICATE_SIGNED_DATA) (
  IN  CONST DFCI_AUTHENTICATION_PROTOCOL   *This,
  IN  CONST UINT8                          *SignedData,
  IN  UINTN                                SignedDataLength,
  IN  CONST WIN_CERTIFICATE                *Signature,
  IN OUT DFCI_AUTH_TOKEN                   *IdentityToken
  );

typedef
EFI_STATUS
(EFIAPI *DFCI_AUTHENTICATE_DISPOSE_AUTH_TOKEN) (
  IN  CONST DFCI_AUTHENTICATION_PROTOCOL  *This,
  IN OUT DFCI_AUTH_TOKEN                  *IdentityToken
  );


/////////////////*** VERIFY/QUERY FUNCTIONS ***////////////////

/**
Function to Get the Identity Properties of a given Identity Token.

This function will recieve caller input and needs to protect against brute force
attack by rate limiting or some other means as the IdentityToken values are limited.

@param This           Auth Protocol Instance Pointer
@param IdentityToken  Token to get Properties
@param Properties     caller allocated memory for the current Identity properties to be copied to.

@retval EFI_SUCCESS   If the auth token is valid and Properties updated with current values
@retval ERROR         Failed

**/
typedef
EFI_STATUS
(EFIAPI *DFCI_AUTHENTICATE_GET_IDENTITY_PROPERTIES) (
  IN  CONST DFCI_AUTHENTICATION_PROTOCOL     *This,
  IN  CONST DFCI_AUTH_TOKEN                  *IdentityToken,
  IN OUT DFCI_IDENTITY_PROPERTIES            *Properties
  );


/**
Function to return the currently enrolled identities within the system.

This is a combonation of all identities (not just keys).

@param This               Auth Protocol Instance Pointer
@param EnrolledIdentites  pointer to Mask to be updated


@retval EFI_SUCCESS   EnrolledIdentities will contain a valid MASK for all identities
@retval ERROR         Couldn't get identities

**/
typedef
EFI_STATUS
(EFIAPI *DFCI_AUTHENTICATE_GET_ENROLLED_IDENTITY_MASK) (
  IN CONST DFCI_AUTHENTICATION_PROTOCOL     *This,
  OUT      DFCI_IDENTITY_MASK                 *EnrolledIdentities
  );


/**
This function returns a dynamically populated CertInfo struct members;

The strings can also be NULL.

caller should free the CertInfo members once finished.

@param This               Auth Protocol Instance Pointer
@param Identity           identity to get cert info for
@param CertInfo           Caller allocted CertInfo struct.  
                          Members will be populated with dynamically allocated strings
                          or NULL. 

@retval EFI_SUCCESS   CertInfo is updated
@retval ERROR         Couldn't get certinfo
**/
typedef
EFI_STATUS
(EFIAPI *DFCI_AUTHENTICATE_GET_CERT_INFO) (
  IN CONST DFCI_AUTHENTICATION_PROTOCOL     *This,
  IN       DFCI_IDENTITY_ID                  Identity,
  IN       UINT8                            *Cert,
  IN       UINTN                             CertSize,
  OUT      DFCI_CERT_STRINGS                *CertInfo
  );

/**
This function returns a dynamically allocated Recovery Packet. 
caller should free the Packet once finished.
Identity must be a valid key and have permission to do recovery

@param This               Auth Protocol Instance Pointer
@param Identity           identity to use to create recovery packet
@param Packet             Dynamically allocated Encrypted Recovery Packet

@retval EFI_SUCCESS   Packet is valid
@retval ERROR         no packet created
**/
typedef
EFI_STATUS
(EFIAPI *DFCI_AUTHENTICATE_GET_RECOVERY_PACKET) (
  IN CONST DFCI_AUTHENTICATION_PROTOCOL       *This,
  IN       DFCI_IDENTITY_ID                    Identity,
  OUT      DFCI_AUTH_RECOVERY_PACKET         **Packet
  );

/**
This function validates the user provided Recovery response against 
the active recovery packet for this session.  (1 packet at a given time/boot)

@param This               Auth Protocol Instance Pointer
@param RecoveryResponse   binary bytes of the recovery response
@param Size               Size of the response buffer in bytes.  

@retval EFI_SUCCESS            - Recovery successful.  DFCI is 
        unenrolled
@retval EFI_SECURITY_VIOLATION - All valid attempts have been 
        exceeded.  Device needs rebooted.  Recovery session over
@retval EFI_ACCESS_DENIED      - Incorrect RecoveryResponse.  Try again.
@retval ERROR                  - Other error
**/
typedef
EFI_STATUS
(EFIAPI *DFCI_AUTHENTICATE_SET_RECOVERY_RESPONSE) (
  IN CONST DFCI_AUTHENTICATION_PROTOCOL    *This,
  IN CONST UINT8                           *RecoveryResponse,
  IN       UINTN                            Size
  );
 


//
// DFCI AUTHENTICATION protocol structure
//
#pragma pack (push, 1)
struct _DFCI_AUTHENTICATION_PROTOCOL
{
  DFCI_AUTHENTICATE_GET_ENROLLED_IDENTITY_MASK  GetEnrolledIdentities;
  DFCI_AUTHENTICATE_WITH_PASSWORD               AuthWithPW;
  DFCI_AUTHENTICATE_SIGNED_DATA                 AuthWithSignedData;
  DFCI_AUTHENTICATE_DISPOSE_AUTH_TOKEN          DisposeAuthToken;
  DFCI_AUTHENTICATE_GET_IDENTITY_PROPERTIES     GetIdentityProperties;
  DFCI_AUTHENTICATE_GET_CERT_INFO               GetCertInfo;
  DFCI_AUTHENTICATE_GET_RECOVERY_PACKET         GetRecoveryPacket;
  DFCI_AUTHENTICATE_SET_RECOVERY_RESPONSE       SetRecoveryResponse; 
};
#pragma pack (pop)

extern EFI_GUID gDfciAuthenticationProtocolGuid;
extern EFI_GUID gDfciAuthenticationProvisioningPendingGuid;  //This protocol is only a flag to tell other modules in the system that Update pending

#endif //__DFCI_AUTHENTICATION_PROTOCOL_H__
