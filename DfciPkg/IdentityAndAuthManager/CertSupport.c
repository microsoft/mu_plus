/**@file
CertSupport.c

Functions to extract certain information from certificates

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
#include <Library/BaseCryptLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/DfciDeviceIdSupportLib.h>

#define MAX_SUBJECT_ISSUER_LENGTH 300

/**
Function to Get the Subject Name from an X509 cert.  Will return a dynamically
allocated Unicode string.  Caller must free once finished

@param[in] TrustedCert - Raw DER encoded Certificate
@param     CertLength  - Length of the raw cert buffer
@param     MaxStringLength - Limit for the number of characters

@retval Unicode String or NULL
**/
CHAR16*
EFIAPI
GetSubjectName(
  IN CONST UINT8    *TrustedCert,
  UINTN    CertLength,
  UINTN    MaxStringLength
  )
{
  CHAR16      *NameBuffer = NULL;
  CHAR8       *AsciiName = NULL;
  UINTN       NameBufferSize;
  UINTN       AsciiNameSize;
  EFI_STATUS  Status;

  if ((TrustedCert == NULL) || (CertLength == 0) || (MaxStringLength == 0) || (MaxStringLength > MAX_SUBJECT_ISSUER_LENGTH))
  {
    DEBUG((DEBUG_ERROR, "%a: Invalid input parameters.\n", __FUNCTION__));
    goto CLEANUP;
  }

  Status = X509GetCommonName (TrustedCert, CertLength, NULL, &AsciiNameSize);
  if (Status != EFI_BUFFER_TOO_SMALL)
  {
      DEBUG((DEBUG_ERROR, "%a: Couldn't get CommonName size\n", __FUNCTION__));
      return NULL;
  }

  AsciiName = AllocatePool (AsciiNameSize);
  if (AsciiName == NULL)
  {
     DEBUG((DEBUG_ERROR, "%a: Unable to allocate memory for common name Ascii.\n", __FUNCTION__));
     return NULL;
  }

  Status = X509GetCommonName (TrustedCert, CertLength, AsciiName, &AsciiNameSize);
  if (EFI_ERROR(Status ))
  {
      DEBUG((DEBUG_ERROR, "%a: Couldn't get CommonName\n", __FUNCTION__));
      return NULL;
  }

  //allocate CHAR16 and convert
  NameBufferSize = AsciiNameSize * sizeof(CHAR16);
  
  if (AsciiNameSize >= MaxStringLength) {
    AsciiName[MaxStringLength -1] = '\0';
    AsciiNameSize = MaxStringLength;
  }
  NameBuffer = (CHAR16 *)AllocatePool(AsciiNameSize * sizeof(CHAR16));
  if (NameBuffer == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a: failed to allocate memory for NameBuffer\n", __FUNCTION__));
    goto CLEANUP;
  }

  AsciiStrToUnicodeStrS(AsciiName, NameBuffer, AsciiNameSize);

 CLEANUP:
  if (AsciiName)
  {
    //This covers start and end ptrs
    FreePool(AsciiName);
  }

  return NameBuffer;
}

/**
Function to Get the Issuer Name from an X509 cert.  Will return a dynamically
allocated Unicode string.  Caller must free once finished

@param[in] TrustedCert - Raw DER encoded Certificate
@param     CertLength  - Length of the raw cert buffer
@param     MaxStringLength - Max length of the return string

@retval Unicode String or NULL
**/
CHAR16*
EFIAPI
GetIssuerName(
  IN CONST UINT8    *TrustedCert,
  UINTN    CertLength,
  UINTN    MaxStringLength
  )
{
  CHAR16      *NameBuffer = NULL;
  CHAR8       *AsciiName = NULL;
  UINTN       NameBufferSize;
  UINTN       AsciiNameSize;
  EFI_STATUS  Status;

  if ((TrustedCert == NULL) || (CertLength == 0) || (MaxStringLength == 0) || (MaxStringLength > MAX_SUBJECT_ISSUER_LENGTH))
  {
    DEBUG((DEBUG_ERROR, "%a: Invalid input parameters.\n", __FUNCTION__));
    goto CLEANUP;
  }

  Status = X509GetOrganizationName (TrustedCert, CertLength, NULL, &AsciiNameSize);
  if (Status != EFI_BUFFER_TOO_SMALL)
  {
      DEBUG((DEBUG_ERROR, "%a: Couldn't get OrganizationName size\n", __FUNCTION__));
      return NULL;
  }

  AsciiName = AllocatePool (AsciiNameSize);
  if (AsciiName == NULL)
  {
     DEBUG((DEBUG_ERROR, "%a: Unable to allocate memory for OrganizationName Ascii.\n", __FUNCTION__));
     return NULL;
  }

  Status = X509GetOrganizationName (TrustedCert, CertLength, AsciiName, &AsciiNameSize);
  if (EFI_ERROR(Status ))
  {
      DEBUG((DEBUG_ERROR, "%a: Couldn't get CommonName\n", __FUNCTION__));
      return NULL;
  }

  //allocate CHAR16 and convert
  NameBufferSize = AsciiNameSize * sizeof(CHAR16);

  if (AsciiNameSize >= MaxStringLength) {
    AsciiName[MaxStringLength -1] = '\0';
    AsciiNameSize = MaxStringLength;
  }
  NameBuffer = (CHAR16 *)AllocatePool(AsciiNameSize * sizeof(CHAR16));
  if (NameBuffer == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a: failed to allocate memory for NameBuffer\n", __FUNCTION__));
    goto CLEANUP;
  }

  AsciiStrToUnicodeStrS(AsciiName, NameBuffer, AsciiNameSize);

 CLEANUP:
  if (AsciiName)
  {
    //This covers start and end ptrs
    FreePool(AsciiName);
  }

  return NameBuffer;
}

/**
Function to compute the Sha1 Thumbprint from an X509 cert.  Will return a dynamically
allocated Unicode string.  Caller must free once finished

@param[in] TrustedCert - Raw DER encoded Certificate
@param     CertLength  - Length of the raw cert buffer

@retval Unicode String containing the Hex bytes or NULL
**/
CHAR16*
EFIAPI
GetSha1Thumbprint(
  IN CONST UINT8    *TrustedCert,
  UINTN    CertLength)
{
  VOID      *Sha1Ctx= NULL;
  UINTN     CtxSize;
  UINT8     Digest[20]; // SHA1 Digest size is 20 bytes
  CHAR16    *Result = NULL;    //SHA1 Digest in Unicode format with space in between.
  BOOLEAN   Flag = FALSE;
  CHAR16    *Temp = NULL;

  if ((TrustedCert == NULL) || (CertLength == 0))
  {
    DEBUG((DEBUG_ERROR, "%a: Invalid input parameters.\n", __FUNCTION__));
    goto CLEANUP;
  }

  //Thumbprint is nothing but SHA1 Digest. There are no library functions available to read this from X509 Cert.
  CtxSize = Sha1GetContextSize();
  Sha1Ctx = AllocatePool(CtxSize);
  if (Sha1Ctx == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a: Failed to allocate Sha1Ctx.\n", __FUNCTION__));
    goto CLEANUP;
  }

  Flag = Sha1Init(Sha1Ctx);
  if (!Flag)
  {
    DEBUG((DEBUG_ERROR, "%a: Failed to Sha1Init.\n", __FUNCTION__));
    goto CLEANUP;
  }

  Flag = Sha1Update(Sha1Ctx, TrustedCert, CertLength);
  if (!Flag)
  {
    DEBUG((DEBUG_ERROR, "%a: Failed to Sha1Update.\n", __FUNCTION__));
    goto CLEANUP;
  }

  Flag = Sha1Final(Sha1Ctx, Digest);
  if (!Flag)
  {
    DEBUG((DEBUG_ERROR, "%a: Failed to Sha1Final.\n", __FUNCTION__));
    goto CLEANUP;
  }

  Result = AllocateZeroPool(((sizeof(Digest) * 3) + 1) * sizeof(CHAR16));  //each byte is 2 hex char and then a space between each char + finally a NULL terminator
  if (Result == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a: Failed to allocate Result string.\n", __FUNCTION__));
    goto CLEANUP;
  }

  Temp = Result;
  for (UINTN i = 0; i < sizeof(Digest); i++)
  {
    if (i != 0) //add space between each byte.
    {
      *Temp = L' ';
      Temp++;
    }
    // Temp is within the result string as computed.  There will always be at least
    // 3 characters left in Temp
    UnicodeValueToStringS(Temp, 3 * sizeof(CHAR16), PREFIX_ZERO | RADIX_HEX, Digest[i], 2);
    Temp += 2;
  }
  *Temp = L'\0';

CLEANUP:
  if (Sha1Ctx) 
  {
    FreePool(Sha1Ctx);
  }
  if (!Flag && (Result != NULL))
  {
    FreePool(Result);
    Result = NULL;
  }
  return Result;
}

