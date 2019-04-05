
/** @file
  This module is consumed by the reduced version of PEI

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
#include <Shared/SharedCrypto.h>
#include <Library/BaseCryptLib.h>
#include "SharedCryptoVersion.h"


CONST SHARED_CRYPTO_FUNCTIONS mSharedCryptoFunctions = {
  GetCryptoVersion,
  /// HMAC
  NULL, //CBD_HMAC_MD5_GetContextSize HMAC_MD5_GetContextSize;
  NULL, //CBD_HMAC_MD5_New HMAC_MD5_New;
  NULL, //CBD_HMAC_MD5_Free HMAC_MD5_Free;
  NULL, //CBD_HMAC_MD5_Init HMAC_MD5_Init;
  NULL, //CBD_HMAC_MD5_Duplicate HMAC_MD5_Duplicate;
  NULL, //CBD_HMAC_MD5_Update HMAC_MD5_Update;
  NULL, //CBD_HMAC_MD5_Final HMAC_MD5_Final;
  NULL, //CBD_HMAC_SHA1_GetContextSize HMAC_SHA1_GetContextSize;
  NULL, //CBD_HMAC_SHA1_New HMAC_SHA1_New;
  NULL, //CBD_HMAC_SHA1_Free HMAC_SHA1_Free;
  NULL, //CBD_HMAC_SHA1_Init HMAC_SHA1_Init;
  NULL, //CBD_HMAC_SHA1_Duplicate HMAC_SHA1_Duplicate;
  NULL, //CBD_HMAC_SHA1_Update HMAC_SHA1_Update;
  NULL, //CBD_HMAC_SHA1_Final HMAC_SHA1_Final;
  NULL, //CBD_HMAC_SHA256_GetContextSize HMAC_SHA256_GetContextSize;
  NULL, //CBD_HMAC_SHA256_New HMAC_SHA256_New;
  NULL, //CBD_HMAC_SHA256_Free HMAC_SHA256_Free;
  NULL, //CBD_HMAC_SHA256_Init HMAC_SHA256_Init;
  NULL, //CBD_HMAC_SHA256_Duplicate HMAC_SHA256_Duplicate;
  NULL, //CBD_HMAC_SHA256_Update HMAC_SHA256_Update;
  NULL, //CBD_HMAC_SHA256_Final HMAC_SHA256_Final;
  /// Md4
  NULL, //CBD_MD4_GetContextSize MD4_GetContextSize;
  NULL, //CBD_MD4_Init MD4_Init;
  NULL, //CBD_MD4_Duplicate MD4_Duplicate;
  NULL, //CBD_MD4_Update MD4_Update;
  NULL, //CBD_MD4_Final MD4_Final;
  NULL, //CBD_MD4_HashAll MD4_HashAll;
  /// Md5
  NULL, //CBD_MD5_GetContextSize MD5_GetContextSize;
  NULL, //CBD_MD5_Init MD5_Init;
  NULL, //CBD_MD5_Duplicate MD5_Duplicate;
  NULL, //CBD_MD5_Update MD5_Update;
  NULL, //CBD_MD5_Final MD5_Final;
  NULL, //CBD_MD5_HashAll MD5_HashAll;
  /// Pkcs
  NULL, //CBD_PKCS1_ENCRYPT_V2 PKCS1_ENCRYPT_V2;
  NULL, //CBD_PKCS5_PW_HASH PKCS5_PW_HASH;
  NULL, //CBD_PKCS7_VERIFY PKCS7_VERIFY;
  NULL, //CBD_PKCS7_VERIFY_EKU PKCS7_VERIFY_EKU;
  NULL, //CBD_PKCS7_GetSigners PKCS7_GetSigners;
  NULL, //CBD_PKCS7_FreeSigners PKCS7_FreeSigners;
  NULL, //CBD_PKCS7_Sign PKCS7_Sign;
  NULL, //CBD_PKCS7_GetAttachedContent PKCS7_GetAttachedContent;
  NULL, //CBD_PKCS7_GetCertificatesList PKCS7_GetCertificatesList;
  NULL, //CBD_AuthenticodeVerify AuthenticodeVerify;
  NULL, //CBD_ImageTimestampVerify ImageTimestampVerify;
  /// DH
  NULL, //CBD_DH_New DH_New;
  NULL, //CBD_DH_Free DH_Free;
  NULL, //CBD_DH_GenerateParameter DH_GenerateParameter;
  NULL, //CBD_DH_SetParameter DH_SetParameter;
  NULL, //CBD_DH_GenerateKey DH_GenerateKey;
  NULL, //CBD_DH_ComputeKey DH_ComputeKey;
  /// Random
  NULL, //CBD_RANDOM_Seed RANDOM_Seed;
  NULL, //CBD_RANDOM_Bytes RANDOM_Bytes;
  /// RSA
  NULL, //CBD_RSA_VERIFY_PKCS1 RSA_VERIFY_PKCS1;
  NULL, //CBD_RSA_FREE RSA_FREE;
  NULL, //CBD_RSA_GET_PUBLIC_KEY_FROM_X509 RSA_GET_PUBLIC_KEY_FROM_X509;
  NULL, //CBD_RSA_New RSA_New;
  NULL, //CBD_RSA_Free RSA_Free;
  NULL, //CBD_RSA_SetKey RSA_SetKey;
  NULL, //CBD_RSA_GetKey RSA_GetKey;
  NULL, //CBD_RSA_GenerateKey RSA_GenerateKey;
  NULL, //CBD_RSA_CheckKey RSA_CheckKey;
  NULL, //CBD_RSA_Pkcs1Sign RSA_Pkcs1Sign;
  NULL, //CBD_RSA_Pkcs1Verify RSA_Pkcs1Verify;
  NULL, //CBD_RSA_GetPrivateKeyFromPem RSA_GetPrivateKeyFromPem;
  NULL, //CBD_RSA_GetPublicKeyFromX509 RSA_GetPublicKeyFromX509;
  /// Sha protocol
  Sha1GetContextSize, //CBD_SHA1_GET_CONTEXT_SIZE SHA1_GET_CONTEXT_SIZE;
  Sha1Init, //CBD_SHA1_INIT SHA1_INIT;
  Sha1Duplicate, //CBD_SHA1_DUPLICATE SHA1_DUPLICATE;
  Sha1Update, //CBD_SHA1_UPDATE SHA1_UPDATE;
  Sha1Final, //CBD_SHA1_FINAL SHA1_FINAL;
  NULL, //CBD_SHA1_HASH_ALL SHA1_HASH_ALL;
  Sha256GetContextSize, //CBD_SHA256_GET_CONTEXT_SIZE SHA256_GET_CONTEXT_SIZE;
  Sha256Init, //CBD_SHA256_INIT SHA256_INIT;
  Sha256Duplicate, //CBD_SHA256_DUPLICATE SHA256_DUPLICATE;
  Sha256Update, //CBD_SHA256_UPDATE SHA256_UPDATE;
  Sha256Final, //CBD_SHA256_FINAL SHA256_FINAL;
  NULL, //CBD_SHA256_HASH_ALL SHA256_HASH_ALL;
  NULL, //CBD_SHA384_GetContextSize SHA384_GetContextSize;
  NULL, //CBD_SHA384_Init SHA384_Init;
  NULL, //CBD_SHA384_Duplicate SHA384_Duplicate;
  NULL, //CBD_SHA384_Update SHA384_Update;
  NULL, //CBD_SHA384_Final SHA384_Final;
  NULL, //CBD_SHA384_HashAll SHA384_HashAll;
  NULL, //CBD_SHA512_GetContextSize SHA512_GetContextSize;
  NULL, //CBD_SHA512_Init SHA512_Init;
  NULL, //CBD_SHA512_Duplicate SHA512_Duplicate;
  NULL, //CBD_SHA512_Update SHA512_Update;
  NULL, //CBD_SHA512_Final SHA512_Final;
  NULL, //CBD_SHA512_HashAll SHA512_HashAll;
  /// X509
  NULL, //CBD_X509_GET_SUBJECT_NAME X509_GET_SUBJECT_NAME;
  NULL, //CBD_X509_GET_COMMON_NAME X509_GET_COMMON_NAME;
  NULL, //CBD_X509_GET_ORGANIZATION_NAME X509_GET_ORGANIZATION_NAME;
  NULL, //CBD_X509_VerifyCert X509_VerifyCert;
  NULL, //CBD_X509_ConstructCertificate X509_ConstructCertificate;
  NULL, //CBD_X509_ConstructCertificateStack X509_ConstructCertificateStack;
  NULL, //CBD_X509_Free X509_Free;
  NULL, //CBD_X509_StackFree X509_StackFree;
  NULL, //CBD_X509_GetTBSCert X509_GetTBSCert;
  /// TDES
  NULL, //CBD_TDES_GetContextSize TDES_GetContextSize;
  NULL, //CBD_TDES_Init TDES_Init;
  NULL, //CBD_TDES_EcbEncrypt TDES_EcbEncrypt;
  NULL, //CBD_TDES_EcbDecrypt TDES_EcbDecrypt;
  NULL, //CBD_TDES_CbcEncrypt TDES_CbcEncrypt;
  NULL, //CBD_TDES_CbcDecrypt TDES_CbcDecrypt;
  /// AES
  NULL, //CBD_AES_GetContextSize AES_GetContextSize;
  NULL, //CBD_AES_Init AES_Init;
  NULL, //CBD_AES_EcbEncrypt AES_EcbEncrypt;
  NULL, //CBD_AES_EcbDecrypt AES_EcbDecrypt;
  NULL, //CBD_AES_CbcEncrypt AES_CbcEncrypt;
  NULL, //CBD_AES_CbcDecrypt AES_CbcDecrypt;
  /// Arc4
  NULL, //CBD_ARC4_GetContextSize ARC4_GetContextSize;
  NULL, //CBD_ARC4_Init ARC4_Init;
  NULL, //CBD_ARC4_Encrypt ARC4_Encrypt;
  NULL, //CBD_ARC4_Decrypt ARC4_Decrypt;
  NULL //CBD_ARC4_Reset ARC4_Reset;

};