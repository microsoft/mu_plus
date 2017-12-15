
#include "IdentityAndAuthManager.h"
#include <Protocol/Hash.h>  //for the hash guids
#include <Library/BaseCryptLib.h>


EFI_STATUS
EFIAPI
VerifyUsingPkcs1
(
  IN CONST WIN_CERTIFICATE_EFI_PKCS1_15   *WinCert,
  IN CONST UINT8                          *TrustedCertData,
  IN       UINTN                          TrustedCertDataSize,
  IN CONST UINT8                          *SignedData,
  IN       UINTN                          SignedDataLength
  )
{
  EFI_STATUS Status; 
  VOID *HashCtx = NULL;
  VOID *RsaCtx = NULL;
  UINTN HashCtxSize = 0;
  EFI_SHA256_HASH HashBuffer;
  UINT8 *Pkcs1Data = NULL;
  UINTN  Pkcs1DataSize = 0;

  if ((WinCert == NULL) || (TrustedCertData == NULL) || (SignedData == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((SignedDataLength == 0) || (TrustedCertDataSize == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Inspect the WinCert and make sure its supported
  if (!CompareGuid(&WinCert->HashAlgorithm, &gEfiHashAlgorithmSha256Guid))
  {
    DEBUG((DEBUG_ERROR, "%a - Unsupported Hash Algorithm %g\n", __FUNCTION__, &(WinCert->HashAlgorithm)));
    Status = EFI_UNSUPPORTED;
    goto CLEANUP;
  }

  //Setup our pointers and size to PKCS1 data within the WIN_CERT
  Pkcs1DataSize = WinCert->Hdr.dwLength - sizeof(WIN_CERTIFICATE_EFI_PKCS1_15);
  Pkcs1Data = ((UINT8 *)WinCert) + sizeof(WIN_CERTIFICATE_EFI_PKCS1_15);


  // Get the hash digest of the SignedData

  // Sha256 Hash -- if we need to support more types better structure or hash protocol 2 should probably be used
  HashCtxSize = Sha256GetContextSize();
  HashCtx = AllocatePool(HashCtxSize);
  if (HashCtx == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to allocate 0x%X bytes for Hash Buffer\n", __FUNCTION__, HashCtxSize));
    Status = EFI_OUT_OF_RESOURCES;
    goto CLEANUP;
  }

  Sha256Init(HashCtx);
  Sha256Update(HashCtx, SignedData, SignedDataLength);
  Sha256Final(HashCtx, (UINT8*)&HashBuffer);

  DEBUG((DEBUG_INFO, "%a - Sha256 Hash Complete\n", __FUNCTION__));
  DebugDumpMemory(DEBUG_INFO, &HashBuffer, sizeof(HashBuffer), (DEBUG_DM_PRINT_OFFSET));

  //Create a RSA context from the TrustedCertData
  if (!RsaGetPublicKeyFromX509(TrustedCertData, TrustedCertDataSize, &RsaCtx))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to create RSA context from Trusted Cert Data\n", __FUNCTION__));
    Status = EFI_UNSUPPORTED;
    goto CLEANUP;
  }

  //Do RSA Verify with all results
  if(RsaPkcs1Verify(RsaCtx, (UINT8*)&HashBuffer, sizeof(HashBuffer), Pkcs1Data, Pkcs1DataSize))
  {
    Status = EFI_SUCCESS;
  }
  else
  {
    Status = EFI_SECURITY_VIOLATION;
  }

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a Failed to validate the PCKS1 Signature with the supplied key. Status = %r\n", __FUNCTION__, Status));
    goto CLEANUP;
  }

  DEBUG((DEBUG_INFO, "%a Signature Verified.  RSA validation Success\n", __FUNCTION__));

CLEANUP:
  if (RsaCtx != NULL)
  {
    RsaFree(RsaCtx);
  }
  if (HashCtx != NULL)
  {
    FreePool(HashCtx);
  }
  return Status;
}
