/**@file
AuthManager.c

Implements the Auth Manager Protocol - Verifies all signatures

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

#include "IdentityAndAuthManager.h"
#include <Protocol/Rng.h>       //protocol based rng support
#include <Library/DfciPasswordLib.h>
#include <Library/ZeroTouchSettingsLib.h>

//Statically allocate the supported identities.  
DFCI_IDENTITY_PROPERTIES mIdentityProperties_Local              = { DFCI_IDENTITY_LOCAL        };
DFCI_IDENTITY_PROPERTIES mIdentityProperties_SIGNER_USER1       = { DFCI_IDENTITY_SIGNER_USER1 };
DFCI_IDENTITY_PROPERTIES mIdentityProperties_SIGNER_USER2       = { DFCI_IDENTITY_SIGNER_USER2 };
DFCI_IDENTITY_PROPERTIES mIdentityProperties_SIGNER_USER        = { DFCI_IDENTITY_SIGNER_USER  };
DFCI_IDENTITY_PROPERTIES mIdentityProperties_SIGNER_OWNER       = { DFCI_IDENTITY_SIGNER_OWNER };
DFCI_IDENTITY_PROPERTIES mIdentityProperties_SIGNER_ZTD         = { DFCI_IDENTITY_SIGNER_ZTD    };

//Random Number Protocol
EFI_RNG_PROTOCOL* mRngGenerator = NULL;
#define AUTH_MANAGER_PW_STATE_UNKNOWN   (0)
#define AUTH_MANAGER_PW_STATE_NO_PW     (1)
#define AUTH_MANAGER_PW_STATE_PW        (2)
UINT8 mAdminPasswordSetState = 0;  



DFCI_AUTH_TOKEN
CreateRandomAuthToken()
{
  EFI_STATUS Status = EFI_SUCCESS; 
  UINTN RandomValue = 0;
  UINTN RandomSize = sizeof(UINTN);
  if (mRngGenerator == NULL)
  {
    Status = gBS->LocateProtocol(&gEfiRngProtocolGuid, NULL, (VOID **)&mRngGenerator);
  }

  if (!EFI_ERROR(Status))
  {
    DEBUG((DEBUG_INFO, "%a - Using RNG protocol to random\n", __FUNCTION__));
    Status = mRngGenerator->GetRNG(mRngGenerator, NULL, RandomSize, (UINT8*)&RandomValue);  //Get a random number of size UINTN
  }

  //Hit an error getting random
  if (EFI_ERROR(Status))
  {
    ASSERT_EFI_ERROR(Status);
    return DFCI_AUTH_TOKEN_INVALID;
  }

  return (DFCI_AUTH_TOKEN)RandomValue;
}


/**
Function used to create an Auth Token
and add it to the map for a given ID.

If error occurs it will return invalid auth token.
**/
DFCI_AUTH_TOKEN
EFIAPI
CreateAuthTokenWithMapping(DFCI_IDENTITY_ID Id)
{
  DFCI_AUTH_TOKEN Random = DFCI_AUTH_TOKEN_INVALID;
  DFCI_IDENTITY_PROPERTIES *Props = NULL;
  EFI_STATUS Status;

  //Create Auth Token
  Random = CreateRandomAuthToken();
  if (Random == DFCI_AUTH_TOKEN_INVALID)
  {
    DEBUG((DEBUG_ERROR, "%a - Couldn't create Auth Token.\n", __FUNCTION__));
    return Random;
  }

  //figure out what properties it should be
  switch (Id) {
  case DFCI_IDENTITY_LOCAL:
    Props = &mIdentityProperties_Local;
    break;

  case DFCI_IDENTITY_SIGNER_USER:
    Props = &mIdentityProperties_SIGNER_USER;
    break;

  case DFCI_IDENTITY_SIGNER_USER1:
    Props = &mIdentityProperties_SIGNER_USER1;
    break;

  case DFCI_IDENTITY_SIGNER_USER2:
    Props = &mIdentityProperties_SIGNER_USER2;
    break;

  case DFCI_IDENTITY_SIGNER_OWNER:
    Props = &mIdentityProperties_SIGNER_OWNER;
    break;

  case DFCI_IDENTITY_SIGNER_ZTD :
    Props = &mIdentityProperties_SIGNER_ZTD;
    break;

  default:
    DEBUG((DEBUG_ERROR, "%a: invalid Id\n", __FUNCTION__));
    return DFCI_AUTH_TOKEN_INVALID;
  }

  //add it to the mapping list
  Status = AddAuthHandleMapping(&Random, Props);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Couldn't add auth mapping. %r\n", __FUNCTION__, Status));
    return DFCI_AUTH_TOKEN_INVALID;
  }
  return Random;
}

/**
  Authenticate using a Password

**/
EFI_STATUS
EFIAPI 
AuthWithPW(
  IN  CONST DFCI_AUTHENTICATION_PROTOCOL     *This,
  IN  CONST CHAR16                         *Password OPTIONAL,
  IN  UINTN                                PasswordLength,
  OUT DFCI_AUTH_TOKEN                        *IdentityToken
  )
{
  DFCI_AUTH_TOKEN Random = DFCI_AUTH_TOKEN_INVALID;
  EFI_STATUS Status = EFI_SECURITY_VIOLATION;

  if ((This == NULL) || (IdentityToken == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Check if we know the state of the system password
  //
  if (mAdminPasswordSetState == AUTH_MANAGER_PW_STATE_UNKNOWN)
  {
    if (IsPasswordSet(ADMIN_PW_HANDLE))
    {
      mAdminPasswordSetState = AUTH_MANAGER_PW_STATE_PW;
    }
    else
    {
      mAdminPasswordSetState = AUTH_MANAGER_PW_STATE_NO_PW;
    }
  }

  //
  // Handle the easy case.  No PW set
  //
  if (mAdminPasswordSetState == AUTH_MANAGER_PW_STATE_NO_PW)
  {
    if((Password == NULL) && (PasswordLength < 1))
    { 
      DEBUG((DEBUG_INFO, "[AM] NULL Password Valid\n"));
      Status = EFI_SUCCESS;
      goto AUTH_APPROVED_EXIT;
    }
    //In this case it is invalid as No password is set yet caller tried to use something. 
    DEBUG((DEBUG_ERROR, "[AM] Called with Password when no password set.  Fail.\n"));
    Status = EFI_SECURITY_VIOLATION;
    goto EXIT;
  }

  //
  // System Has PW set
  //
  if ((Password == NULL) || (PasswordLength < 1))
  {
    DEBUG((DEBUG_ERROR, "[AM] NULL Password provided while System PW set\n"));
    Status = EFI_SECURITY_VIOLATION;
    goto EXIT;
  }

  //
  // TODO: add anti-hammering attack mitigation
  //


  //
  // Check the Password
  //
  if (AuthenticatePassword(ADMIN_PW_HANDLE, Password))
  {
    DEBUG((DEBUG_INFO, "[AM] Password Valid\n"));
    Status = EFI_SUCCESS;
    goto AUTH_APPROVED_EXIT;
  }

  //!! Failed Validation

  //
  // TODO: Save state for anti-hammering attack mitigation
  //

  DEBUG((DEBUG_ERROR, "[AM] Incorrect PW\n"));
  Status = EFI_SECURITY_VIOLATION;
  goto EXIT;


AUTH_APPROVED_EXIT:

  Random = CreateAuthTokenWithMapping(DFCI_IDENTITY_LOCAL);
  if (Random == DFCI_AUTH_TOKEN_INVALID)
  {
    DEBUG((DEBUG_ERROR, "%a - Couldn't create Auth Token.\n", __FUNCTION__));
    Status = EFI_DEVICE_ERROR;
    goto EXIT;
  }

EXIT:
  *IdentityToken = Random;  //copy auth token to user passed in buffer
  return Status;
}


/**
 Authenticate using signed data
**/
EFI_STATUS
EFIAPI
AuthWithSignedData(
  IN  CONST DFCI_AUTHENTICATION_PROTOCOL   *This,
  IN  CONST UINT8                          *SignedData,
  IN  UINTN                                SignedDataLength,
  IN  CONST WIN_CERTIFICATE                *Signature,
  IN OUT DFCI_AUTH_TOKEN                   *IdentityToken
  )
{

  EFI_STATUS Status = EFI_SUCCESS;
  DFCI_AUTH_TOKEN Random = DFCI_AUTH_TOKEN_INVALID;
  DFCI_IDENTITY_ID Id = DFCI_IDENTITY_INVALID;
  DFCI_IDENTITY_MASK IdMask = 0;

  //Check input parameters
  if ((This == NULL) || (SignedData == NULL) || (Signature == NULL) || (IdentityToken == NULL))
  {
    Status = EFI_INVALID_PARAMETER;
    goto CLEANUP;
  }

  if (SignedDataLength < 1)
  {
    DEBUG((DEBUG_ERROR, "%a - Signed Data Length is too small. \n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto CLEANUP;
  }

  //Check if we have been provisioned with any certs
  IdMask = Provisioned();
  if ((IdMask & DFCI_IDENTITY_MASK_KEYS) == 0)
  {
    DEBUG((DEBUG_ERROR, "%a - No Keys Provisioned\n", __FUNCTION__));
    Status = EFI_NOT_READY;
    goto CLEANUP;
  }

  //All Signature data will be validated in VerifySignature function

  //
  //If ID still DFCI_IDENTITY_INVALID then check ZTD Key
  //
  if ((Id == DFCI_IDENTITY_INVALID) && (IdMask & DFCI_IDENTITY_SIGNER_ZTD ))
  {
    UINT8* OwnerCertData = NULL;
    UINTN  OwnerCertSize = 0;

    Status = GetProvisionedCertDataAndSize(&OwnerCertData, &OwnerCertSize, DFCI_IDENTITY_SIGNER_ZTD );
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Couldn't Get ZTD Key Data or Size. (%r)\n", __FUNCTION__, Status));
    }
    else
    {
      Status = VerifySignature(SignedData, SignedDataLength, Signature, OwnerCertData, OwnerCertSize);
      if (!EFI_ERROR(Status))
      {
        DEBUG((DEBUG_INFO, "%a Input Data validated with ZTD Cert.\n", __FUNCTION__));
        Id = DFCI_IDENTITY_SIGNER_ZTD ;
      }
    }
  }

  //
  //If ID still DFCI_IDENTITY_INVALID then check Owner Key
  //
  if ((Id == DFCI_IDENTITY_INVALID) && (IdMask & DFCI_IDENTITY_SIGNER_OWNER))
  {
    UINT8* OwnerCertData = NULL;
    UINTN  OwnerCertSize = 0;

    Status = GetProvisionedCertDataAndSize(&OwnerCertData, &OwnerCertSize, DFCI_IDENTITY_SIGNER_OWNER);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Couldn't Get Owner Key Data or Size. (%r)\n", __FUNCTION__, Status));
    }
    else
    {
      Status = VerifySignature(SignedData, SignedDataLength, Signature, OwnerCertData, OwnerCertSize);
      if (!EFI_ERROR(Status))
      {
        DEBUG((DEBUG_INFO, "%a Input Data validated with Owner Cert.\n", __FUNCTION__));
        Id = DFCI_IDENTITY_SIGNER_OWNER;
      }
    }
  }

  //
  //If ID still DFCI_IDENTITY_INVALID then check user key.
  //
  if ((Id == DFCI_IDENTITY_INVALID) && (IdMask & DFCI_IDENTITY_SIGNER_USER))
  {
    UINT8* UserCertData = NULL;
    UINTN  UserCertSize = 0;

    Status = GetProvisionedCertDataAndSize(&UserCertData, &UserCertSize, DFCI_IDENTITY_SIGNER_USER);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Couldn't Get User Key Data or Size. (%r)\n", __FUNCTION__, Status));
    }
    else
    {

      Status = VerifySignature(SignedData, SignedDataLength, Signature, UserCertData, UserCertSize);
      if (!EFI_ERROR(Status))
      {
        Id = DFCI_IDENTITY_SIGNER_USER;
        DEBUG((DEBUG_INFO, "%a Input Data validated with User Cert.\n", __FUNCTION__));
      }
    }
  }

  //
  //If ID still DFCI_IDENTITY_INVALID then check user1 key.
  //
  if ((Id == DFCI_IDENTITY_INVALID) && (IdMask & DFCI_IDENTITY_SIGNER_USER1))
  {
    UINT8* UserCertData = NULL;
    UINTN  UserCertSize = 0;

    Status = GetProvisionedCertDataAndSize(&UserCertData, &UserCertSize, DFCI_IDENTITY_SIGNER_USER1);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Couldn't Get User1 Key Data or Size. (%r)\n", __FUNCTION__, Status));
    }
    else
    {

      Status = VerifySignature(SignedData, SignedDataLength, Signature, UserCertData, UserCertSize);
      if (!EFI_ERROR(Status))
      {
        Id = DFCI_IDENTITY_SIGNER_USER1;
        DEBUG((DEBUG_INFO, "%a Input Data validated with User1 Cert.\n", __FUNCTION__));
      }
    }
  }

  //
  //If ID still DFCI_IDENTITY_INVALID then check user2 key.
  //
  if ((Id == DFCI_IDENTITY_INVALID) && (IdMask & DFCI_IDENTITY_SIGNER_USER2))
  {
    UINT8* UserCertData = NULL;
    UINTN  UserCertSize = 0;

    Status = GetProvisionedCertDataAndSize(&UserCertData, &UserCertSize, DFCI_IDENTITY_SIGNER_USER2);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Couldn't Get User2 Key Data or Size. (%r)\n", __FUNCTION__, Status));
    }
    else
    {

      Status = VerifySignature(SignedData, SignedDataLength, Signature, UserCertData, UserCertSize);
      if (!EFI_ERROR(Status))
      {
        Id = DFCI_IDENTITY_SIGNER_USER2;
        DEBUG((DEBUG_INFO, "%a Input Data validated with User2 Cert.\n", __FUNCTION__));
      }
    }
  }

  if (Id == DFCI_IDENTITY_INVALID)
  {
    DEBUG((DEBUG_ERROR,"[AM] Failed to verify against any provisioned key. %r\n", Status));
    Status = EFI_SECURITY_VIOLATION;
    goto CLEANUP;
  }

  //All good.  Create an Auth token and map it
  Random = CreateAuthTokenWithMapping(Id);
  if (Random == DFCI_AUTH_TOKEN_INVALID)
  {
    DEBUG((DEBUG_ERROR, "%a - Couldn't create Auth Token.\n", __FUNCTION__));
    Status = EFI_DEVICE_ERROR;
    goto CLEANUP;
  }
  
  *IdentityToken = Random;  //copy auth token to user passed in buffer
  Status = EFI_SUCCESS;

CLEANUP:
  return Status;
}



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
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  DFCI_PKCS7_PROTOCOL *Pkcs7Prot = NULL;
  UINTN CertSize = 0;
  BOOLEAN PKCS7 = FALSE;
  BOOLEAN PKCS1 = FALSE;

  //Check input parameters
  if ((SignedData == NULL) || (Signature == NULL) || (TrustedCert == NULL))
  {
    Status = EFI_INVALID_PARAMETER;
    goto CLEANUP;
  }

  if ((SignedDataSize < 1) || (TrustedCertSize < 1))
  {
    DEBUG((DEBUG_ERROR, "[AM] Data Length is too small. \n"));
    Status = EFI_INVALID_PARAMETER;
    goto CLEANUP;
  }

  //////Validate the Signature header data

  //
  // Check embedded size value
  //
  if (Signature->dwLength <= sizeof(WIN_CERTIFICATE))
  {
    DEBUG((DEBUG_ERROR, "[AM] Signature dwLength is not large enough for valid WIN_CERT\n"));
    Status = EFI_INVALID_PARAMETER;
    goto CLEANUP;
  }

  if (Signature->wRevision != 0x200)
  {
    DEBUG((DEBUG_ERROR, "[AM] Signature wRevision incorrect.  0x%x\n", Signature->wRevision));
    Status = EFI_UNSUPPORTED;
    goto CLEANUP;
  }

  //
  // Check Win_Cert type
  //
  // FIRST CHECK FOR PCKS7 
  if (Signature->wCertificateType == WIN_CERT_TYPE_EFI_GUID)
  {
    WIN_CERTIFICATE_UEFI_GUID *Cert = (WIN_CERTIFICATE_UEFI_GUID*)Signature;
    DEBUG((DEBUG_INFO, "[AM] WIN_CERT is of TYPE EFI_GUID\n"));
    if (Signature->dwLength <= OFFSET_OF(WIN_CERTIFICATE_UEFI_GUID, CertData))
    {
      DEBUG((DEBUG_ERROR, "[AM] Signature dwLength is not large enough for valid WIN_CERT_EFI_GUID\n"));
      Status = EFI_INVALID_PARAMETER;
      goto CLEANUP;
    }
    //
    // Now check the Guid for a supported type (PKCS7)
    //
    if (!CompareGuid(&Cert->CertType, &gEfiCertPkcs7Guid))
    {
      DEBUG((DEBUG_ERROR, "[AM] Incorrect Guid\n"));
      Status = EFI_UNSUPPORTED;
      goto CLEANUP;
    }

    // Check to make sure we have some auth data
    if (Signature->dwLength <= OFFSET_OF(WIN_CERTIFICATE_UEFI_GUID, CertData))
    {
      DEBUG((DEBUG_ERROR, "[AM] No Auth data in WIN_CERT struct. \n"));
      Status = EFI_INVALID_PARAMETER;
      goto CLEANUP;
    }
    CertSize = Signature->dwLength - OFFSET_OF(WIN_CERTIFICATE_UEFI_GUID, CertData);

    //Get our protocol for PKCS7
    Status = gBS->LocateProtocol(&gDfciPKCS7ProtocolGuid, NULL, (VOID **)&Pkcs7Prot);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "[AM] Failed to locate PKCS7 Support Protocol. Status = %r\n", Status));
      Status = EFI_UNSUPPORTED;
      goto CLEANUP;
    }

    PKCS7 = TRUE;
  }
  // NOW CHECK FOR PKSC1
  else if (Signature->wCertificateType == WIN_CERT_TYPE_EFI_PKCS115)
  {
    DEBUG((DEBUG_INFO, "[AM] WIN_CERT is of TYPE PKCS115\n"));

    if (Signature->dwLength <= sizeof(WIN_CERTIFICATE_EFI_PKCS1_15))
    {
      DEBUG((DEBUG_ERROR, "[AM] Signature dwLength is not large enough for valid WIN_CERTIFICATE_EFI_PKCS1_15 with data\n"));
      Status = EFI_INVALID_PARAMETER;
      goto CLEANUP;
    }

    CertSize = Signature->dwLength - sizeof(WIN_CERTIFICATE_EFI_PKCS1_15);
    PKCS1 = TRUE;
  }
  else
  {
    DEBUG((DEBUG_ERROR, "[AM] Incorrect Cert Type. 0x%X\n", Signature->wCertificateType));
    Status = EFI_UNSUPPORTED;
    goto CLEANUP;
  }

  DEBUG((DEBUG_INFO, "[AM] %a - CertSize is 0x%X\n", __FUNCTION__, CertSize));


  // Check size...need to know what a good size is?  
  if ((CertSize < 1) || (CertSize > Signature->dwLength))
  {
    DEBUG((DEBUG_ERROR, "[AM] Signature Cert Data Size invalid.\n"));
    Status = EFI_INVALID_PARAMETER;
    goto CLEANUP;
  }

  //
  // Check against Trusted Cert Store
  //

  DEBUG((DEBUG_INFO, "\n====\n[AM] %a - Printing Out The Trusted Cert\n", __FUNCTION__));
  DEBUG_BUFFER(DEBUG_INFO, TrustedCert, TrustedCertSize, (DEBUG_DM_PRINT_OFFSET | DEBUG_DM_PRINT_ASCII));

  DEBUG((DEBUG_INFO, "\n====\n[AM] %a - Printing Out The %a\n", __FUNCTION__, "Incoming Sig Data Struct"));
  DEBUG_BUFFER(DEBUG_INFO, (UINT8 *)(Signature), Signature->dwLength, (DEBUG_DM_PRINT_OFFSET | DEBUG_DM_PRINT_ASCII));

  DEBUG((DEBUG_INFO, "\n====\n[AM] %a - Printing Out The %a\n", __FUNCTION__, "Incoming Signed Data"));
  DEBUG_BUFFER(DEBUG_INFO, (UINT8 *)(SignedData), SignedDataSize, (DEBUG_DM_PRINT_OFFSET | DEBUG_DM_PRINT_ASCII));


  if (PKCS7)
  {
    WIN_CERTIFICATE_UEFI_GUID *Cert = (WIN_CERTIFICATE_UEFI_GUID*)Signature;
    Status = Pkcs7Prot->Verify(Pkcs7Prot, (UINT8 *)(&Cert->CertData), CertSize, TrustedCert, TrustedCertSize, SignedData, SignedDataSize);
  }
  else if (PKCS1)
  {
    Status = VerifyUsingPkcs1((WIN_CERTIFICATE_EFI_PKCS1_15 *)Signature, TrustedCert, TrustedCertSize, SignedData, SignedDataSize);
  }
  else
  {
    Status = EFI_UNSUPPORTED;
  }

CLEANUP:

  DEBUG((DEBUG_INFO, "[AM] - %a - Validation Status %r\n", __FUNCTION__, Status));
  return Status;
}


