# MuCryptoDxe

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

## About

MuCryptoDxe is a DXE_DRIVER you can include in your platform to have a protocol that can call Crypto functions without having to staticly link against the crypto library in many places

## Supported Architectures

This package is not architecturally dependent.

# Methods supported
There are two protocols exposed in this GUID
### _MU_PKCS5_PASSWORD_HASH_PROTOCOL
### __HashPassword__
Hashes a password by passing through to the BaseCryptLib. Returns EFI_STATUS

NOTE: DigestSize will be used to determine the hash algorithm and must correspond to a known hash digest size. Use standards.

    @retval     EFI_SUCCESS             Congratulations! Your hash is in the output buffer.
    @retval     EFI_INVALID_PARAMETER   One of the pointers was NULL or one of the sizes was too large.
    @retval     EFI_INVALID_PARAMETER   The hash algorithm could not be determined from the digest size.
    @retval     EFI_ABORTED             An error occurred in the OpenSSL subroutines.

**Inputs**:

    IN CONST MU_PKCS5_PASSWORD_HASH_PROTOCOL
    IN UINTN                                      PasswordSize
    IN CONST  CHAR8                              *Password
    IN UINTN                                      SaltSize
    IN CONST  UINT8                              *Salt
    IN UINTN                                      IterationCount
    IN UINTN                                      DigestSize
    IN UINTN                                      OutputSize
    OUT UINT8                                    *Output

## _MU_PKCS7_PROTOCOL
### Verify

Verifies the validity of a PKCS#7 signed data as described in "PKCS #7: Cryptographic Message Syntax Standard". The input signed data could be wrapped in a ContentInfo structure.

If P7Data, TrustedCert or InData is NULL, then return EFI_INVALID_PARAMETER.
If P7Length, CertLength or DataLength overflow, then return EFI_INVALID_PARAMETER.
If this interface is not supported, then return EFI_UNSUPPORTED.


    @retval  EFI_SUCCESS  The specified PKCS#7 signed data is valid.
    @retval  EFI_SECURITY_VIOLATION Invalid PKCS#7 signed data.
    @retval  EFI_UNSUPPORTED This interface is not supported.

**Inputs:**

    IN  CONST MU_PKCS7_PROTOCOL
    IN  CONST UINT8                   *P7Data,
    IN  UINTN                          P7DataLength,
    IN  CONST UINT8                   *TrustedCert,
    IN  UINTN                          TrustedCertLength,
    IN  CONST UINT8                   *Data,
    IN  UINTN                          DataLength (in bytes)

### VerifyEKU

This function receives a PKCS7 formatted signature, and then verifies that the specified Enhanced or Extended Key Usages (EKU's) are present in the end-entity leaf signing certificate.

Note that this function does not validate the certificate chain.

Applications for custom EKU's are quite flexible.  For example, a policy EKU may be present in an Issuing Certificate Authority (CA), and any sub-ordinate certificate issued might also contain this EKU, thus constraining the sub-ordinate certificate.  Other applications might allow a certificate embedded in a device to specify that other Object Identifiers (OIDs) are present which contains binary data specifying custom capabilities that the device is able to do.

    @retval EFI_SUCCESS            - The required EKUs were found in the signature.
    @retval EFI_INVALID_PARAMETER  - A parameter was invalid.
    @retval EFI_NOT_FOUND          - One or more EKU's were not found in the signature.

**Inputs:**

    IN CONST MU_PKCS7_PROTOCOL
    IN CONST UINT8                *Pkcs7Signature,
    IN CONST UINT32                SignatureSize,  (in bytes)
    IN CONST CHAR8                *RequiredEKUs[], null-terminated strings listing OIDs of required EKUs
    IN CONST UINT32                RequiredEKUsSize,
    IN BOOLEAN                     RequireAllPresent


# Including in your platform

## Sample DSC change
    [Components.<arch>]
    ...
    ...
    MsCorePkg/MuCryptoDxe/MuCryptoDxe.inf


## Sample FDF change

    [FV.<a DXE firmware volume>]
    ...
    ...
    INF MsCorePkg/MuCryptoDxe/MuCryptoDxe.inf