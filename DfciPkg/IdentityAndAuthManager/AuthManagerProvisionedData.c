/**@file
AuthManagerProvisionData.c

Implements support for the Internal NV storage of Auth Manager data.

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

//PRIVATE VARIABLE INFO FOR STORAGE OF PROVISIONED VARS
//Use gDfciAuthProvisionVarNamespace for the namespace
//Internal var names
#define _INTERNAL_PROVISIONED_CERT_VAR_NAME          L"_IPCVN"

//Internal var attributes
#define _INTERNAL_VAR_VERSION_V1              (1)
#define _INTERNAL_VAR_VERSION_V2              (2)
#define _INTERNAL_VAR_SIGNATURE               SIGNATURE_32('I','P','C','V')

#pragma warning(push)
#pragma warning(disable: 4200) // zero-sized array
#pragma pack (push, 1)


typedef struct {
  UINTN                 HeaderSignature;
  UINT8                 HeaderVersion;
  UINT8                 MaxCerts;
  UINT16                CertSizes[MAX_NUMBER_OF_CERTS_V1];
  UINT8                 PackedCertData[];
} INTERNAL_VAR_STRUCT_V1;

// NOTE - The code assumes that the Header, MaxCerts, and CertSizes is common
//        in both versions of the internal structure
typedef struct {
  UINTN                 HeaderSignature;
  UINT8                 HeaderVersion;
  UINT8                 MaxCerts;
  UINT16                CertSizes[MAX_NUMBER_OF_CERTS];
  UINT32                Version;
  UINT32                Lsv;
  UINT8                 PackedCertData[];
} INTERNAL_VAR_STRUCT;

#pragma pack (pop)
#pragma warning(pop)


INTERNAL_CERT_STORE mInternalCertStore = { 0, 0, DFCI_IDENTITY_LOCAL, {{NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}} };

/**
Free any dynamically allocated memory from the cert store
and update cert ptr to NULL and size to 0. Retain ZTD if
installed.
**/
VOID
FreeCertStore()
{
  for (INT8 i = 0; i < MAX_NUMBER_OF_CERTS; i++)
  {
    if (i != CERT_ZTD_INDEX)
    {
      if (mInternalCertStore.Certs[i].Cert != NULL)
      {
        FreePool((VOID *) mInternalCertStore.Certs[i].Cert);
      }
      mInternalCertStore.Certs[i].CertSize = 0;
      mInternalCertStore.Certs[i].Cert = NULL;
    }
  }
}

/**
Function to initialize the provisioned NV data to defaults.

This will delete any existing variable and recreate it using the default values.
*/
EFI_STATUS
EFIAPI
InitializeProvisionedData()
{
  EFI_STATUS Status;

  //Delete Internal NV Var to clear everything including var attributes
  Status = gRT->SetVariable(_INTERNAL_PROVISIONED_CERT_VAR_NAME, &gDfciInternalVariableGuid, 0, 0, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_INFO, "%a - Failed to Delete internal provisioned var %r\n", __FUNCTION__, Status));
    //If error that is OK as we will re-initialize anyway
  }

  FreeCertStore();  //free any allocated memory
  mInternalCertStore.PopulatedIdentities = DFCI_IDENTITY_LOCAL;
  if (mInternalCertStore.Certs[CERT_ZTD_INDEX].CertSize != 0)
  {
    mInternalCertStore.PopulatedIdentities |= DFCI_IDENTITY_SIGNER_ZTD ;
  }

  return SaveProvisionedData();
}

/**
Transition old NV storage var to new format.  If successful
the variable will be updated in NV storage so it can be loaded.
If failed an error code will be returned and NV storage will not be
changed.

@return Status of the transition
**/
EFI_STATUS
TransitionOldInternalVar()
{
  //DONT CURRENTLY SUPPORT ANY VERSION UPGRADE
  return EFI_UNSUPPORTED;
}


/**
Load the currently provisioned data
from NV Storage to Internal Cert Store.

@retval  EFI_SUCCESS    - Data was valid and loaded into InternalStore from variable.
@retval  EFI_NOT_FOUND  - Variable didn't exist.
@retval  EFI_COMPROMISED_DATA  - Something inside the variable or attributes were not correct.  Can't trust the data.
@retval  EFI_INCOMPATIBLE_VERSION - Version in variable not current.
@retval  EFI_UNSUPPORTED  - Max Cert changed or not valid.
@retval  EFI_OUT_OF_RESOURCES - Couldn't allocate memory for variable

**/
EFI_STATUS
LoadProvisionedData()
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT32 Attributes = 0;
  UINT8   *BytePtr = NULL;
  UINTN    VarSize = 0;
  INTERNAL_VAR_STRUCT *Var = NULL;
  UINTN MaxCertsAllowed;
  DFCI_IDENTITY_ID Identity;

  //Get the variable.  This function will allocate memory
  //so it must be freed if Status is not error.
  Status = GetVariable3(_INTERNAL_PROVISIONED_CERT_VAR_NAME,
    &gDfciInternalVariableGuid,
    (VOID **) &Var,
    &VarSize,
    &Attributes
    );

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_INFO, "%a - Auth Manager Internal Var could not be loaded. %r\n", __FUNCTION__, Status));
    return Status;
  }

  //Check attributes - if incorrect return corrupt
  if (Attributes != DFCI_INTERNAL_VAR_ATTRIBUTES)
  {
    DEBUG((DEBUG_ERROR, "Auth Manager Internal Var attributes not valid.\n"));
    Status = EFI_COMPROMISED_DATA;
    goto CLEANUP;
  }

  //Check ascii signature to make sure var looks as expected.
  if (Var->HeaderSignature != _INTERNAL_VAR_SIGNATURE)
  {
    DEBUG((DEBUG_ERROR, "Auth Manager Internal Var Signature not valid.\n"));
    Status = EFI_COMPROMISED_DATA;
    goto CLEANUP;
  }

  // NOTE - The code assumes that the HeaderSignature, Version, MaxCerts, and CertSizes is common
  //        in both versions of the internal structure

  //Check Version
  switch (Var->HeaderVersion)
  {
  case _INTERNAL_VAR_VERSION_V1:
      {
          INTERNAL_VAR_STRUCT_V1 *Var1 = NULL;

          Var1 = (INTERNAL_VAR_STRUCT_V1 *) Var;
          VarSize -= sizeof(INTERNAL_VAR_STRUCT_V1);  //Track remaining var size to be processed.
          BytePtr = Var1->PackedCertData;
          mInternalCertStore.Version = 0;
          mInternalCertStore.Lsv = 0;
          MaxCertsAllowed = MAX_NUMBER_OF_CERTS_V1;

      }
      break;
  case _INTERNAL_VAR_VERSION_V2:
      {
          VarSize -= sizeof(INTERNAL_VAR_STRUCT);  //Track remaining var size to be processed.
          BytePtr = Var->PackedCertData;
          mInternalCertStore.Version = Var->Version;
          mInternalCertStore.Lsv = Var->Lsv;
          MaxCertsAllowed = MAX_NUMBER_OF_CERTS;
      }
      break;
  default:
      MaxCertsAllowed = 0;
      DEBUG((DEBUG_INFO, "Auth Manager Internal Var Version not recognized (%d).\n"));
      Status = EFI_INCOMPATIBLE_VERSION;
      goto CLEANUP;
      break;
  }

  //
  // Check the max certs.
  // Code can't handle max cert change.
  //
  if (Var->MaxCerts != MaxCertsAllowed)
  {
    DEBUG((DEBUG_ERROR, "Auth Manager Internal var max certs not correct. Cur=%d,Max=%d\n",Var->MaxCerts, MaxCertsAllowed));
    ASSERT(Var->MaxCerts == MAX_NUMBER_OF_CERTS);
    Status = EFI_UNSUPPORTED;
    goto CLEANUP;
  }

  Status = EFI_SUCCESS;

  //We now have good data from varible store.  decompose and populate internal cert store
  for (UINT8 i = 0; i < MaxCertsAllowed; i++)
  {
    mInternalCertStore.Certs[i].CertSize = (UINTN)(Var->CertSizes[i]);
    if (mInternalCertStore.Certs[i].Cert != NULL)
    {
      FreePool ((VOID *)mInternalCertStore.Certs[i].Cert);
      mInternalCertStore.Certs[i].Cert = NULL;
    }
    if (Var->CertSizes[i] == 0)
    {
      continue;
    }

    //Make sure Var internal size data is not corrupt
    if (VarSize < Var->CertSizes[i])
    {
      DEBUG((DEBUG_ERROR, "%a Remaining VarSize less than CertSize\n", __FUNCTION__));
      Status = EFI_COMPROMISED_DATA;
      goto CLEANUP;
    }

    //Copy var cert data to our cert store
    mInternalCertStore.Certs[i].Cert = AllocatePool(Var->CertSizes[i]);
    if (mInternalCertStore.Certs[i].Cert == NULL)
    {
      DEBUG((DEBUG_ERROR, "Auth Manager Failed to Allocate Memory for Cert\n"));
      Status = EFI_OUT_OF_RESOURCES;
      goto CLEANUP;
    }

    CopyMem((VOID *)mInternalCertStore.Certs[i].Cert, BytePtr, mInternalCertStore.Certs[i].CertSize);
    BytePtr += mInternalCertStore.Certs[i].CertSize;
    VarSize -= mInternalCertStore.Certs[i].CertSize;
    Status = EFI_SUCCESS;
    Identity = CertIndexToDfciIdentity(i);
    if (Identity != DFCI_IDENTITY_INVALID)
    {
      mInternalCertStore.PopulatedIdentities |= Identity;  //update Populated Identies value
    }
  } //Finish for loop for each cert

  if (VarSize != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - VarSize not 0 at end of loop (%d)\n", __FUNCTION__, VarSize));
    ASSERT(VarSize == 0);
  }

  //now check that it follows the rules.
  // 1. Can't have user keys if no Owner Key
  //
  if (((mInternalCertStore.PopulatedIdentities & DFCI_IDENTITY_MASK_USER_KEYS) > 0) &&
    ((mInternalCertStore.PopulatedIdentities & DFCI_IDENTITY_SIGNER_OWNER) == 0))
  {
    DEBUG((DEBUG_ERROR, "[AM] - %a - No Owner Key.  Must clear User keys and all data\n", __FUNCTION__));
    Status = EFI_PROTOCOL_ERROR;
    for (UINT8 i = 0; i < MAX_NUMBER_OF_CERTS; i++)
    {
      Identity = CertIndexToDfciIdentity(i);
      if ((Identity != DFCI_IDENTITY_INVALID) && (Identity & DFCI_IDENTITY_MASK_USER_KEYS))
      {
        if (mInternalCertStore.Certs[i].Cert != NULL)
        {
          FreePool ((VOID *) mInternalCertStore.Certs[i].Cert);
          mInternalCertStore.Certs[i].Cert = NULL;
          mInternalCertStore.Version = 0;
          mInternalCertStore.Lsv = 0;
        }
      }
    }
  }

CLEANUP:
  if (Var != NULL)
  {
    FreePool(Var);
  }
  return Status;
}

/**
Save the Internal cert store to NV storage
**/
EFI_STATUS
SaveProvisionedData()
{
  INTERNAL_VAR_STRUCT* Var = NULL;
  UINT8  *BytePtr = NULL;
  EFI_STATUS Status;
  UINTN VarSize = sizeof(INTERNAL_VAR_STRUCT);  //will need to add dynamic size
  for (INT8 i = 0; i < MAX_NUMBER_OF_CERTS; i++)
  {
    if (mInternalCertStore.Certs[i].Cert != NULL)
    {
      VarSize += mInternalCertStore.Certs[i].CertSize;
    }
  }

  //now allocate memory for var
  Var = (INTERNAL_VAR_STRUCT*) AllocatePool(VarSize);
  if (Var == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a failed to allocate memory for var.\n", __FUNCTION__));
    ASSERT(Var != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  //populate standard data
  Var->HeaderSignature = _INTERNAL_VAR_SIGNATURE;
  Var->HeaderVersion = _INTERNAL_VAR_VERSION_V2;
  Var->MaxCerts = MAX_NUMBER_OF_CERTS;
  Var->Version = mInternalCertStore.Version;
  Var->Lsv = mInternalCertStore.Lsv;
  //Populate cert size array and packed data
  BytePtr = Var->PackedCertData;  //set to first byte of data

  for (INT8 i = 0; i < MAX_NUMBER_OF_CERTS; i++)
  {
    Var->CertSizes[i] = (UINT16)mInternalCertStore.Certs[i].CertSize;
    if ((mInternalCertStore.Certs[i].Cert != NULL) && (mInternalCertStore.Certs[i].CertSize > 0))
    {
      CopyMem(BytePtr, mInternalCertStore.Certs[i].Cert, mInternalCertStore.Certs[i].CertSize);
      BytePtr += mInternalCertStore.Certs[i].CertSize;
    }
  }

  //now var is populated.  Now write it using var store
  Status = gRT->SetVariable(_INTERNAL_PROVISIONED_CERT_VAR_NAME, &gDfciInternalVariableGuid, DFCI_INTERNAL_VAR_ATTRIBUTES, VarSize, Var);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to set variable %r\n", __FUNCTION__, Status));
  }

  FreePool(Var);
  return Status;
}


/*
Check to see what identities are provisioned and if provisioned return a bit-mask
that conveys the provisioned identities.
*/
DFCI_IDENTITY_MASK
Provisioned()
{
  return mInternalCertStore.PopulatedIdentities;
}

/*
Get the CertData and Size for a given provisioned Cert

@param CertData  Double Ptr.  On success return, it will point to a
                 const buffer shared within the module for the cert data
@param CertSize  Will be filled in with the size of the Cert
@param Key       Certificate data being requested

@retval  SUCCESS on found otherwise error
*/
EFI_STATUS
GetProvisionedCertDataAndSize(
  OUT UINT8 CONST **CertData,
  OUT UINTN        *CertSize,
  IN  DFCI_IDENTITY_ID Key)
{
  EFI_STATUS Status= EFI_SUCCESS;
  UINT8 Index = 0;

  if ((CertData == NULL) || (CertSize == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((Key & DFCI_IDENTITY_MASK_KEYS) == 0)
  {
    DEBUG((DEBUG_ERROR, "%a - Key invalid (0x%X).  Not a key.", __FUNCTION__, Key));
    return EFI_INVALID_PARAMETER;
  }

  //Check the input parameter to make sure its provisioned
  if ( (Key & Provisioned()) == 0)
  {
    DEBUG((DEBUG_ERROR, "%a - Key(0x%x) not provisioned\n", __FUNCTION__, Key));
    return EFI_NOT_FOUND;
  }

  //Convert KeyMask to Index
  Index = DfciIdentityToCertIndex(Key);
  if (Index == CERT_INVALID_INDEX)
  {
    DEBUG((DEBUG_ERROR, "%a - Key(0x%x) doesn't map to a cert\n", __FUNCTION__, Key));
    return EFI_INVALID_PARAMETER;
  }

  //Update the data
  *CertData = mInternalCertStore.Certs[Index].Cert;
  *CertSize = mInternalCertStore.Certs[Index].CertSize;
  return Status;
}


/**
Provisioned Data entry point.
This function should load or initialize the variable and the Internal Cert Store on
every boot.  This also verifies the contents are valid in flash.


**/
EFI_STATUS
EFIAPI
PopulateInternalCertStore()

{
  EFI_STATUS Status;
  Status = LoadProvisionedData();
  if (EFI_ERROR(Status))
  {
    FreeCertStore();  //free any garbage from load failure.
    switch (Status) {
    case EFI_NOT_FOUND:
      DEBUG((DEBUG_ERROR, "Failed to load provisioned data because it wasn't found. Probably first boot after flash\n"));
      Status = InitializeProvisionedData();
      break;

    case EFI_INCOMPATIBLE_VERSION:
      DEBUG((DEBUG_ERROR, "Provisioned data in different version.  Trying to transition\n"));
      Status = TransitionOldInternalVar();
      break;

    case EFI_OUT_OF_RESOURCES:
      DEBUG((DEBUG_ERROR, "%a - Out of resources\n", __FUNCTION__));
      ASSERT_EFI_ERROR(Status);
      //try again if release build
      Status = EFI_SUCCESS;
      break;

    case EFI_UNSUPPORTED:
    case EFI_COMPROMISED_DATA:
      DEBUG((DEBUG_ERROR, "Data Corrupted or not valid.  Re-initialize. %r\n", Status));
      //UEFI Blue screen - could be unowned system which might not be good.
      Status = InitializeProvisionedData();
      break;

    case EFI_PROTOCOL_ERROR:
      DEBUG((DEBUG_ERROR, "Data Loaded but data didn't follow the rules. Clearing.... %r\n", Status));
      Status = InitializeProvisionedData();
      break;

    default:
      DEBUG((DEBUG_ERROR, "%a - Error.  Unexpected Status Code. %r\n", Status));
      ASSERT_EFI_ERROR(Status);
      break;
    } //close switch

    Status = LoadProvisionedData();
  }

  return Status;
}


/**
Internal function to map external identities to the cert index used
internally to store the certificate.
If the identity is invalid a invalid index will be returned.
**/
UINT8
DfciIdentityToCertIndex(DFCI_IDENTITY_ID IdentityId)
{
  if (IdentityId == DFCI_IDENTITY_SIGNER_ZTD )
  {
    return CERT_ZTD_INDEX;
  }
  else if (IdentityId == DFCI_IDENTITY_SIGNER_USER)
  {
    return CERT_USER_INDEX;
  }
  else if (IdentityId == DFCI_IDENTITY_SIGNER_USER1)
  {
    return CERT_USER1_INDEX;
  }
  else if (IdentityId == DFCI_IDENTITY_SIGNER_USER2)
  {
    return CERT_USER2_INDEX;
  }
  else if (IdentityId == DFCI_IDENTITY_SIGNER_OWNER)
  {
    return CERT_OWNER_INDEX;
  }
  else
  {
    DEBUG((DEBUG_ERROR, "Invalid Cert Identity 0x%X\n", IdentityId));
    return CERT_INVALID_INDEX;
  }
}

/**
 Internal function to map cert Index to the DFCI IDENTITY If the
 identity is invalid a invalid index will be returned.
**/
DFCI_IDENTITY_ID
CertIndexToDfciIdentity(UINT8 Identity)
{
  if (Identity == CERT_USER_INDEX)
  {
    return DFCI_IDENTITY_SIGNER_USER;
  }
  else if (Identity == CERT_USER1_INDEX)
  {
    return DFCI_IDENTITY_SIGNER_USER1;
  }
  else if (Identity == CERT_USER2_INDEX)
  {
    return DFCI_IDENTITY_SIGNER_USER2;
  }
  else if (Identity == CERT_OWNER_INDEX)
  {
    return DFCI_IDENTITY_SIGNER_OWNER;
  }
  else if (Identity == CERT_ZTD_INDEX)
  {
    return DFCI_IDENTITY_SIGNER_ZTD ;
  }
  else if (Identity == CERT_RSVD1_INDEX)
  {
    return DFCI_IDENTITY_INVALID;
  }
  else if (Identity == CERT_RSVD2_INDEX)
  {
    return DFCI_IDENTITY_INVALID;
  }
  else
  {
    DEBUG((DEBUG_ERROR, "Invalid Cert Index 0x%X\n", Identity));
    return DFCI_IDENTITY_INVALID;
  }
}

VOID
DebugPrintCertStore(
  IN CONST INTERNAL_CERT_STORE* Store)
{

  if (Store == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - NULL Store pointer\n", __FUNCTION__));
    return;
  }

  DEBUG((DEBUG_INFO, "\n---------- START PRINTING CERT STORE ---------\n"));
  DEBUG((DEBUG_INFO, " Version: 0x%X\n", Store->Version));
  DEBUG((DEBUG_INFO, " Lsv:     0x%X\n", Store->Lsv));
  DEBUG((DEBUG_INFO, " Populated Identities: 0x%X\n", Store->PopulatedIdentities));
  for (INT8 i = 0; i < MAX_NUMBER_OF_CERTS; i++)
  {
    DEBUG((DEBUG_INFO, " Cert[%d]:", i));
    if (Store->Certs[i].Cert != NULL)
    {
      DEBUG((DEBUG_INFO, " PROVISIONED.  Size = 0x%X\n", Store->Certs[i].CertSize));
    }
    else
    {
      DEBUG((DEBUG_INFO, " NOT PRESENT\n"));
    }
  }
  DEBUG((DEBUG_INFO, "---------- END PRINTING CERT STORE ---------\n\n"));
}

/**
This function returns a dynamically populated CertInfo struct members;

The strings can also be NULL.

caller should free the CertInfo members once finished.

@param This               Auth Protocol Instance Pointer
@param Identity           identity to get cert provisioned cert if Cert == NULL
@param Cert               Cert to extract information from
@param CertSize           Sized of caller provided cert.
@param CertRequest        Requested information from cert
@param CertFormat         Format of the returned data
@param Value              Where to store callee provided buffer. Must
                          be freed by caller.
@param ValueSize          If not NULL, where to store size of object returned


@retval EFI_SUCCESS   Cert Information returned
@retval ERROR         Couldn't get certinfo
**/
EFI_STATUS
EFIAPI
GetCertInfo(
  IN CONST DFCI_AUTHENTICATION_PROTOCOL      *This,
  IN       DFCI_IDENTITY_ID                   Identity,
  IN CONST UINT8                             *Cert          OPTIONAL,
  IN       UINTN                              CertSize,
  IN       DFCI_CERT_REQUEST                  CertRequest,
  IN       DFCI_CERT_FORMAT                   CertFormat,
  OUT      VOID                             **Value,
  OUT      UINTN                             *ValueSize     OPTIONAL
  )
{

  EFI_STATUS Status;
  BOOLEAN    UiFormat;
  UINT8  CertDigest[SHA1_FINGERPRINT_DIGEST_SIZE]; // SHA1 Digest size is 20 bytes


  if ((Value == NULL) || (This == NULL) ||
      (CertRequest >= DFCI_CERT_REQUEST_MAX) ||
      (CertFormat >= DFCI_CERT_FORMAT_MAX))
  {
    Status = EFI_INVALID_PARAMETER;
    goto CLEANUP;
  }

  if (Cert == NULL) {
    //Get the Cert
    CertSize = 0;
    Status = GetProvisionedCertDataAndSize(&Cert, &CertSize, Identity);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a: failed to get cert data %r\n", __FUNCTION__, Status));
      goto CLEANUP;
    }
  }

  Status = EFI_UNSUPPORTED;
  UiFormat = FALSE;
  switch (CertRequest) {
    case DFCI_CERT_SUBJECT:
      switch (CertFormat) {
        case DFCI_CERT_FORMAT_CHAR8:
          Status = GetSubjectName8 (Cert, CertSize, CERT_STRING_SIZE, (CHAR8 **) Value, ValueSize);
          break;
        case DFCI_CERT_FORMAT_CHAR16:
          Status = GetSubjectName16 (Cert,CertSize, CERT_STRING_SIZE, (CHAR16 **) Value, ValueSize);
          break;
        default:
          DEBUG((DEBUG_ERROR, "%a: Invalid request format %d for %d\n", CertFormat, CertRequest));
          break;
      }
      break;

    case   DFCI_CERT_ISSUER:
      switch (CertFormat) {
        case DFCI_CERT_FORMAT_CHAR8:
          Status = GetIssuerName8 (Cert, CertSize, CERT_STRING_SIZE, (CHAR8 **) Value, ValueSize);
          break;
        case DFCI_CERT_FORMAT_CHAR16:
          Status = GetIssuerName16 (Cert, CertSize, CERT_STRING_SIZE, (CHAR16 **) Value, ValueSize);
          break;
        default:
          DEBUG((DEBUG_ERROR, "%a: Invalid request format %d for %d\n", CertFormat, CertRequest));
          break;
      }
      break;

    case   DFCI_CERT_THUMBPRINT:
      switch (CertFormat) {
        case DFCI_CERT_FORMAT_CHAR8_UI:
          UiFormat = TRUE;
          // Fall through to next case
        case DFCI_CERT_FORMAT_CHAR8:
          Status = GetSha1Thumbprint8 (Cert, CertSize, UiFormat, (CHAR8 **) Value, ValueSize);
          break;

        case DFCI_CERT_FORMAT_CHAR16_UI:
          UiFormat = TRUE;
          // Fall through to next case
        case DFCI_CERT_FORMAT_CHAR16:
          Status = GetSha1Thumbprint16 (Cert, CertSize, UiFormat, (CHAR16 **) Value, ValueSize);
          break;

        case DFCI_CERT_FORMAT_BINARY:
          Status = GetSha1Thumbprint (Cert, CertSize, &CertDigest);
          if (!EFI_ERROR(Status)) {
            *Value = AllocateCopyPool (SHA1_FINGERPRINT_DIGEST_SIZE, CertDigest);
            if (NULL != *Value) {
              if (NULL != ValueSize) {
                *ValueSize = SHA1_FINGERPRINT_DIGEST_SIZE;
              }
            } else {
              Status = EFI_OUT_OF_RESOURCES;
            }
          }
          break;

        default:
          DEBUG((DEBUG_ERROR, "%a: Invalid request format %d for %d\n", CertFormat, CertRequest));
          break;
      }

      break;

    default:
      Status = EFI_INVALID_PARAMETER;
      goto CLEANUP;
  }

CLEANUP:
  return Status;
}

/**
Function to return the currently enrolled identities within the system.

This is a combination of all identities (not just keys).

@param This               Auth Protocol Instance Pointer
@param EnrolledIdentites  pointer to Mask to be updated


@retval EFI_SUCCESS   EnrolledIdentities will contain a valid MASK for all identities
@retval ERROR         Couldn't get identities

**/
EFI_STATUS
EFIAPI
GetEnrolledIdentities(
  IN CONST DFCI_AUTHENTICATION_PROTOCOL       *This,
  OUT      DFCI_IDENTITY_MASK                 *EnrolledIdentities
  )
{
  if (This == NULL || EnrolledIdentities == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  *EnrolledIdentities = Provisioned();
  return EFI_SUCCESS;
}
