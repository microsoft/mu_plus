
/** @file
  This module is consumed by both DXE and SMM as well as PEI

  This links the functions in the protocol to the functions in BaseCryptLib.
  This is the Full flavor, which means this supports all the functions that are
  in BaseCryptLib.

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
  HmacMd5GetContextSize, //SHAREDCRYPTO_HMAC_MD5_GetContextSize HMAC_MD5_GetContextSize;
  HmacMd5New, //SHAREDCRYPTO_HMAC_MD5_New HMAC_MD5_New;
  HmacMd5Free, //SHAREDCRYPTO_HMAC_MD5_Free HMAC_MD5_Free;
  HmacMd5Init, //SHAREDCRYPTO_HMAC_MD5_Init HMAC_MD5_Init;
  HmacMd5Duplicate, //SHAREDCRYPTO_HMAC_MD5_Duplicate HMAC_MD5_Duplicate;
  HmacMd5Update, //SHAREDCRYPTO_HMAC_MD5_Update HMAC_MD5_Update;
  HmacMd5Final, //SHAREDCRYPTO_HMAC_MD5_Final HMAC_MD5_Final;
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
  Md4GetContextSize, //SHAREDCRYPTO_MD4_GetContextSize MD4_GetContextSize;
  Md4Init, //SHAREDCRYPTO_MD4_Init MD4_Init;
  Md4Duplicate, //SHAREDCRYPTO_MD4_Duplicate MD4_Duplicate;
  Md4Update, //SHAREDCRYPTO_MD4_Update MD4_Update;
  Md4Final, //SHAREDCRYPTO_MD4_Final MD4_Final;
  Md4HashAll, //SHAREDCRYPTO_MD4_HashAll MD4_HashAll;
  /// Md5
  Md5GetContextSize, //SHAREDCRYPTO_MD5_GetContextSize MD5_GetContextSize;
  Md5Init, //SHAREDCRYPTO_MD5_Init MD5_Init;
  Md5Duplicate, //SHAREDCRYPTO_MD5_Duplicate MD5_Duplicate;
  Md5Update, //SHAREDCRYPTO_MD5_Update MD5_Update;
  Md5Final, //SHAREDCRYPTO_MD5_Final MD5_Final;
  Md5HashAll, //SHAREDCRYPTO_MD5_HashAll MD5_HashAll;
  /// Pkcs
  Pkcs1v2Encrypt, //SHAREDCRYPTO_PKCS1_ENCRYPT_V2 PKCS1_ENCRYPT_V2;
  Pkcs5HashPassword, //SHAREDCRYPTO_PKCS5_PW_HASH PKCS5_PW_HASH;
  Pkcs7Verify, //SHAREDCRYPTO_PKCS7_VERIFY PKCS7_VERIFY;
  VerifyEKUsInPkcs7Signature, //SHAREDCRYPTO_PKCS7_VERIFY_EKU PKCS7_VERIFY_EKU;
  Pkcs7GetSigners, //SHAREDCRYPTO_PKCS7_GetSigners PKCS7_GetSigners;
  Pkcs7FreeSigners, //SHAREDCRYPTO_PKCS7_FreeSigners PKCS7_FreeSigners;
  Pkcs7Sign, //SHAREDCRYPTO_PKCS7_Sign PKCS7_Sign;
  Pkcs7GetAttachedContent, //SHAREDCRYPTO_PKCS7_GetAttachedContent PKCS7_GetAttachedContent;
  Pkcs7GetCertificatesList, //SHAREDCRYPTO_PKCS7_GetCertificatesList PKCS7_GetCertificatesList;
  AuthenticodeVerify, //SHAREDCRYPTO_AuthenticodeVerify Authenticode_Verify;
  ImageTimestampVerify, //SHAREDCRYPTO_ImageTimestampVerify Image_TimestampVerify;
  /// DH
  DhNew, //SHAREDCRYPTO_DH_New DH_New;
  DhFree, //SHAREDCRYPTO_DH_Free DH_Free;
  DhGenerateParameter, //SHAREDCRYPTO_DH_GenerateParameter DH_GenerateParameter;
  DhSetParameter, //SHAREDCRYPTO_DH_SetParameter DH_SetParameter;
  DhGenerateKey, //SHAREDCRYPTO_DH_GenerateKey DH_GenerateKey;
  DhComputeKey, //SHAREDCRYPTO_DH_ComputeKey DH_ComputeKey;
  /// Random
  RandomSeed, //SHAREDCRYPTO_RANDOM_Seed RANDOM_Seed;
  RandomBytes, //SHAREDCRYPTO_RANDOM_Bytes RANDOM_Bytes;
  /// RSA
  RsaPkcs1Verify, //SHAREDCRYPTO_RSA_VERIFY_PKCS1 RSA_VERIFY_PKCS1;
  RsaNew, //SHAREDCRYPTO_RSA_New RSA_New;
  RsaFree, //SHAREDCRYPTO_RSA_Free RSA_Free;
  RsaSetKey, //SHAREDCRYPTO_RSA_SetKey RSA_SetKey;
  RsaGetKey, //SHAREDCRYPTO_RSA_GetKey RSA_GetKey;
  RsaGenerateKey, //SHAREDCRYPTO_RSA_GenerateKey RSA_GenerateKey;
  RsaCheckKey, //SHAREDCRYPTO_RSA_CheckKey RSA_CheckKey;
  RsaPkcs1Sign, //SHAREDCRYPTO_RSA_Pkcs1Sign RSA_Pkcs1Sign;
  RsaPkcs1Verify, //SHAREDCRYPTO_RSA_Pkcs1Verify RSA_Pkcs1Verify;
  RsaGetPrivateKeyFromPem, //SHAREDCRYPTO_RSA_GetPrivateKeyFromPem RSA_GetPrivateKeyFromPem;
  RsaGetPublicKeyFromX509, //SHAREDCRYPTO_RSA_GetPublicKeyFromX509 RSA_GetPublicKeyFromX509;
  /// Sha protocol
  Sha1GetContextSize, //SHAREDCRYPTO_SHA1_GET_CONTEXT_SIZE SHA1_GET_CONTEXT_SIZE;
  Sha1Init, //SHAREDCRYPTO_SHA1_INIT SHA1_INIT;
  Sha1Duplicate, //SHAREDCRYPTO_SHA1_DUPLICATE SHA1_DUPLICATE;
  Sha1Update, //SHAREDCRYPTO_SHA1_UPDATE SHA1_UPDATE;
  Sha1Final, //SHAREDCRYPTO_SHA1_FINAL SHA1_FINAL;
  Sha1HashAll, //SHAREDCRYPTO_SHA1_HASH_ALL SHA1_HASH_ALL;
  Sha256GetContextSize, //SHAREDCRYPTO_SHA256_GET_CONTEXT_SIZE SHA256_GET_CONTEXT_SIZE;
  Sha256Init, //SHAREDCRYPTO_SHA256_INIT SHA256_INIT;
  Sha256Duplicate, //SHAREDCRYPTO_SHA256_DUPLICATE SHA256_DUPLICATE;
  Sha256Update, //SHAREDCRYPTO_SHA256_UPDATE SHA256_UPDATE;
  Sha256Final, //SHAREDCRYPTO_SHA256_FINAL SHA256_FINAL;
  Sha256HashAll, //SHAREDCRYPTO_SHA256_HASH_ALL SHA256_HASH_ALL;
  Sha384GetContextSize, //SHAREDCRYPTO_SHA384_GetContextSize SHA384_GetContextSize;
  Sha384Init, //SHAREDCRYPTO_SHA384_Init SHA384_Init;
  Sha384Duplicate, //SHAREDCRYPTO_SHA384_Duplicate SHA384_Duplicate;
  Sha384Update, //SHAREDCRYPTO_SHA384_Update SHA384_Update;
  Sha384Final, //SHAREDCRYPTO_SHA384_Final SHA384_Final;
  Sha384HashAll, //SHAREDCRYPTO_SHA384_HashAll SHA384_HashAll;
  Sha512GetContextSize, //SHAREDCRYPTO_SHA512_GetContextSize SHA512_GetContextSize;
  Sha512Init, //SHAREDCRYPTO_SHA512_Init SHA512_Init;
  Sha512Duplicate, //SHAREDCRYPTO_SHA512_Duplicate SHA512_Duplicate;
  Sha512Update, //SHAREDCRYPTO_SHA512_Update SHA512_Update;
  Sha512Final, //SHAREDCRYPTO_SHA512_Final SHA512_Final;
  Sha512HashAll, //SHAREDCRYPTO_SHA512_HashAll SHA512_HashAll;
  /// X509
  X509GetSubjectName, //SHAREDCRYPTO_X509_GET_SUBJECT_NAME X509_GET_SUBJECT_NAME;
  X509GetCommonName, //SHAREDCRYPTO_X509_GET_COMMON_NAME X509_GET_COMMON_NAME;
  X509GetOrganizationName, //SHAREDCRYPTO_X509_GET_ORGANIZATION_NAME X509_GET_ORGANIZATION_NAME;
  X509VerifyCert, //SHAREDCRYPTO_X509_VerifyCert X509_VerifyCert;
  X509ConstructCertificate, //SHAREDCRYPTO_X509_ConstructCertificate X509_ConstructCertificate;
  X509ConstructCertificateStack, //SHAREDCRYPTO_X509_ConstructCertificateStack X509_ConstructCertificateStack;
  X509Free, //SHAREDCRYPTO_X509_Free X509_Free;
  X509StackFree, //SHAREDCRYPTO_X509_StackFree X509_StackFree;
  X509GetTBSCert, //SHAREDCRYPTO_X509_GetTBSCert X509_GetTBSCert;
  /// TDES
  TdesGetContextSize, //SHAREDCRYPTO_TDES_GetContextSize TDES_GetContextSize;
  TdesInit, //SHAREDCRYPTO_TDES_Init TDES_Init;
  TdesEcbEncrypt, //SHAREDCRYPTO_TDES_EcbEncrypt TDES_EcbEncrypt;
  TdesEcbDecrypt, //SHAREDCRYPTO_TDES_EcbDecrypt TDES_EcbDecrypt;
  TdesCbcEncrypt, //SHAREDCRYPTO_TDES_CbcEncrypt TDES_CbcEncrypt;
  TdesCbcDecrypt, //SHAREDCRYPTO_TDES_CbcDecrypt TDES_CbcDecrypt;
  /// AES
  AesGetContextSize, //SHAREDCRYPTO_AES_GetContextSize AES_GetContextSize;
  AesInit, //SHAREDCRYPTO_AES_Init AES_Init;
  AesEcbEncrypt, //SHAREDCRYPTO_AES_EcbEncrypt AES_EcbEncrypt;
  AesEcbDecrypt, //SHAREDCRYPTO_AES_EcbDecrypt AES_EcbDecrypt;
  AesCbcEncrypt, //SHAREDCRYPTO_AES_CbcEncrypt AES_CbcEncrypt;
  AesCbcDecrypt, //SHAREDCRYPTO_AES_CbcDecrypt AES_CbcDecrypt;
  /// Arc4
  Arc4GetContextSize, //SHAREDCRYPTO_ARC4_GetContextSize ARC4_GetContextSize;
  Arc4Init, //SHAREDCRYPTO_ARC4_Init ARC4_Init;
  Arc4Encrypt, //SHAREDCRYPTO_ARC4_Encrypt ARC4_Encrypt;
  Arc4Decrypt, //SHAREDCRYPTO_ARC4_Decrypt ARC4_Decrypt;
  Arc4Reset //SHAREDCRYPTO_ARC4_Reset ARC4_Reset;
};