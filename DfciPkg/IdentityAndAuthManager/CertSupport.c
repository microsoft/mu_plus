
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
    DEBUG((DEBUG_ERROR, __FUNCTION__" Invalid input parameters.\n"));
    goto CLEANUP;
  }

  Status = X509GetCommonName (TrustedCert, CertLength, NULL, &AsciiNameSize);
  if (Status != EFI_BUFFER_TOO_SMALL)
  {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " Couldn't get CommonName size\n"));
      return NULL;
  }

  AsciiName = AllocatePool (AsciiNameSize);
  if (AsciiName == NULL)
  {
     DEBUG((DEBUG_ERROR, __FUNCTION__ "Unable to allocate memory for common name Ascii.\n"));
     return NULL;
  }

  Status = X509GetCommonName (TrustedCert, CertLength, AsciiName, &AsciiNameSize);
  if (EFI_ERROR(Status ))
  {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " Couldn't get CommonName\n"));
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
    DEBUG((DEBUG_ERROR, __FUNCTION__ " failed to allocate memory for NameBuffer\n"));
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
    DEBUG((DEBUG_ERROR, __FUNCTION__" Invalid input parameters.\n"));
    goto CLEANUP;
  }

  Status = X509GetOrganizationName (TrustedCert, CertLength, NULL, &AsciiNameSize);
  if (Status != EFI_BUFFER_TOO_SMALL)
  {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " Couldn't get OrganizationName size\n"));
      return NULL;
  }

  AsciiName = AllocatePool (AsciiNameSize);
  if (AsciiName == NULL)
  {
     DEBUG((DEBUG_ERROR, __FUNCTION__ "Unable to allocate memory for OrganizationName Ascii.\n"));
     return NULL;
  }

  Status = X509GetOrganizationName (TrustedCert, CertLength, AsciiName, &AsciiNameSize);
  if (EFI_ERROR(Status ))
  {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " Couldn't get CommonName\n"));
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
    DEBUG((DEBUG_ERROR, __FUNCTION__ " failed to allocate memory for NameBuffer\n"));
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
    DEBUG((DEBUG_ERROR, __FUNCTION__" Invalid input parameters.\n"));
    goto CLEANUP;
  }

  //Thumbprint is nothing but SHA1 Digest. There are no library functions available to read this from X509 Cert.
  CtxSize = Sha1GetContextSize();
  Sha1Ctx = AllocatePool(CtxSize);
  if (Sha1Ctx == NULL)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__" Failed to allocate Sha1Ctx.\n"));
    goto CLEANUP;
  }

  Flag = Sha1Init(Sha1Ctx);
  if (!Flag)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__" Failed to Sha1Init.\n"));
    goto CLEANUP;
  }

  Flag = Sha1Update(Sha1Ctx, TrustedCert, CertLength);
  if (!Flag)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__" Failed to Sha1Update.\n"));
    goto CLEANUP;
  }

  Flag = Sha1Final(Sha1Ctx, Digest);
  if (!Flag)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__" Failed to Sha1Final.\n"));
    goto CLEANUP;
  }

  Result = AllocateZeroPool(((sizeof(Digest) * 3) + 1) * sizeof(CHAR16));  //each byte is 2 hex char and then a space between each char + finally a NULL terminator
  if (Result == NULL)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__" Failed to allocate Result string.\n"));
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

