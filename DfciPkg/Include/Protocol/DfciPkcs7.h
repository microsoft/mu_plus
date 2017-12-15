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
