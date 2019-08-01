/** @file
DfciIdentityAndAuthManagerVariables.h

Contains definitions for Identity and Authentication Manager variables.

These variables are used to provision or change the Authentication Manager Certificates

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_IDENTITY_AND_AUTH_MANAGER_VARIABLES_H__
#define __DFCI_IDENTITY_AND_AUTH_MANAGER_VARIABLES_H__

#include <Guid/DfciPacketHeader.h>

//
// Variable namespace
//
extern EFI_GUID gDfciAuthProvisionVarNamespace;

#define DFCI_IDENTITY_CURRENT_VAR_NAME  L"DfciIdentityCurrent"
#define DFCI_IDENTITY_APPLY_VAR_NAME    L"DfciIdentityApply"
#define DFCI_IDENTITY_RESULT_VAR_NAME   L"DfciIdentityResult"
#define DFCI_IDENTITY2_APPLY_VAR_NAME   L"DfciIdentity2Apply"
#define DFCI_IDENTITY2_RESULT_VAR_NAME  L"DfciIdentity2Result"
#define DFCI_IDENTITY_VAR_ATTRIBUTES    (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

#define DFCI_IDENTITY_APPLY_VAR_SIGNATURE     SIGNATURE_32('M','S','P','A')
#define DFCI_IDENTITY_RESULT_VAR_SIGNATURE    SIGNATURE_32('M','S','P','R')

#define DFCI_IDENTITY_VAR_VERSION    (2)

#define DFCI_IDENTITY_RESULT_VERSION (1)

#define DFCI_SIGNER_PROVISION_IDENTITY_INVALID (0)
#define DFCI_SIGNER_PROVISION_IDENTITY_OWNER (1)
#define DFCI_SIGNER_PROVISION_IDENTITY_USER (2)
#define DFCI_SIGNER_PROVISION_IDENTITY_USER1 (3)
#define DFCI_SIGNER_PROVISION_IDENTITY_USER2 (4)
#define DFCI_SIGNER_PROVISION_IDENTITY_ZTD (5)

#pragma pack (push, 1)


typedef struct {
  DFCI_PACKET_HEADER Header;    // (22) Signature =  'M', 'S', 'P', 'A'
                                //      Header Version = 2
  UINT8  Service;               //  (1) Service class
  UINT8  Rsvd;                  //  (1) Alignment
  UINT32 Version;               //  (4) Current packet version
  UINT32 LSV;                   //  (4) New Lowest Supported Version value
                                // (32)
  UINT8  SmBiosStrings[];       // Where SmBios strings start
  //
  // The strings and certificate MUST be in this order in order to use the Offsets to determine the max
  // string size.  eg. StrSize(MfgName) == SystemProductOffset-SystemMfgOffset.
  //
  // CHAR8 MfgName              // NULL terminated MfgName
  // CHAR8 ProductName          // NULL terminated ProductName
  // CHAR8 SerialNumber         // NULL terminated SerialNumber
  // WIN_CERT TestSignature;    // The Test Signature  (not required when TrustedCertSize is 0).
                                // The Test signature is of the TrustedCert using the a key that verifies against the trusted cert.
                                // This is used to make sure the TrustedCert that will be provisioned is capabile of verifying signatures.
  // WIN_CERT Signature;        //Signature Auth data - Signature covers all data in this struct except SessionId is 0.
} DFCI_SIGNER_PROVISION_APPLY_VAR;

typedef struct {
  DFCI_PACKET_SIGNATURE Header; // Signature = 'M', 'S', 'P', 'R'
                                // Version = 1
  UINT8  Identity;              // Owner = 1, User = 2, User1 = 3, User2 = 4, ZTD = 5
  UINT32 SessionId;             // Session Id of the apply var that this result is for
  UINT64 StatusCode;            // Status code showing result or error.  0 = Success.  Non-zero = Error
} DFCI_SIGNER_PROVISION_RESULT_VAR;

#pragma pack (pop)


#endif // __DFCI_IDENTITY_AND_AUTH_MANAGER_VARIABLES_H__
