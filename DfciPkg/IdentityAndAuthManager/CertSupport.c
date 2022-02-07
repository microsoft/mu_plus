/**@file
CertSupport.c

Functions to extract certain information from certificates

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "IdentityAndAuthManager.h"
#include <Library/BaseCryptLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/DfciDeviceIdSupportLib.h>

/**
Function to Get the Subject Name from an X509 cert.  Will return a dynamically
allocated ASCII string.  Caller must free once finished

@param[in]  TrustedCert - Raw DER encoded Certificate
@param      CertLength  - Length of the raw cert buffer
@param      MaxStringLength - Limit for the number of characters
@param[out] Value       - Where to store pointer to value
@param[out] ValueSize   - where to store value size

@retval Unicode String or NULL
**/
EFI_STATUS
EFIAPI
GetSubjectName8 (
  IN CONST UINT8  *TrustedCert,
  IN       UINTN  CertLength,
  IN       UINTN  MaxStringLength,
  OUT      CHAR8  **Value,
  OUT      UINTN  *ValueSize  OPTIONAL
  )
{
  CHAR8       *AsciiName = NULL;
  UINTN       AsciiNameSize;
  EFI_STATUS  Status;

  if ((TrustedCert == NULL) || (CertLength == 0) ||
      (Value == NULL) || (MaxStringLength == 0) || (MaxStringLength > MAX_SUBJECT_ISSUER_LENGTH))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid input parameters.\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  Status = X509GetCommonName (TrustedCert, CertLength, NULL, &AsciiNameSize);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_ERROR, "%a: Couldn't get CommonName size\n", __FUNCTION__));
    return Status;
  }

  AsciiName = AllocatePool (AsciiNameSize);
  if (AsciiName == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Unable to allocate memory for common name Ascii.\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = X509GetCommonName (TrustedCert, CertLength, AsciiName, &AsciiNameSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Couldn't get CommonName\n", __FUNCTION__));
    FreePool (AsciiName);
    return Status;
  }

  if (AsciiNameSize > (MaxStringLength + sizeof (CHAR8))) {
    AsciiNameSize = MaxStringLength + sizeof (CHAR8);
    AsciiStrCpyS (&AsciiName[MaxStringLength-sizeof (MORE_INDICATOR)+1], sizeof (MORE_INDICATOR), MORE_INDICATOR);
  }

  *Value = AsciiName;
  if (NULL != ValueSize) {
    *ValueSize = AsciiNameSize;
  }

  return Status;
}

/**
Function to Get the Subject Name from an X509 cert.  Will return a dynamically
allocated Unicode string.  Caller must free once finished

@param[in]  TrustedCert - Raw DER encoded Certificate
@param      CertLength  - Length of the raw cert buffer
@param      MaxStringLength - Limit for the number of characters
@param[out] Value       - Where to store pointer to value
@param[out] ValueSize   - where to store value size

@retval Unicode String or NULL
**/
EFI_STATUS
EFIAPI
GetSubjectName16 (
  IN CONST UINT8   *TrustedCert,
  IN       UINTN   CertLength,
  IN       UINTN   MaxStringLength,
  OUT      CHAR16  **Value,
  OUT      UINTN   *ValueSize  OPTIONAL
  )
{
  CHAR8       *AsciiName = NULL;
  UINTN       AsciiNameSize;
  CHAR16      *NameBuffer = NULL;
  UINTN       NameBufferSize;
  EFI_STATUS  Status;

  if ((TrustedCert == NULL) || (CertLength == 0) || (MaxStringLength == 0) || (MaxStringLength > MAX_SUBJECT_ISSUER_LENGTH)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid input parameters.\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetSubjectName8 (TrustedCert, CertLength, MaxStringLength, &AsciiName, &AsciiNameSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // allocate CHAR16 and convert
  NameBufferSize = AsciiNameSize * sizeof (CHAR16);

  NameBuffer = (CHAR16 *)AllocatePool (NameBufferSize);
  if (NameBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed to allocate memory for NameBuffer\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AsciiStrToUnicodeStrS (AsciiName, NameBuffer, AsciiNameSize);
  if (!EFI_ERROR (Status)) {
    *Value = NameBuffer;
    if (NULL != ValueSize) {
      *ValueSize = NameBufferSize;
    }
  }

  FreePool (AsciiName);

  return Status;
}

/**
Function to Get the Issuer Name from an X509 cert.  Will return a dynamically
allocated Ascii string.  Caller must free once finished

@param[in]  TrustedCert - Raw DER encoded Certificate
@param      CertLength  - Length of the raw cert buffer
@param      MaxStringLength - Max length of the return string
@param[out] Value       - Where to store pointer to value
@param[out] ValueSize   - where to store value size

@retval Unicode String or NULL
**/
EFI_STATUS
EFIAPI
GetIssuerName8 (
  IN  CONST UINT8  *TrustedCert,
  IN        UINTN  CertLength,
  IN        UINTN  MaxStringLength,
  OUT       CHAR8  **Value,
  OUT       UINTN  *ValueSize  OPTIONAL
  )
{
  CHAR8       *AsciiName = NULL;
  UINTN       AsciiNameSize;
  EFI_STATUS  Status;

  if ((TrustedCert == NULL) || (CertLength == 0) ||
      (Value == NULL) || (MaxStringLength == 0) || (MaxStringLength > MAX_SUBJECT_ISSUER_LENGTH))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid input parameters.\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  Status = X509GetOrganizationName (TrustedCert, CertLength, NULL, &AsciiNameSize);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_ERROR, "%a: Couldn't get OrganizationName size\n", __FUNCTION__));
    return Status;
  }

  AsciiName = AllocatePool (AsciiNameSize);
  if (AsciiName == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Unable to allocate memory for OrganizationName Ascii.\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = X509GetOrganizationName (TrustedCert, CertLength, AsciiName, &AsciiNameSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Couldn't get CommonName\n", __FUNCTION__));
    FreePool (AsciiName);
    return Status;
  }

  if (AsciiNameSize >= MaxStringLength) {
    AsciiNameSize              = MaxStringLength;
    AsciiName[MaxStringLength] = '\0';
  }

  *Value = AsciiName;
  if (NULL != ValueSize) {
    *ValueSize = AsciiNameSize;
  }

  return Status;
}

/**
Function to Get the Issuer Name from an X509 cert.  Will return a dynamically
allocated Unicode string.  Caller must free once finished

@param[in]  TrustedCert - Raw DER encoded Certificate
@param      CertLength  - Length of the raw cert buffer
@param      MaxStringLength - Max length of the return string
@param[out] Value       - Where to store pointer to value
@param[out] ValueSize   - where to store value size

@retval Unicode String or NULL
**/
EFI_STATUS
EFIAPI
GetIssuerName16 (
  IN CONST UINT8   *TrustedCert,
  IN       UINTN   CertLength,
  IN       UINTN   MaxStringLength,
  OUT      CHAR16  **Value,
  OUT      UINTN   *ValueSize  OPTIONAL

  )
{
  CHAR8       *AsciiName = NULL;
  UINTN       AsciiNameSize;
  CHAR16      *NameBuffer = NULL;
  UINTN       NameBufferSize;
  EFI_STATUS  Status;

  if ((TrustedCert == NULL) || (CertLength == 0) || (MaxStringLength == 0) || (MaxStringLength > MAX_SUBJECT_ISSUER_LENGTH)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid input parameters.\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetIssuerName8 (TrustedCert, CertLength, MaxStringLength, &AsciiName, &AsciiNameSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // allocate CHAR16 and convert
  NameBufferSize = AsciiNameSize * sizeof (CHAR16);

  NameBuffer = (CHAR16 *)AllocatePool (AsciiNameSize * sizeof (CHAR16));
  if (NameBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed to allocate memory for NameBuffer\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiStrToUnicodeStrS (AsciiName, NameBuffer, AsciiNameSize);
  FreePool (AsciiName);
  *Value = NameBuffer;
  if (NULL != ValueSize) {
    *ValueSize = NameBufferSize;
  }

  return Status;
}

/**
Function to compute the Sha1 Thumbprint from an X509 cert.  Fills in the caller provided digest
with the sha1 hash.

@param[in]  TrustedCert - Raw DER encoded Certificate
@param      CertLength  - Length of the raw cert buffer
@param[out] CertDigest  - Binary shaw1 digest

@retval EFI_SUCCESS     - CertDigest filled in
        error           - unable to get the digest.
**/
EFI_STATUS
EFIAPI
GetSha1Thumbprint (
  IN  CONST UINT8  *TrustedCert,
  IN        UINTN  CertLength,
  OUT       UINT8 (*CertDigest)[SHA1_FINGERPRINT_DIGEST_SIZE]
  )
{
  VOID        *Sha1Ctx = NULL;
  UINTN       CtxSize;
  BOOLEAN     Flag = FALSE;
  EFI_STATUS  Status;

  if ((TrustedCert == NULL) || (CertLength == 0) || (CertDigest == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid input parameters.\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  // Thumbprint is nothing but SHA1 Digest. There are no library functions available to read this from X509 Cert.
  CtxSize = Sha1GetContextSize ();
  Sha1Ctx = AllocatePool (CtxSize);
  if (Sha1Ctx == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate Sha1Ctx.\n", __FUNCTION__));
    Status = EFI_OUT_OF_RESOURCES;
    goto CLEANUP;
  }

  Status = EFI_ABORTED;
  Flag   = Sha1Init (Sha1Ctx);
  if (!Flag) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to Sha1Init.\n", __FUNCTION__));
    goto CLEANUP;
  }

  Flag = Sha1Update (Sha1Ctx, TrustedCert, CertLength);
  if (!Flag) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to Sha1Update.\n", __FUNCTION__));
    goto CLEANUP;
  }

  Flag = Sha1Final (Sha1Ctx, (UINT8 *)CertDigest);
  if (!Flag) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to Sha1Final.\n", __FUNCTION__));
    goto CLEANUP;
  }

  Status = EFI_SUCCESS;

CLEANUP:
  if (Sha1Ctx) {
    FreePool (Sha1Ctx);
  }

  return Status;
}

/**
Function to compute the Sha1 Thumbprint from an X509 cert.  Will return a dynamically
allocated Unicode string.  Caller must free once finished

@param[in]  TrustedCert - Raw DER encoded Certificate
@param      CertLength  - Length of the raw cert buffer
@param      UiFormat    - TRUE, add space after every two bytes
@param[in]  Value       - Where to store pointer to allocated buffer
@param[out] ValueSize   - If not null, size of allocated buffer

@retval EFI_SUCCESS if value stored, error if not
**/
EFI_STATUS
EFIAPI
GetSha1Thumbprint8 (
  IN CONST UINT8    *TrustedCert,
  IN       UINTN    CertSize,
  IN       BOOLEAN  UiFormat,
  OUT      CHAR8    **Value,
  OUT      UINTN    *ValueSize  OPTIONAL
  )
{
  UINT8       CertDigest[SHA1_FINGERPRINT_DIGEST_SIZE]; // SHA1 Digest size is 20 bytes
  UINTN       FormatSize;
  CHAR8       *Result;
  UINTN       ResultSize;
  EFI_STATUS  Status;
  CHAR8       *Temp;

  if (NULL == Value) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetSha1Thumbprint (TrustedCert, CertSize, &CertDigest);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // each byte is 2 hex chars and, if UiFormat is TRUE, a space between each char, + finally a NULL terminator
  FormatSize = (UiFormat) ? 3 : 2;
  ResultSize = (sizeof (CertDigest) * FormatSize) + 1;
  Result     = AllocateZeroPool (ResultSize);
  if (Result == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate Result string.\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EFI_INVALID_PARAMETER;
  Temp   = Result;
  for (UINTN i = 0; i < sizeof (CertDigest); i++) {
    if (UiFormat && (i != 0)) {
      // add space between each byte.
      *Temp = L' ';
      Temp++;
    }

    // Temp is within the result string as computed.  There will always be at least
    // 3 characters left in Temp
    Status = AsciiValueToStringS (Temp, (FormatSize + 1), PREFIX_ZERO | RADIX_HEX, CertDigest[i], 2);
    if (EFI_ERROR (Status)) {
      break;
    }

    Temp += 2;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error %r\n", __FUNCTION__, Status));
    FreePool (Result);
  } else {
    *Temp  = L'\0';
    *Value = Result;
    if (NULL != ValueSize) {
      *ValueSize = ResultSize;
    }
  }

  return Status;
}

/**
Function to compute the Sha1 Thumbprint from an X509 cert.  Will return a dynamically
allocated Unicode string.  Caller must free once finished

@param[in]  TrustedCert - Raw DER encoded Certificate
@param      CertLength  - Length of the raw cert buffer
@param      UiFormat    - TRUE, add space after every two bytes
@param[in]  Value       - Where to store pointer to allocated buffer
@param[out] ValueSize   - If not null, size of allocated buffer

@retval EFI_SUCCESS if value stored, error if not
**/
EFI_STATUS
EFIAPI
GetSha1Thumbprint16 (
  IN CONST UINT8    *TrustedCert,
  IN       UINTN    CertSize,
  IN       BOOLEAN  UiFormat,
  OUT      CHAR16   **Value,
  OUT      UINTN    *ValueSize  OPTIONAL
  )
{
  UINT8       CertDigest[SHA1_FINGERPRINT_DIGEST_SIZE]; // SHA1 Digest size is 20 bytes
  UINTN       FormatSize;
  CHAR16      *Result;
  UINTN       ResultSize;
  EFI_STATUS  Status;
  CHAR16      *Temp;

  if (NULL == Value) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetSha1Thumbprint (TrustedCert, CertSize, &CertDigest);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // each byte is 2 hex chars and, if UiFormat is TRUE, a space between each char, + finally a NULL terminator
  FormatSize = (UiFormat) ? 3 : 2;
  ResultSize = ((sizeof (CertDigest) * FormatSize) + 1) * sizeof (CHAR16);
  Result     = AllocateZeroPool (ResultSize);
  if (Result == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate Result string.\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EFI_INVALID_PARAMETER;
  Temp   = Result;
  for (UINTN i = 0; i < sizeof (CertDigest); i++) {
    if (UiFormat && (i != 0)) {
      // add space between each byte if UiFormat == TRUE.
      *Temp = L' ';
      Temp++;
    }

    // Temp is within the result string as computed.  There will always be at least
    // 3 characters left in Temp
    Status = UnicodeValueToStringS (Temp, (FormatSize + 1) * sizeof (CHAR16), PREFIX_ZERO | RADIX_HEX, CertDigest[i], 2);
    if (EFI_ERROR (Status)) {
      break;
    }

    Temp += 2;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error %r\n", __FUNCTION__, Status));
    FreePool (Result);
  } else {
    *Temp  = L'\0';
    *Value = Result;
    if (NULL != ValueSize) {
      *ValueSize = ResultSize;
    }
  }

  return Status;
}
