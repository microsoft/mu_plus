/** @file
DfciPkcs7.h

This protocol to provide Pkcs7 validation.

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

#ifndef __DFCI_PKCS7_H__
#define __DFCI_PKCS7_H__

typedef struct _DFCI_PKCS7_PROTOCOL  DFCI_PKCS7_PROTOCOL;

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
(EFIAPI *DFCI_PKCS7_VERIFY) (
IN  CONST DFCI_PKCS7_PROTOCOL     *This,
IN  CONST UINT8                   *P7Data,
IN  UINTN                          P7DataLength,
IN  CONST UINT8                   *TrustedCert,
IN  UINTN                          TrustedCertLength,
IN  CONST UINT8                   *Data,
IN  UINTN                          DataLength
);

///
/// PKCS7 protocol
///
struct _DFCI_PKCS7_PROTOCOL
{
  DFCI_PKCS7_VERIFY Verify;
};

extern EFI_GUID gDfciPKCS7ProtocolGuid;

#endif // __DFCI_PKCS7_H__
