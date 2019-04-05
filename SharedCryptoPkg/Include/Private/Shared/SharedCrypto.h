
/** @file
This is the shared data between the DXE protocol, the SMM protocol, and the PEI PPI

Copyright (c) 2019, Microsoft Corporation

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

#include <Library/BaseCryptLib.h>

#ifndef __SHARED_CRYPTO_H__
#define __SHARED_CRYPTO_H__

#define SHARED_CRYPTO_VERSION 1

typedef struct _SHARED_CRYPTO_FUNCTIONS SHARED_CRYPTO_FUNCTIONS;


//A function that returns the version number of the packaged crypto efi
typedef
UINTN
(EFIAPI *CBD_GET_VERSION) (
  VOID
);

//=====================================================================================
//    MAC (Message Authentication Code) Primitive
//=====================================================================================

typedef
UINTN
(EFIAPI *CBD_HMAC_MD5_GetContextSize) (
  VOID
  );

typedef
VOID*
(EFIAPI *CBD_HMAC_MD5_New) (
  VOID
  );


typedef
VOID
(EFIAPI *CBD_HMAC_MD5_Free) (
  IN  VOID  *HmacMd5Ctx
  );


typedef
BOOLEAN
(EFIAPI *CBD_HMAC_MD5_Init) (
  OUT  VOID         *HmacMd5Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  );


typedef
BOOLEAN
(EFIAPI *CBD_HMAC_MD5_Duplicate) (
  IN   CONST VOID  *HmacMd5Context,
  OUT  VOID        *NewHmacMd5Context
  );


typedef
BOOLEAN
(EFIAPI *CBD_HMAC_MD5_Update) (
  IN OUT  VOID        *HmacMd5Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );


typedef
BOOLEAN
(EFIAPI *CBD_HMAC_MD5_Final) (
  IN OUT  VOID   *HmacMd5Context,
  OUT     UINT8  *HmacValue
  );
typedef
UINTN
(EFIAPI *CBD_HMAC_SHA1_GetContextSize )(
  VOID
  );

typedef
VOID*
(EFIAPI *CBD_HMAC_SHA1_New) (
  VOID
  );


typedef
VOID
(EFIAPI *CBD_HMAC_SHA1_Free) (
  IN  VOID  *HmacSha1Ctx
  );


typedef
BOOLEAN
(EFIAPI *CBD_HMAC_SHA1_Init) (
  OUT  VOID         *HmacSha1Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  );


typedef
BOOLEAN
(EFIAPI *CBD_HMAC_SHA1_Duplicate) (
  IN   CONST VOID  *HmacSha1Context,
  OUT  VOID        *NewHmacSha1Context
  );


typedef
BOOLEAN
(EFIAPI *CBD_HMAC_SHA1_Update) (
  IN OUT  VOID        *HmacSha1Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );


typedef
BOOLEAN
(EFIAPI *CBD_HMAC_SHA1_Final) (
  IN OUT  VOID   *HmacSha1Context,
  OUT     UINT8  *HmacValue
  );


typedef
UINTN
(EFIAPI *CBD_HMAC_SHA256_GetContextSize) (
  VOID
  );

typedef
VOID *
(EFIAPI *CBD_HMAC_SHA256_New) (
  VOID
  );


typedef
VOID
(EFIAPI *CBD_HMAC_SHA256_Free) (
  IN  VOID  *HmacSha256Ctx
  );


typedef
BOOLEAN
(EFIAPI *CBD_HMAC_SHA256_Init) (
  OUT  VOID         *HmacSha256Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  );


typedef
BOOLEAN
(EFIAPI *CBD_HMAC_SHA256_Duplicate) (
  IN   CONST VOID  *HmacSha256Context,
  OUT  VOID        *NewHmacSha256Context
  );


typedef
BOOLEAN
(EFIAPI *CBD_HMAC_SHA256_Update) (
  IN OUT  VOID        *HmacSha256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );


typedef
BOOLEAN
(EFIAPI *CBD_HMAC_SHA256_Final) (
  IN OUT  VOID   *HmacSha256Context,
  OUT     UINT8  *HmacValue
  );


//=====================================================================================
//    One-Way Cryptographic Hash Primitives
//=====================================================================================

typedef
UINTN
(EFIAPI *CBD_MD4_GetContextSize) (
  VOID
  );


typedef
BOOLEAN
(EFIAPI *CBD_MD4_Init) (
  OUT  VOID  *Md4Context
  );


typedef
BOOLEAN
(EFIAPI *CBD_MD4_Duplicate) (
  IN   CONST VOID  *Md4Context,
  OUT  VOID        *NewMd4Context
  );


typedef
BOOLEAN
(EFIAPI *CBD_MD4_Update) (
  IN OUT  VOID        *Md4Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );


typedef
BOOLEAN
(EFIAPI *CBD_MD4_Final) (
  IN OUT  VOID   *Md4Context,
  OUT     UINT8  *HashValue
  );


typedef
BOOLEAN
(EFIAPI *CBD_MD4_HashAll) (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  );

// ----------------------------------------------------------------------------

typedef
UINTN
(EFIAPI* CBD_MD5_GetContextSize )(
  VOID
  );



typedef
BOOLEAN
(EFIAPI* CBD_MD5_Init)(
    OUT VOID *Md5Context);

typedef
BOOLEAN
(EFIAPI* CBD_MD5_Duplicate) (
    IN CONST VOID *Md5Context,
    OUT VOID *NewMd5Context);


typedef
BOOLEAN
(EFIAPI* CBD_MD5_Update)(
    IN OUT VOID *Md5Context,
    IN CONST VOID *Data,
    IN UINTN DataSize);


typedef
BOOLEAN
(EFIAPI* CBD_MD5_Final)(
    IN OUT VOID *Md5Context,
    OUT UINT8 *HashValue);


typedef
BOOLEAN
(EFIAPI* CBD_MD5_HashAll)(
    IN CONST VOID *Data,
    IN UINTN DataSize,
    OUT UINT8 *HashValue);


typedef
UINTN
(EFIAPI *CBD_SHA384_GetContextSize) (
  VOID
  );


typedef
BOOLEAN
(EFIAPI *CBD_SHA384_Init) (
  OUT  VOID  *Sha384Context
  );


typedef
BOOLEAN
(EFIAPI *CBD_SHA384_Duplicate) (
  IN   CONST VOID  *Sha384Context,
  OUT  VOID        *NewSha384Context
  );


typedef
BOOLEAN
(EFIAPI *CBD_SHA384_Update) (
  IN OUT  VOID        *Sha384Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );


typedef
BOOLEAN
(EFIAPI *CBD_SHA384_Final) (
  IN OUT  VOID   *Sha384Context,
  OUT     UINT8  *HashValue
  );


typedef
BOOLEAN
(EFIAPI *CBD_SHA384_HashAll) (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  );

typedef
UINTN
(EFIAPI *CBD_SHA512_GetContextSize) (
  VOID
  );


typedef
BOOLEAN
(EFIAPI *CBD_SHA512_Init) (
  OUT  VOID  *Sha512Context
  );


typedef
BOOLEAN
(EFIAPI *CBD_SHA512_Duplicate) (
  IN   CONST VOID  *Sha512Context,
  OUT  VOID        *NewSha512Context
  );


typedef
BOOLEAN
(EFIAPI *CBD_SHA512_Update) (
  IN OUT  VOID        *Sha512Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );


typedef
BOOLEAN
(EFIAPI *CBD_SHA512_Final) (
  IN OUT  VOID   *Sha512Context,
  OUT     UINT8  *HashValue
  );


typedef
BOOLEAN
(EFIAPI *CBD_SHA512_HashAll) (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  );

//=====================================================================================
//    MAC (Message Authentication Code) Primitive
//=====================================================================================


typedef
BOOLEAN
(EFIAPI *CBD_PKCS1_ENCRYPT_V2) (
IN   CONST UINT8                   *PublicKey,
IN   UINTN                          PublicKeySize,
IN   UINT8                         *InData,
IN   UINTN                          InDataSize,
IN   CONST UINT8                   *PrngSeed OPTIONAL,
IN   UINTN                          PrngSeedSize OPTIONAL,
OUT  UINT8                        **EncryptedData,
OUT  UINTN                         *EncryptedDataSize
);




// ---------------------------------------------
// PKCS5

typedef
BOOLEAN
(EFIAPI *CBD_PKCS5_PW_HASH) (
  IN UINTN                      PasswordSize,
  IN CONST  CHAR8              *Password,
  IN UINTN                      SaltSize,
  IN CONST  UINT8              *Salt,
  IN UINTN                      IterationCount,
  IN UINTN                      DigestSize,
  IN UINTN                      OutputSize,
  OUT UINT8                    *Output
  );



// ---------------------------------------------
// PKCS7

typedef
BOOLEAN
(EFIAPI *CBD_PKCS7_VERIFY) (
IN  CONST UINT8                   *P7Data,
IN  UINTN                          P7DataLength,
IN  CONST UINT8                   *TrustedCert,
IN  UINTN                          TrustedCertLength,
IN  CONST UINT8                   *Data,
IN  UINTN                          DataLength
);

typedef
EFI_STATUS
(EFIAPI *CBD_PKCS7_VERIFY_EKU) (
IN CONST UINT8                *Pkcs7Signature,
IN CONST UINT32                SignatureSize,
IN CONST CHAR8                *RequiredEKUs[],
IN CONST UINT32                RequiredEKUsSize,
IN BOOLEAN                     RequireAllPresent
);

typedef
BOOLEAN
(EFIAPI *CBD_PKCS7_GetSigners) (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT UINT8        **CertStack,
  OUT UINTN        *StackLength,
  OUT UINT8        **TrustedCert,
  OUT UINTN        *CertLength
  );

typedef
VOID
(EFIAPI *CBD_PKCS7_FreeSigners) (
  IN  UINT8        *Certs
  );

typedef
BOOLEAN
(EFIAPI *CBD_PKCS7_Sign) (
  IN   CONST UINT8  *PrivateKey,
  IN   UINTN        PrivateKeySize,
  IN   CONST UINT8  *KeyPassword,
  IN   UINT8        *InData,
  IN   UINTN        InDataSize,
  IN   UINT8        *SignCert,
  IN   UINT8        *OtherCerts      OPTIONAL,
  OUT  UINT8        **SignedData,
  OUT  UINTN        *SignedDataSize
  );

typedef
BOOLEAN
(EFIAPI *CBD_PKCS7_GetAttachedContent) (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT VOID         **Content,
  OUT UINTN        *ContentSize
  );


typedef
BOOLEAN
(EFIAPI *CBD_PKCS7_GetCertificatesList) (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT UINT8        **SignerChainCerts,
  OUT UINTN        *ChainLength,
  OUT UINT8        **UnchainCerts,
  OUT UINTN        *UnchainLength
  );

typedef
BOOLEAN
(EFIAPI *CBD_AuthenticodeVerify) (
  IN  CONST UINT8  *AuthData,
  IN  UINTN        DataSize,
  IN  CONST UINT8  *TrustedCert,
  IN  UINTN        CertSize,
  IN  CONST UINT8  *ImageHash,
  IN  UINTN        HashSize
  );

typedef
BOOLEAN
(EFIAPI *CBD_ImageTimestampVerify) (
  IN  CONST UINT8  *AuthData,
  IN  UINTN        DataSize,
  IN  CONST UINT8  *TsaCert,
  IN  UINTN        CertSize,
  OUT EFI_TIME     *SigningTime
  );


//=====================================================================================
//    DH Key Exchange Primitive
//=====================================================================================


typedef
VOID*
(EFIAPI *CBD_DH_New) (
  VOID
  );


typedef
VOID
(EFIAPI *CBD_DH_Free) (
  IN  VOID  *DhContext
  );


typedef
BOOLEAN
(EFIAPI *CBD_DH_GenerateParameter) (
  IN OUT  VOID   *DhContext,
  IN      UINTN  Generator,
  IN      UINTN  PrimeLength,
  OUT     UINT8  *Prime
  );

typedef
BOOLEAN
(EFIAPI *CBD_DH_SetParameter) (
  IN OUT  VOID         *DhContext,
  IN      UINTN        Generator,
  IN      UINTN        PrimeLength,
  IN      CONST UINT8  *Prime
  );


typedef
BOOLEAN
(EFIAPI *CBD_DH_GenerateKey) (
  IN OUT  VOID   *DhContext,
  OUT     UINT8  *PublicKey,
  IN OUT  UINTN  *PublicKeySize
  );


typedef
BOOLEAN
(EFIAPI *CBD_DH_ComputeKey) (
  IN OUT  VOID         *DhContext,
  IN      CONST UINT8  *PeerPublicKey,
  IN      UINTN        PeerPublicKeySize,
  OUT     UINT8        *Key,
  IN OUT  UINTN        *KeySize
  );

//=====================================================================================
//    Pseudo-Random Generation Primitive
//=====================================================================================

typedef
BOOLEAN
(EFIAPI *CBD_RANDOM_Seed) (
  IN  CONST  UINT8  *Seed  OPTIONAL,
  IN  UINTN         SeedSize
  );


typedef
BOOLEAN
(EFIAPI *CBD_RANDOM_Bytes) (
  OUT  UINT8  *Output,
  IN   UINTN  Size
  );

typedef
BOOLEAN
(EFIAPI *CBD_RSA_VERIFY_PKCS1 ) (
  IN  VOID                         *RsaContext,
  IN  CONST UINT8                  *MessageHash,
  IN  UINTN                         HashSize,
  IN  CONST UINT8                  *Signature,
  IN  UINTN                         SigSize
  );


typedef
VOID
(EFIAPI *CBD_RSA_FREE) (
  IN  VOID  *RsaContext
  );


typedef
BOOLEAN
(EFIAPI *CBD_RSA_GET_PUBLIC_KEY_FROM_X509) (
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  VOID         **RsaContext
  );

typedef
VOID*
(EFIAPI *CBD_RSA_New) (
  VOID
  );


typedef
VOID
(EFIAPI *CBD_RSA_Free) (
  IN  VOID  *RsaContext
  );

typedef
BOOLEAN
(EFIAPI *CBD_RSA_SetKey) (
  IN OUT  VOID         *RsaContext,
  IN      RSA_KEY_TAG  KeyTag,
  IN      CONST UINT8  *BigNumber,
  IN      UINTN        BnSize
  );


typedef
BOOLEAN
(EFIAPI *CBD_RSA_GetKey) (
  IN OUT  VOID         *RsaContext,
  IN      RSA_KEY_TAG  KeyTag,
  OUT     UINT8        *BigNumber,
  IN OUT  UINTN        *BnSize
  );

typedef
BOOLEAN
(EFIAPI *CBD_RSA_GenerateKey) (
  IN OUT  VOID         *RsaContext,
  IN      UINTN        ModulusLength,
  IN      CONST UINT8  *PublicExponent,
  IN      UINTN        PublicExponentSize
  );


typedef
BOOLEAN
(EFIAPI *CBD_RSA_CheckKey) (
  IN  VOID  *RsaContext
  );

typedef
BOOLEAN
(EFIAPI *CBD_RSA_Pkcs1Sign) (
  IN      VOID         *RsaContext,
  IN      CONST UINT8  *MessageHash,
  IN      UINTN        HashSize,
  OUT     UINT8        *Signature,
  IN OUT  UINTN        *SigSize
  );


typedef
BOOLEAN
(EFIAPI *CBD_RSA_Pkcs1Verify) (
  IN  VOID         *RsaContext,
  IN  CONST UINT8  *MessageHash,
  IN  UINTN        HashSize,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigSize
  );


typedef
BOOLEAN
(EFIAPI *CBD_RSA_GetPrivateKeyFromPem) (
  IN   CONST UINT8  *PemData,
  IN   UINTN        PemSize,
  IN   CONST CHAR8  *Password,
  OUT  VOID         **RsaContext
  );


typedef
BOOLEAN
(EFIAPI *CBD_RSA_GetPublicKeyFromX509) (
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  VOID         **RsaContext
  );

//----------------------------------------
// SHA
//----------------------------------------

typedef
UINTN
(EFIAPI *CBD_SHA1_GET_CONTEXT_SIZE ) (
  VOID
  );


typedef
BOOLEAN
(EFIAPI *CBD_SHA1_INIT ) (
  OUT  VOID  *Sha1Context
  );


typedef
BOOLEAN
(EFIAPI *CBD_SHA1_DUPLICATE ) (
  IN   CONST VOID  *Sha1Context,
  OUT  VOID        *NewSha1Context
  );


typedef
BOOLEAN
(EFIAPI *CBD_SHA1_UPDATE ) (
  IN OUT  VOID        *Sha1Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );



typedef
BOOLEAN
(EFIAPI *CBD_SHA1_FINAL ) (
  IN OUT  VOID   *Sha1Context,
  OUT     UINT8  *HashValue
  );



typedef
BOOLEAN
(EFIAPI *CBD_SHA1_HASH_ALL ) (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  );

typedef
UINTN
(EFIAPI *CBD_SHA256_GET_CONTEXT_SIZE ) (
  VOID
);


typedef
BOOLEAN
(EFIAPI *CBD_SHA256_INIT ) (
  OUT  VOID  *Sha256Context
  );


typedef
BOOLEAN
(EFIAPI *CBD_SHA256_DUPLICATE ) (
  IN   CONST VOID  *Sha256Context,
  OUT  VOID        *NewSha256Context
  );


typedef
BOOLEAN
(EFIAPI *CBD_SHA256_UPDATE ) (
  IN OUT  VOID        *Sha256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );



typedef
BOOLEAN
(EFIAPI *CBD_SHA256_FINAL ) (
  IN OUT  VOID   *Sha256Context,
  OUT     UINT8  *HashValue
  );



typedef
BOOLEAN
(EFIAPI *CBD_SHA256_HASH_ALL ) (
  IN   CONST VOID                  *Data,
  IN   UINTN                       DataSize,
  OUT  UINT8                       *HashValue
  );



typedef
BOOLEAN
(EFIAPI *CBD_X509_GET_SUBJECT_NAME) (
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     UINT8        *CertSubject,
  IN OUT  UINTN        *SubjectSize
  );


typedef
EFI_STATUS
(EFIAPI *CBD_X509_GET_COMMON_NAME) (
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     CHAR8        *CommonName,  OPTIONAL
  IN OUT  UINTN        *CommonNameSize
  );



typedef
EFI_STATUS
(EFIAPI *CBD_X509_GET_ORGANIZATION_NAME) (
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     CHAR8        *NameBuffer,  OPTIONAL
  IN OUT  UINTN        *NameBufferSize
  );

typedef
BOOLEAN
(EFIAPI *CBD_X509_VerifyCert) (
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  IN  CONST UINT8  *CACert,
  IN  UINTN        CACertSize
  );

typedef
BOOLEAN
(EFIAPI *CBD_X509_ConstructCertificate) (
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  UINT8        **SingleX509Cert
  );

typedef
BOOLEAN
(EFIAPI *CBD_X509_ConstructCertificateStack) (
  IN OUT  UINT8  **X509Stack,
  ...
  );

typedef
VOID
(EFIAPI *CBD_X509_Free) (
  IN  VOID  *X509Cert
  );

typedef
VOID
(EFIAPI *CBD_X509_StackFree) (
  IN  VOID  *X509Stack
  );

typedef
BOOLEAN
(EFIAPI *CBD_X509_GetTBSCert) (
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  OUT UINT8        **TBSCert,
  OUT UINTN        *TBSCertSize
  );



//=====================================================================================
//    Symmetric Cryptography Primitive
//=====================================================================================

typedef
UINTN
(EFIAPI *CBD_TDES_GetContextSize) (
  VOID
  );

typedef
BOOLEAN
(EFIAPI *CBD_TDES_Init) (
  OUT  VOID         *TdesContext,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeyLength
  );


typedef
BOOLEAN
(EFIAPI *CBD_TDES_EcbEncrypt) (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  );


typedef
BOOLEAN
(EFIAPI *CBD_TDES_EcbDecrypt) (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  );


typedef
BOOLEAN
(EFIAPI *CBD_TDES_CbcEncrypt) (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  );


typedef
BOOLEAN
(EFIAPI *CBD_TDES_CbcDecrypt) (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  );

typedef
UINTN
(EFIAPI *CBD_AES_GetContextSize) (
  VOID
  );


typedef
BOOLEAN
(EFIAPI *CBD_AES_Init) (
  OUT  VOID         *AesContext,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeyLength
  );


typedef
BOOLEAN
(EFIAPI *CBD_AES_EcbEncrypt) (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  );


typedef
BOOLEAN
(EFIAPI *CBD_AES_EcbDecrypt) (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  );


typedef
BOOLEAN
(EFIAPI *CBD_AES_CbcEncrypt) (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  );


typedef
BOOLEAN
(EFIAPI *CBD_AES_CbcDecrypt) (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  );

typedef
UINTN
(EFIAPI *CBD_ARC4_GetContextSize) (
  VOID
  );


typedef
BOOLEAN
(EFIAPI *CBD_ARC4_Init) (
  OUT  VOID         *Arc4Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  );


typedef
BOOLEAN
(EFIAPI *CBD_ARC4_Encrypt) (
  IN OUT  VOID         *Arc4Context,
  IN      CONST UINT8  *Input,
  IN      UINTN        InputSize,
  OUT     UINT8        *Output
  );


typedef
BOOLEAN
(EFIAPI *CBD_ARC4_Decrypt) (
  IN OUT  VOID   *Arc4Context,
  IN      UINT8  *Input,
  IN      UINTN  InputSize,
  OUT     UINT8  *Output
  );


typedef
BOOLEAN
(EFIAPI *CBD_ARC4_Reset) (
  IN OUT  VOID  *Arc4Context
  );

#pragma pack(push, 1)
///
struct _SHARED_CRYPTO_FUNCTIONS
{
  /// Shared Crypto Functions
  CBD_GET_VERSION SharedCrypto_GetLowestSupportedVersion;
  /// HMAC
  CBD_HMAC_MD5_GetContextSize HMAC_MD5_GetContextSize;
  CBD_HMAC_MD5_New HMAC_MD5_New;
  CBD_HMAC_MD5_Free HMAC_MD5_Free;
  CBD_HMAC_MD5_Init HMAC_MD5_Init;
  CBD_HMAC_MD5_Duplicate HMAC_MD5_Duplicate;
  CBD_HMAC_MD5_Update HMAC_MD5_Update;
  CBD_HMAC_MD5_Final HMAC_MD5_Final;
  CBD_HMAC_SHA1_GetContextSize HMAC_SHA1_GetContextSize;
  CBD_HMAC_SHA1_New HMAC_SHA1_New;
  CBD_HMAC_SHA1_Free HMAC_SHA1_Free;
  CBD_HMAC_SHA1_Init HMAC_SHA1_Init;
  CBD_HMAC_SHA1_Duplicate HMAC_SHA1_Duplicate;
  CBD_HMAC_SHA1_Update HMAC_SHA1_Update;
  CBD_HMAC_SHA1_Final HMAC_SHA1_Final;
  CBD_HMAC_SHA256_GetContextSize HMAC_SHA256_GetContextSize;
  CBD_HMAC_SHA256_New HMAC_SHA256_New;
  CBD_HMAC_SHA256_Free HMAC_SHA256_Free;
  CBD_HMAC_SHA256_Init HMAC_SHA256_Init;
  CBD_HMAC_SHA256_Duplicate HMAC_SHA256_Duplicate;
  CBD_HMAC_SHA256_Update HMAC_SHA256_Update;
  CBD_HMAC_SHA256_Final HMAC_SHA256_Final;
  /// Md4
  CBD_MD4_GetContextSize MD4_GetContextSize;
  CBD_MD4_Init MD4_Init;
  CBD_MD4_Duplicate MD4_Duplicate;
  CBD_MD4_Update MD4_Update;
  CBD_MD4_Final MD4_Final;
  CBD_MD4_HashAll MD4_HashAll;
  /// Md5
  CBD_MD5_GetContextSize MD5_GetContextSize;
  CBD_MD5_Init MD5_Init;
  CBD_MD5_Duplicate MD5_Duplicate;
  CBD_MD5_Update MD5_Update;
  CBD_MD5_Final MD5_Final;
  CBD_MD5_HashAll MD5_HashAll;
  /// Pkcs
  CBD_PKCS1_ENCRYPT_V2 PKCS1_ENCRYPT_V2;
  CBD_PKCS5_PW_HASH PKCS5_PW_HASH;
  CBD_PKCS7_VERIFY PKCS7_VERIFY;
  CBD_PKCS7_VERIFY_EKU PKCS7_VERIFY_EKU;
  CBD_PKCS7_GetSigners PKCS7_GetSigners;
  CBD_PKCS7_FreeSigners PKCS7_FreeSigners;
  CBD_PKCS7_Sign PKCS7_Sign;
  CBD_PKCS7_GetAttachedContent PKCS7_GetAttachedContent;
  CBD_PKCS7_GetCertificatesList PKCS7_GetCertificatesList;
  CBD_AuthenticodeVerify Authenticode_Verify;
  CBD_ImageTimestampVerify Image_TimestampVerify;
  /// DH
  CBD_DH_New DH_New;
  CBD_DH_Free DH_Free;
  CBD_DH_GenerateParameter DH_GenerateParameter;
  CBD_DH_SetParameter DH_SetParameter;
  CBD_DH_GenerateKey DH_GenerateKey;
  CBD_DH_ComputeKey DH_ComputeKey;
  /// Random
  CBD_RANDOM_Seed RANDOM_Seed;
  CBD_RANDOM_Bytes RANDOM_Bytes;
  /// RSA
  CBD_RSA_VERIFY_PKCS1 RSA_VERIFY_PKCS1;
  CBD_RSA_FREE RSA_FREE;
  CBD_RSA_GET_PUBLIC_KEY_FROM_X509 RSA_GET_PUBLIC_KEY_FROM_X509;
  CBD_RSA_New RSA_New;
  CBD_RSA_Free RSA_Free;
  CBD_RSA_SetKey RSA_SetKey;
  CBD_RSA_GetKey RSA_GetKey;
  CBD_RSA_GenerateKey RSA_GenerateKey;
  CBD_RSA_CheckKey RSA_CheckKey;
  CBD_RSA_Pkcs1Sign RSA_Pkcs1Sign;
  CBD_RSA_Pkcs1Verify RSA_Pkcs1Verify;
  CBD_RSA_GetPrivateKeyFromPem RSA_GetPrivateKeyFromPem;
  CBD_RSA_GetPublicKeyFromX509 RSA_GetPublicKeyFromX509;
  /// Sha protocol
  CBD_SHA1_GET_CONTEXT_SIZE SHA1_GET_CONTEXT_SIZE;
  CBD_SHA1_INIT SHA1_INIT;
  CBD_SHA1_DUPLICATE SHA1_DUPLICATE;
  CBD_SHA1_UPDATE SHA1_UPDATE;
  CBD_SHA1_FINAL SHA1_FINAL;
  CBD_SHA1_HASH_ALL SHA1_HASH_ALL;
  CBD_SHA256_GET_CONTEXT_SIZE SHA256_GET_CONTEXT_SIZE;
  CBD_SHA256_INIT SHA256_INIT;
  CBD_SHA256_DUPLICATE SHA256_DUPLICATE;
  CBD_SHA256_UPDATE SHA256_UPDATE;
  CBD_SHA256_FINAL SHA256_FINAL;
  CBD_SHA256_HASH_ALL SHA256_HASH_ALL;
  CBD_SHA384_GetContextSize SHA384_GetContextSize;
  CBD_SHA384_Init SHA384_Init;
  CBD_SHA384_Duplicate SHA384_Duplicate;
  CBD_SHA384_Update SHA384_Update;
  CBD_SHA384_Final SHA384_Final;
  CBD_SHA384_HashAll SHA384_HashAll;
  CBD_SHA512_GetContextSize SHA512_GetContextSize;
  CBD_SHA512_Init SHA512_Init;
  CBD_SHA512_Duplicate SHA512_Duplicate;
  CBD_SHA512_Update SHA512_Update;
  CBD_SHA512_Final SHA512_Final;
  CBD_SHA512_HashAll SHA512_HashAll;
  /// X509
  CBD_X509_GET_SUBJECT_NAME X509_GET_SUBJECT_NAME;
  CBD_X509_GET_COMMON_NAME X509_GET_COMMON_NAME;
  CBD_X509_GET_ORGANIZATION_NAME X509_GET_ORGANIZATION_NAME;
  CBD_X509_VerifyCert X509_VerifyCert;
  CBD_X509_ConstructCertificate X509_ConstructCertificate;
  CBD_X509_ConstructCertificateStack X509_ConstructCertificateStack;
  CBD_X509_Free X509_Free;
  CBD_X509_StackFree X509_StackFree;
  CBD_X509_GetTBSCert X509_GetTBSCert;
  /// TDES
  CBD_TDES_GetContextSize TDES_GetContextSize;
  CBD_TDES_Init TDES_Init;
  CBD_TDES_EcbEncrypt TDES_EcbEncrypt;
  CBD_TDES_EcbDecrypt TDES_EcbDecrypt;
  CBD_TDES_CbcEncrypt TDES_CbcEncrypt;
  CBD_TDES_CbcDecrypt TDES_CbcDecrypt;
  /// AES
  CBD_AES_GetContextSize AES_GetContextSize;
  CBD_AES_Init AES_Init;
  CBD_AES_EcbEncrypt AES_EcbEncrypt;
  CBD_AES_EcbDecrypt AES_EcbDecrypt;
  CBD_AES_CbcEncrypt AES_CbcEncrypt;
  CBD_AES_CbcDecrypt AES_CbcDecrypt;
  /// Arc4
  CBD_ARC4_GetContextSize ARC4_GetContextSize;
  CBD_ARC4_Init ARC4_Init;
  CBD_ARC4_Encrypt ARC4_Encrypt;
  CBD_ARC4_Decrypt ARC4_Decrypt;
  CBD_ARC4_Reset ARC4_Reset;
};
#pragma pack(pop)

#endif