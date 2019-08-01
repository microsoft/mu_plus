/** @file
  This module is designed to be just SHA and compile to a small EFI.

  Currently it is consumed by the reduced version of PEI (to reduce the size of
  the PEI FV since it isn't compressed on many platforms.) Other phases (DXE,SMM)
  can utilize this flavor if SHA is all they need.

  This links the functions in the protocol to the functions in BaseCryptLib.
  This is the ShaOnly flavor, which means we only support SHA1 and SHA256. This
  is used by PEI on Microsoft Surface platforms as SHA is the only crypto
  functions that we use in PEI. Using the ShaOnly flavor reduces in a smaller
  binary size. See Readme.md in the root of SharedCryptoPkg for more info.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
  NULL, //SHAREDCRYPTO_HMAC_SHA1_GetContextSize HMAC_SHA1_GetContextSize;
  NULL, //SHAREDCRYPTO_HMAC_SHA1_New HMAC_SHA1_New;
  NULL, //SHAREDCRYPTO_HMAC_SHA1_Free HMAC_SHA1_Free;
  NULL, //SHAREDCRYPTO_HMAC_SHA1_Init HMAC_SHA1_Init;
  NULL, //SHAREDCRYPTO_HMAC_SHA1_Duplicate HMAC_SHA1_Duplicate;
  NULL, //SHAREDCRYPTO_HMAC_SHA1_Update HMAC_SHA1_Update;
  NULL, //SHAREDCRYPTO_HMAC_SHA1_Final HMAC_SHA1_Final;
  NULL, //SHAREDCRYPTO_HMAC_SHA256_GetContextSize HMAC_SHA256_GetContextSize;
  NULL, //SHAREDCRYPTO_HMAC_SHA256_New HMAC_SHA256_New;
  NULL, //SHAREDCRYPTO_HMAC_SHA256_Free HMAC_SHA256_Free;
  NULL, //SHAREDCRYPTO_HMAC_SHA256_Init HMAC_SHA256_Init;
  NULL, //SHAREDCRYPTO_HMAC_SHA256_Duplicate HMAC_SHA256_Duplicate;
  NULL, //SHAREDCRYPTO_HMAC_SHA256_Update HMAC_SHA256_Update;
  NULL, //SHAREDCRYPTO_HMAC_SHA256_Final HMAC_SHA256_Final;
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
  NULL, //SHAREDCRYPTO_PKCS1_ENCRYPT_V2 PKCS1_ENCRYPT_V2;
  NULL, //SHAREDCRYPTO_PKCS5_PW_HASH PKCS5_PW_HASH;
  NULL, //SHAREDCRYPTO_PKCS7_VERIFY PKCS7_VERIFY;
  NULL, //SHAREDCRYPTO_PKCS7_VERIFY_EKU PKCS7_VERIFY_EKU;
  NULL, //SHAREDCRYPTO_PKCS7_GetSigners PKCS7_GetSigners;
  NULL, //SHAREDCRYPTO_PKCS7_FreeSigners PKCS7_FreeSigners;
  NULL, //SHAREDCRYPTO_PKCS7_Sign PKCS7_Sign;
  NULL, //SHAREDCRYPTO_PKCS7_GetAttachedContent PKCS7_GetAttachedContent;
  NULL, //SHAREDCRYPTO_PKCS7_GetCertificatesList PKCS7_GetCertificatesList;
  NULL, //SHAREDCRYPTO_AuthenticodeVerify AuthenticodeVerify;
  NULL, //SHAREDCRYPTO_ImageTimestampVerify ImageTimestampVerify;
  /// DH
  NULL, //SHAREDCRYPTO_DH_New DH_New;
  NULL, //SHAREDCRYPTO_DH_Free DH_Free;
  NULL, //SHAREDCRYPTO_DH_GenerateParameter DH_GenerateParameter;
  NULL, //SHAREDCRYPTO_DH_SetParameter DH_SetParameter;
  NULL, //SHAREDCRYPTO_DH_GenerateKey DH_GenerateKey;
  NULL, //SHAREDCRYPTO_DH_ComputeKey DH_ComputeKey;
  /// Random
  NULL, //SHAREDCRYPTO_RANDOM_Seed RANDOM_Seed;
  NULL, //SHAREDCRYPTO_RANDOM_Bytes RANDOM_Bytes;
  /// RSA
  NULL, //SHAREDCRYPTO_RSA_VERIFY_PKCS1 RSA_VERIFY_PKCS1;
  NULL, //SHAREDCRYPTO_RSA_New RSA_New;
  NULL, //SHAREDCRYPTO_RSA_Free RSA_Free;
  NULL, //SHAREDCRYPTO_RSA_SetKey RSA_SetKey;
  NULL, //SHAREDCRYPTO_RSA_GetKey RSA_GetKey;
  NULL, //SHAREDCRYPTO_RSA_GenerateKey RSA_GenerateKey;
  NULL, //SHAREDCRYPTO_RSA_CheckKey RSA_CheckKey;
  NULL, //SHAREDCRYPTO_RSA_Pkcs1Sign RSA_Pkcs1Sign;
  NULL, //SHAREDCRYPTO_RSA_Pkcs1Verify RSA_Pkcs1Verify;
  NULL, //SHAREDCRYPTO_RSA_GetPrivateKeyFromPem RSA_GetPrivateKeyFromPem;
  NULL, //SHAREDCRYPTO_RSA_GetPublicKeyFromX509 RSA_GetPublicKeyFromX509;
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
  NULL, //SHAREDCRYPTO_X509_GET_SUBJECT_NAME X509_GET_SUBJECT_NAME;
  NULL, //SHAREDCRYPTO_X509_GET_COMMON_NAME X509_GET_COMMON_NAME;
  NULL, //SHAREDCRYPTO_X509_GET_ORGANIZATION_NAME X509_GET_ORGANIZATION_NAME;
  NULL, //SHAREDCRYPTO_X509_VerifyCert X509_VerifyCert;
  NULL, //SHAREDCRYPTO_X509_ConstructCertificate X509_ConstructCertificate;
  NULL, //SHAREDCRYPTO_X509_ConstructCertificateStack X509_ConstructCertificateStack;
  NULL, //SHAREDCRYPTO_X509_Free X509_Free;
  NULL, //SHAREDCRYPTO_X509_StackFree X509_StackFree;
  NULL, //SHAREDCRYPTO_X509_GetTBSCert X509_GetTBSCert;
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