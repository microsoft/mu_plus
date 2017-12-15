
#include "IdentityAndAuthManager.h"
#include <Library/BaseCryptLib.h>
#include <openssl/x509v3.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/DfciSerialNumberSupportLib.h>

#define MAX_SUBJECT_ISSUER_LENGTH 300


/**
Function to Get the Subject Name from an X509 cert.  Will return a dynamically
allocated Unicode string.  Caller must free once finished

@param[in] TrustedCert - Raw DER encoded Certificate
@param     CertLength  - Length of the raw cert buffer

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
  CHAR8     *q = NULL;
  CHAR16    *SubjectName = NULL;
  CHAR8     *Start = NULL;
  CHAR8     *End = NULL;
  UINTN     SubjectSize;

  if ((TrustedCert == NULL) || (CertLength == 0) || (MaxStringLength == 0) || (MaxStringLength > MAX_SUBJECT_ISSUER_LENGTH))
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__" Invalid input parameters.\n"));
    goto CLEANUP;
  }

  //Get subject name line from Cert
  q = X509GetUTF8SubjectName(TrustedCert, CertLength);

  if (q == NULL)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__" X509_NAME_oneline failed. result == NULL\n"));
    goto CLEANUP;
  }

  //Loop thru and find the CN (common name)
  Start = AsciiStrStr(q, "CN = ");
  if (Start == NULL)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " Couldn't find CN \n"));
    goto CLEANUP;
  }

  Start += 5;  //Move past "CN = "
  End = AsciiStrStr(Start, ",");
  if (End != NULL)
  {
    *End = '\0';  //insert NULL to end string early
  }

  //allocate CHAR16 and convert
  SubjectSize = (AsciiStrLen(Start) + 1);
  
  if (SubjectSize > MaxStringLength) {
    Start[MaxStringLength - 1] = '\0'; 
    SubjectSize = MaxStringLength;
  }
  SubjectName = (CHAR16 *)AllocatePool(SubjectSize * sizeof(CHAR16));
  if (SubjectName == NULL)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " failed to allocate memory for SubjectName\n"));
    goto CLEANUP;
  }

  AsciiStrToUnicodeStr(Start, SubjectName); 

 CLEANUP:
  if (q)
  {
    //This covers start and end ptrs
    FreePool(q);
  }

  return SubjectName;
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
  X509      *Cert = NULL;
  CHAR8     *q = NULL;
  CHAR16    *Name = NULL;
  CHAR8     *Start = NULL;
  CHAR8     *End = NULL;
  UINTN     Size;

  if ((TrustedCert == NULL) || (CertLength == 0) || (MaxStringLength == 0) || (MaxStringLength > MAX_SUBJECT_ISSUER_LENGTH))
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__" Invalid input parameters.\n"));
    goto CLEANUP;
  }

  //Convert Raw DER Cert into OpenSSL Cert Object 
  Cert = d2i_X509(NULL, &TrustedCert, (UINT32)CertLength);
  if (Cert == NULL)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__" d2i_X509 failed. Cert == NULL\n"));
    goto CLEANUP;
  }

  //Get subject name line from Cert
  q = X509_NAME_oneline(X509_get_issuer_name(Cert), 0, 0);
  if (q == NULL)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__" X509_NAME_oneline failed. result == NULL\n"));
    goto CLEANUP;
  }

  //Loop thru and find the /O (Organization name)
  Start = AsciiStrStr(q, "/O");
  if (Start == NULL)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " Couldn't find Org\n"));
    goto CLEANUP;
  }

  Start += 3;  //Move past "/O "
  End = AsciiStrStr(Start, "/");
  if (End != NULL)
  {
    *End = '\0';  //insert NULL to end string early
  }

  //allocate CHAR16 and convert
  Size = (AsciiStrLen(Start) + 1);

  if (Size > MaxStringLength) {
    Start[MaxStringLength - 1] = '\0';
    Size = MaxStringLength;
  }
  Name = (CHAR16 *)AllocatePool(Size * sizeof(CHAR16));
  if (Name == NULL)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " failed to allocate memory for Name\n"));
    goto CLEANUP;
  }

  AsciiStrToUnicodeStr(Start, Name);

CLEANUP:
  if (q)
  {
    //This covers start and end ptrs
    FreePool(q);
  }
  if (Cert)
  {
    FreePool(Cert);
  }

  return Name;
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
    UnicodeValueToString(Temp, PREFIX_ZERO | RADIX_HEX, Digest[i], 2);
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

