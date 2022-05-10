/** @file
  Cryptographic Library instance for host based unit test in MFCI.

  These functions specifically provide PKCS#7 SignedData Verification
  interfaces based on cmocka implementation to inspect input arguments,
  instead of truly performing the signature authentication steps.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

/**
  Verifies the validity of a PKCS#7 signed data as described in "PKCS #7:
  Cryptographic Message Syntax Standard". The input signed data could be wrapped
  in a ContentInfo structure.

  Return FALSE to indicate this interface is not supported.

  @param[in]  P7Data       Pointer to the PKCS#7 message to verify.
  @param[in]  P7Length     Length of the PKCS#7 message in bytes.
  @param[in]  TrustedCert  Pointer to a trusted/root certificate encoded in DER, which
                           is used for certificate chain verification.
  @param[in]  CertLength   Length of the trusted certificate in bytes.
  @param[in]  InData       Pointer to the content to be verified.
  @param[in]  DataLength   Length of InData in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Pkcs7Verify (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  IN  CONST UINT8  *TrustedCert,
  IN  UINTN        CertLength,
  IN  CONST UINT8  *InData,
  IN  UINTN        DataLength
  )
{
  check_expected (P7Data);
  check_expected (P7Length);

  return (BOOLEAN)mock ();
}

/**
  Extracts the attached content from a PKCS#7 signed data if existed. The input signed
  data could be wrapped in a ContentInfo structure.

  Return FALSE to indicate this interface is not supported.

  @param[in]   P7Data       Pointer to the PKCS#7 signed data to process.
  @param[in]   P7Length     Length of the PKCS#7 signed data in bytes.
  @param[out]  Content      Pointer to the extracted content from the PKCS#7 signedData.
                            It's caller's responsibility to free the buffer with FreePool().
  @param[out]  ContentSize  The size of the extracted content in bytes.

  @retval     TRUE          The P7Data was correctly formatted for processing.
  @retval     FALSE         The P7Data was not correctly formatted for processing.

**/
BOOLEAN
EFIAPI
Pkcs7GetAttachedContent (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT VOID         **Content,
  OUT UINTN        *ContentSize
  )
{
  CONST VOID  *ret_buf;
  UINTN       ret_size;

  assert_non_null (P7Data);
  assert_non_null (Content);
  assert_non_null (ContentSize);

  // The incoming should be either of the current or target blob
  check_expected (P7Data);
  check_expected (P7Length);

  ret_buf = (VOID *)mock ();
  if (ret_buf != NULL) {
    ret_size     = (UINTN)mock ();
    *ContentSize = ret_size;
    *Content     = AllocateCopyPool (*ContentSize, ret_buf);
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  This function receives a PKCS#7 formatted signature blob,
  looks for the EKU SEQUENCE blob, and if found then looks
  for all the required EKUs. This function was created so that
  the Surface team can cut down on the number of Certificate
  Authorities (CA's) by checking EKU's on leaf signers for
  a specific product. This prevents one product's certificate
  from signing another product's firmware or unlock blobs.

  Note that this function does not validate the certificate chain.
  That needs to be done before using this function.

  @param[in]  Pkcs7Signature       The PKCS#7 signed information content block. An array
                                   containing the content block with both the signature,
                                   the signer's certificate, and any necessary intermediate
                                   certificates.
  @param[in]  Pkcs7SignatureSize   Number of bytes in Pkcs7Signature.
  @param[in]  RequiredEKUs         Array of null-terminated strings listing OIDs of
                                   required EKUs that must be present in the signature.
  @param[in]  RequiredEKUsSize     Number of elements in the RequiredEKUs string array.
  @param[in]  RequireAllPresent    If this is TRUE, then all of the specified EKU's
                                   must be present in the leaf signer.  If it is
                                   FALSE, then we will succeed if we find any
                                   of the specified EKU's.

  @retval EFI_SUCCESS              The required EKUs were found in the signature.
  @retval EFI_INVALID_PARAMETER    A parameter was invalid.
  @retval EFI_NOT_FOUND            One or more EKU's were not found in the signature.

**/
EFI_STATUS
EFIAPI
VerifyEKUsInPkcs7Signature (
  IN CONST UINT8   *Pkcs7Signature,
  IN CONST UINT32  SignatureSize,
  IN CONST CHAR8   *RequiredEKUs[],
  IN CONST UINT32  RequiredEKUsSize,
  IN BOOLEAN       RequireAllPresent
  )
{
  assert_int_equal (RequiredEKUsSize, 1);
  assert_non_null (RequiredEKUs);
  assert_memory_equal (
    (CHAR8 *)FixedPcdGetPtr (PcdMfciPkcs7RequiredLeafEKU),
    RequiredEKUs[0],
    AsciiStrSize (FixedPcdGetPtr (PcdMfciPkcs7RequiredLeafEKU))
    );
  assert_true (RequireAllPresent);

  check_expected (Pkcs7Signature);
  check_expected (SignatureSize);

  return (EFI_STATUS)mock ();
}
