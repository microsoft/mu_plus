/** @file
 *
 *
 *
  This module is consumed by both DXE and SMM as well as PEI

  This links the functions in the protocol to the functions in BaseCryptLib.
  This is the Mu flavor, which means we only support the functions that are used
  by Microsoft Surface platforms and what we believe is a sensible set of Crypto
  functions to support.

  See Readme.md in the root of SharedCryptoPkg for more info.

  Copyright (c) 2019, Microsoft Corporation. All rights reserved.

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
#include <Protocol/SharedCrypto.h>
#include <Library/BaseCryptLib.h>
#include "SharedCryptoVersion.h"

CONST SHARED_CRYPTO_FUNCTIONS mSharedCryptoFunctions = {
  GetCryptoVersion,
  /// HMAC
  NULL, //SHAREDCRYPTO_HMAC_MD5_GetContextSize HMAC_MD5_GetContextSize;
  NULL, //SHAREDCRYPTO_HMAC_MD5_New HMAC_MD5_New;
  NULL, //SHAREDCRYPTO_HMAC_MD5_Free HMAC_MD5_Free;
  NULL, //SHAREDCRYPTO_HMAC_MD5_Init HMAC_MD5_Init;
  NULL, //SHAREDCRYPTO_HMAC_MD5_Duplicate HMAC_MD5_Duplicate;
  NULL, //SHAREDCRYPTO_HMAC_MD5_Update HMAC_MD5_Update;
  NULL, //SHAREDCRYPTO_HMAC_MD5_Final HMAC_MD5_Final;
  HmacSha1GetContextSize, //SHAREDCRYPTO_HMAC_SHA1_GetContextSize HMAC_SHA1_GetContextSize;
  HmacSha1New, //SHAREDCRYPTO_HMAC_SHA1_New HMAC_SHA1_New;
  HmacSha1Free, //SHAREDCRYPTO_HMAC_SHA1_Free HMAC_SHA1_Free;
  HmacSha1Init, //SHAREDCRYPTO_HMAC_SHA1_Init HMAC_SHA1_Init;
  HmacSha1Duplicate, //SHAREDCRYPTO_HMAC_SHA1_Duplicate HMAC_SHA1_Duplicate;
  HmacSha1Update, //SHAREDCRYPTO_HMAC_SHA1_Update HMAC_SHA1_Update;
  HmacSha1Final, //SHAREDCRYPTO_HMAC_SHA1_Final HMAC_SHA1_Final;
  HmacSha256GetContextSize, //SHAREDCRYPTO_HMAC_SHA256_GetContextSize HMAC_SHA256_GetContextSize;
  HmacSha256New, //SHAREDCRYPTO_HMAC_SHA256_New HMAC_SHA256_New;
  HmacSha256Free, //SHAREDCRYPTO_HMAC_SHA256_Free HMAC_SHA256_Free;
  HmacSha256Init, //SHAREDCRYPTO_HMAC_SHA256_Init HMAC_SHA256_Init;
  HmacSha256Duplicate, //SHAREDCRYPTO_HMAC_SHA256_Duplicate HMAC_SHA256_Duplicate;
  HmacSha256Update, //SHAREDCRYPTO_HMAC_SHA256_Update HMAC_SHA256_Update;
  HmacSha256Final, //SHAREDCRYPTO_HMAC_SHA256_Final HMAC_SHA256_Final;
  /// Md4
  NULL, //SHAREDCRYPTO_MD4_GetContextSize MD4_GetContextSize;
  NULL, //SHAREDCRYPTO_MD4_Init MD4_Init;
  NULL, //SHAREDCRYPTO_MD4_Duplicate MD4_Duplicate;
  NULL, //SHAREDCRYPTO_MD4_Update MD4_Update;
  NULL, //SHAREDCRYPTO_MD4_Final MD4_Final;
  NULL, //SHAREDCRYPTO_MD4_HashAll MD4_HashAll;
  /// Md5
  NULL, //SHAREDCRYPTO_MD5_GetContextSize MD5_GetContextSize;
  NULL, //SHAREDCRYPTO_MD5_Init MD5_Init;
  NULL, //SHAREDCRYPTO_MD5_Duplicate MD5_Duplicate;
  NULL, //SHAREDCRYPTO_MD5_Update MD5_Update;
  NULL, //SHAREDCRYPTO_MD5_Final MD5_Final;
  NULL, //SHAREDCRYPTO_MD5_HashAll MD5_HashAll;
  /// Pkcs
  Pkcs1v2Encrypt, //SHAREDCRYPTO_PKCS1_ENCRYPT_V2 PKCS1_ENCRYPT_V2;
  Pkcs5HashPassword, //SHAREDCRYPTO_PKCS5_PW_HASH PKCS5_PW_HASH;
  Pkcs7Verify, //SHAREDCRYPTO_PKCS7_VERIFY PKCS7_VERIFY;
  VerifyEKUsInPkcs7Signature, //SHAREDCRYPTO_PKCS7_VERIFY_EKU PKCS7_VERIFY_EKU;
  Pkcs7GetSigners, //SHAREDCRYPTO_PKCS7_GetSigners PKCS7_GetSigners;
  Pkcs7FreeSigners, //SHAREDCRYPTO_PKCS7_FreeSigners PKCS7_FreeSigners;
  NULL, //SHAREDCRYPTO_PKCS7_Sign PKCS7_Sign;
  NULL, //SHAREDCRYPTO_PKCS7_GetAttachedContent PKCS7_GetAttachedContent;
  NULL, //SHAREDCRYPTO_PKCS7_GetCertificatesList PKCS7_GetCertificatesList;
  AuthenticodeVerify, //SHAREDCRYPTO_AuthenticodeVerify AuthenticodeVerify;
  NULL, //SHAREDCRYPTO_ImageTimestampVerify ImageTimestampVerify;
  /// DH
  NULL, //SHAREDCRYPTO_DH_New DH_New;
  NULL, //SHAREDCRYPTO_DH_Free DH_Free;
  NULL, //SHAREDCRYPTO_DH_GenerateParameter DH_GenerateParameter;
  NULL, //SHAREDCRYPTO_DH_SetParameter DH_SetParameter;
  NULL, //SHAREDCRYPTO_DH_GenerateKey DH_GenerateKey;
  NULL, //SHAREDCRYPTO_DH_ComputeKey DH_ComputeKey;
  /// Random
  RandomSeed, //SHAREDCRYPTO_RANDOM_Seed RANDOM_Seed;
  RandomBytes, //SHAREDCRYPTO_RANDOM_Bytes RANDOM_Bytes;
  /// RSA
  RsaPkcs1Verify, //SHAREDCRYPTO_RSA_VERIFY_PKCS1 RSA_VERIFY_PKCS1;
  RsaNew, //SHAREDCRYPTO_RSA_New RSA_New;
  RsaFree, //SHAREDCRYPTO_RSA_Free RSA_Free;
  NULL, //SHAREDCRYPTO_RSA_SetKey RSA_SetKey;
  NULL, //SHAREDCRYPTO_RSA_GetKey RSA_GetKey;
  NULL, //SHAREDCRYPTO_RSA_GenerateKey RSA_GenerateKey;
  NULL, //SHAREDCRYPTO_RSA_CheckKey RSA_CheckKey;
  NULL, //SHAREDCRYPTO_RSA_Pkcs1Sign RSA_Pkcs1Sign;
  NULL, //SHAREDCRYPTO_RSA_Pkcs1Verify RSA_Pkcs1Verify;
  NULL, //SHAREDCRYPTO_RSA_GetPrivateKeyFromPem RSA_GetPrivateKeyFromPem;
  RsaGetPublicKeyFromX509, //SHAREDCRYPTO_RSA_GetPublicKeyFromX509 RSA_GetPublicKeyFromX509;
  /// Sha protocol
  Sha1GetContextSize, //SHAREDCRYPTO_SHA1_GET_CONTEXT_SIZE SHA1_GET_CONTEXT_SIZE;
  Sha1Init, //SHAREDCRYPTO_SHA1_INIT SHA1_INIT;
  Sha1Duplicate, //SHAREDCRYPTO_SHA1_DUPLICATE SHA1_DUPLICATE;
  Sha1Update, //SHAREDCRYPTO_SHA1_UPDATE SHA1_UPDATE;
  Sha1Final, //SHAREDCRYPTO_SHA1_FINAL SHA1_FINAL;
  NULL, //SHAREDCRYPTO_SHA1_HASH_ALL SHA1_HASH_ALL;
  Sha256GetContextSize, //SHAREDCRYPTO_SHA256_GET_CONTEXT_SIZE SHA256_GET_CONTEXT_SIZE;
  Sha256Init, //SHAREDCRYPTO_SHA256_INIT SHA256_INIT;
  Sha256Duplicate, //SHAREDCRYPTO_SHA256_DUPLICATE SHA256_DUPLICATE;
  Sha256Update, //SHAREDCRYPTO_SHA256_UPDATE SHA256_UPDATE;
  Sha256Final, //SHAREDCRYPTO_SHA256_FINAL SHA256_FINAL;
  NULL, //SHAREDCRYPTO_SHA256_HASH_ALL SHA256_HASH_ALL;
  NULL, //SHAREDCRYPTO_SHA384_GetContextSize SHA384_GetContextSize;
  NULL, //SHAREDCRYPTO_SHA384_Init SHA384_Init;
  NULL, //SHAREDCRYPTO_SHA384_Duplicate SHA384_Duplicate;
  NULL, //SHAREDCRYPTO_SHA384_Update SHA384_Update;
  NULL, //SHAREDCRYPTO_SHA384_Final SHA384_Final;
  NULL, //SHAREDCRYPTO_SHA384_HashAll SHA384_HashAll;
  NULL, //SHAREDCRYPTO_SHA512_GetContextSize SHA512_GetContextSize;
  NULL, //SHAREDCRYPTO_SHA512_Init SHA512_Init;
  NULL, //SHAREDCRYPTO_SHA512_Duplicate SHA512_Duplicate;
  NULL, //SHAREDCRYPTO_SHA512_Update SHA512_Update;
  NULL, //SHAREDCRYPTO_SHA512_Final SHA512_Final;
  NULL, //SHAREDCRYPTO_SHA512_HashAll SHA512_HashAll;
  /// X509
  X509GetSubjectName, //SHAREDCRYPTO_X509_GET_SUBJECT_NAME X509_GET_SUBJECT_NAME;
  X509GetCommonName, //SHAREDCRYPTO_X509_GET_COMMON_NAME X509_GET_COMMON_NAME;
  X509GetOrganizationName, //SHAREDCRYPTO_X509_GET_ORGANIZATION_NAME X509_GET_ORGANIZATION_NAME;
  NULL, //SHAREDCRYPTO_X509_VerifyCert X509_VerifyCert;
  NULL, //SHAREDCRYPTO_X509_ConstructCertificate X509_ConstructCertificate;
  NULL, //SHAREDCRYPTO_X509_ConstructCertificateStack X509_ConstructCertificateStack;
  NULL, //SHAREDCRYPTO_X509_Free X509_Free;
  NULL, //SHAREDCRYPTO_X509_StackFree X509_StackFree;
  X509GetTBSCert, //SHAREDCRYPTO_X509_GetTBSCert X509_GetTBSCert;
  /// TDES
  NULL, //SHAREDCRYPTO_TDES_GetContextSize TDES_GetContextSize;
  NULL, //SHAREDCRYPTO_TDES_Init TDES_Init;
  NULL, //SHAREDCRYPTO_TDES_EcbEncrypt TDES_EcbEncrypt;
  NULL, //SHAREDCRYPTO_TDES_EcbDecrypt TDES_EcbDecrypt;
  NULL, //SHAREDCRYPTO_TDES_CbcEncrypt TDES_CbcEncrypt;
  NULL, //SHAREDCRYPTO_TDES_CbcDecrypt TDES_CbcDecrypt;
  /// AES
  NULL, //SHAREDCRYPTO_AES_GetContextSize AES_GetContextSize;
  NULL, //SHAREDCRYPTO_AES_Init AES_Init;
  NULL, //SHAREDCRYPTO_AES_EcbEncrypt AES_EcbEncrypt;
  NULL, //SHAREDCRYPTO_AES_EcbDecrypt AES_EcbDecrypt;
  NULL, //SHAREDCRYPTO_AES_CbcEncrypt AES_CbcEncrypt;
  NULL, //SHAREDCRYPTO_AES_CbcDecrypt AES_CbcDecrypt;
  /// Arc4
  NULL, //SHAREDCRYPTO_ARC4_GetContextSize ARC4_GetContextSize;
  NULL, //SHAREDCRYPTO_ARC4_Init ARC4_Init;
  NULL, //SHAREDCRYPTO_ARC4_Encrypt ARC4_Encrypt;
  NULL, //SHAREDCRYPTO_ARC4_Decrypt ARC4_Decrypt;
  NULL //SHAREDCRYPTO_ARC4_Reset ARC4_Reset;
};