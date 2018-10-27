/** @file
This driver supports basic Pkcs 7 crypto functions


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

#ifndef __MU_PKCS7_H__
#define __MU_PKCS7_H__

typedef struct _MU_PKCS7_PROTOCOL  MU_PKCS7_PROTOCOL;

/**
Verifies the validility of a PKCS#7 signed data as described in "PKCS #7:
Cryptographic Message Syntax Standard". The input signed data could be wrapped
in a ContentInfo structure.

If P7Data, TrustedCert or InData is NULL, then return EFI_INVALID_PARAMETER.
If P7Length, CertLength or DataLength overflow, then return EFI_INVALID_PARAMETER.
If this interface is not supported, then return EFI_UNSUPPORTED.

@param[in]  P7Data              Pointer to the PKCS#7 message to verify.
@param[in]  P7Length            Length of the PKCS#7 message in bytes.
@param[in]  TrustedCert         Pointer to a trusted/root certificate encoded in DER, which
is used for certificate chain verification.
@param[in]  TrustedCertLength   Length of the trusted certificate in bytes.
@param[in]  Data                Pointer to the content to be verified.
@param[in]  DataLength          Length of Data in bytes.

@retval  EFI_SUCCESS  The specified PKCS#7 signed data is valid.
@retval  EFI_SECURITY_VIOLATION Invalid PKCS#7 signed data.
@retval  EFI_UNSUPPORTED This interface is not supported.

**/
typedef
EFI_STATUS
(EFIAPI *MU_PKCS7_VERIFY) (
IN  CONST MU_PKCS7_PROTOCOL     *This,
IN  CONST UINT8                   *P7Data,
IN  UINTN                          P7DataLength,
IN  CONST UINT8                   *TrustedCert,
IN  UINTN                          TrustedCertLength,
IN  CONST UINT8                   *Data,
IN  UINTN                          DataLength
);


/**
  VerifyEKUsInPkcs7Signature()

  This function receives a PKCS7 formatted signature, and then verifies that 
  the specified Enhanced or Extended Key Usages (EKU's) are present in the end-entity 
  leaf signing certificate.

  Note that this function does not validate the certificate chain.
  
  Applications for custom EKU's are quite flexible.  For example, a policy EKU
  may be present in an Issuing Certificate Authority (CA), and any sub-ordinate
  certificate issued might also contain this EKU, thus constraining the 
  sub-ordinate certificate.  Other applications might allow a certificate 
  embedded in a device to specify that other Object Identifiers (OIDs) are
  present which contains binary data specifying custom capabilities that 
  the device is able to do.
 
  @param[in]  Pkcs7Signature     - The PKCS#7 signed information content block. An array
                                   containing the content block with both the signature,
                                   the signer's certificate, and any necessary intermediate
                                   certificates.
 
  @param[in]  Pkcs7SignatureSize - Number of bytes in Pkcs7Signature.
 
  @param[in]  RequiredEKUs       - Array of null-terminated strings listing OIDs of
                                   required EKUs that must be present in the signature.
 
  @param[in]  RequiredEKUsSize   - Number of elements in the RequiredEKUs string array.

  @param[in]  RequireAllPresent  - If this is TRUE, then all of the specified EKU's
                                   must be present in the leaf signer.  If it is
                                   FALSE, then we will succeed if we find any
                                   of the specified EKU's.

  @retval EFI_SUCCESS            - The required EKUs were found in the signature.
  @retval EFI_INVALID_PARAMETER  - A parameter was invalid.
  @retval EFI_NOT_FOUND          - One or more EKU's were not found in the signature.

**/
typedef
EFI_STATUS
(EFIAPI *MU_PKCS7_VERIFY_EKU) (
IN CONST MU_PKCS7_PROTOCOL  *This,
IN CONST UINT8                *Pkcs7Signature,
IN CONST UINT32                SignatureSize,
IN CONST CHAR8                *RequiredEKUs[],
IN CONST UINT32                RequiredEKUsSize,
IN BOOLEAN                     RequireAllPresent
);


///
/// PKCS7 protocol
///
struct _MU_PKCS7_PROTOCOL
{
  MU_PKCS7_VERIFY_EKU VerifyEKU;
  MU_PKCS7_VERIFY Verify;
};

extern EFI_GUID gMuPKCS7ProtocolGuid;

#endif // __MU_PKCS7_H__
