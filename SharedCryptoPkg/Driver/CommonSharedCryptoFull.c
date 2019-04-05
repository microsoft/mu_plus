
/** @file
  This module is consumed by both DXE and SMM as well as PEI

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
  HmacMd5GetContextSize, //CBD_HMAC_MD5_GetContextSize HMAC_MD5_GetContextSize;
  HmacMd5New, //CBD_HMAC_MD5_New HMAC_MD5_New;
  HmacMd5Free, //CBD_HMAC_MD5_Free HMAC_MD5_Free;
  HmacMd5Init, //CBD_HMAC_MD5_Init HMAC_MD5_Init;
  HmacMd5Duplicate, //CBD_HMAC_MD5_Duplicate HMAC_MD5_Duplicate;
  HmacMd5Update, //CBD_HMAC_MD5_Update HMAC_MD5_Update;
  HmacMd5Final, //CBD_HMAC_MD5_Final HMAC_MD5_Final;
  HmacSha1GetContextSize, //CBD_HMAC_SHA1_GetContextSize HMAC_SHA1_GetContextSize;
  HmacSha1New, //CBD_HMAC_SHA1_New HMAC_SHA1_New;
  HmacSha1Free, //CBD_HMAC_SHA1_Free HMAC_SHA1_Free;
  HmacSha1Init, //CBD_HMAC_SHA1_Init HMAC_SHA1_Init;
  HmacSha1Duplicate, //CBD_HMAC_SHA1_Duplicate HMAC_SHA1_Duplicate;
  HmacSha1Update, //CBD_HMAC_SHA1_Update HMAC_SHA1_Update;
  HmacSha1Final, //CBD_HMAC_SHA1_Final HMAC_SHA1_Final;
  HmacSha256GetContextSize, //CBD_HMAC_SHA256_GetContextSize HMAC_SHA256_GetContextSize;
  HmacSha256New, //CBD_HMAC_SHA256_New HMAC_SHA256_New;
  HmacSha256Free, //CBD_HMAC_SHA256_Free HMAC_SHA256_Free;
  HmacSha256Init, //CBD_HMAC_SHA256_Init HMAC_SHA256_Init;
  HmacSha256Duplicate, //CBD_HMAC_SHA256_Duplicate HMAC_SHA256_Duplicate;
  HmacSha256Update, //CBD_HMAC_SHA256_Update HMAC_SHA256_Update;
  HmacSha256Final, //CBD_HMAC_SHA256_Final HMAC_SHA256_Final;
  /// Md4
  Md4GetContextSize, //CBD_MD4_GetContextSize MD4_GetContextSize;
  Md4Init, //CBD_MD4_Init MD4_Init;
  Md4Duplicate, //CBD_MD4_Duplicate MD4_Duplicate;
  Md4Update, //CBD_MD4_Update MD4_Update;
  Md4Final, //CBD_MD4_Final MD4_Final;
  Md4HashAll, //CBD_MD4_HashAll MD4_HashAll;
  /// Md5
  Md5GetContextSize, //CBD_MD5_GetContextSize MD5_GetContextSize;
  Md5Init, //CBD_MD5_Init MD5_Init;
  Md5Duplicate, //CBD_MD5_Duplicate MD5_Duplicate;
  Md5Update, //CBD_MD5_Update MD5_Update;
  Md5Final, //CBD_MD5_Final MD5_Final;
  Md5HashAll, //CBD_MD5_HashAll MD5_HashAll;
  /// Pkcs
  Pkcs1v2Encrypt, //CBD_PKCS1_ENCRYPT_V2 PKCS1_ENCRYPT_V2;
  Pkcs5HashPassword, //CBD_PKCS5_PW_HASH PKCS5_PW_HASH;
  Pkcs7Verify, //CBD_PKCS7_VERIFY PKCS7_VERIFY;
  VerifyEKUsInPkcs7Signature, //CBD_PKCS7_VERIFY_EKU PKCS7_VERIFY_EKU;
  Pkcs7GetSigners, //CBD_PKCS7_GetSigners PKCS7_GetSigners;
  Pkcs7FreeSigners, //CBD_PKCS7_FreeSigners PKCS7_FreeSigners;
  Pkcs7Sign, //CBD_PKCS7_Sign PKCS7_Sign;
  Pkcs7GetAttachedContent, //CBD_PKCS7_GetAttachedContent PKCS7_GetAttachedContent;
  Pkcs7GetCertificatesList, //CBD_PKCS7_GetCertificatesList PKCS7_GetCertificatesList;
  AuthenticodeVerify, //CBD_AuthenticodeVerify Authenticode_Verify;
  ImageTimestampVerify, //CBD_ImageTimestampVerify Image_TimestampVerify;
  /// DH
  DhNew, //CBD_DH_New DH_New;
  DhFree, //CBD_DH_Free DH_Free;
  DhGenerateParameter, //CBD_DH_GenerateParameter DH_GenerateParameter;
  DhSetParameter, //CBD_DH_SetParameter DH_SetParameter;
  DhGenerateKey, //CBD_DH_GenerateKey DH_GenerateKey;
  DhComputeKey, //CBD_DH_ComputeKey DH_ComputeKey;
  /// Random
  RandomSeed, //CBD_RANDOM_Seed RANDOM_Seed;
  RandomBytes, //CBD_RANDOM_Bytes RANDOM_Bytes;
  /// RSA
  RsaPkcs1Verify, //CBD_RSA_VERIFY_PKCS1 RSA_VERIFY_PKCS1;
  RsaFree, //CBD_RSA_FREE RSA_FREE;
  RsaGetPublicKeyFromX509, //CBD_RSA_GET_PUBLIC_KEY_FROM_X509 RSA_GET_PUBLIC_KEY_FROM_X509;
  RsaNew, //CBD_RSA_New RSA_New;
  RsaFree, //CBD_RSA_Free RSA_Free;
  RsaSetKey, //CBD_RSA_SetKey RSA_SetKey;
  RsaGetKey, //CBD_RSA_GetKey RSA_GetKey;
  RsaGenerateKey, //CBD_RSA_GenerateKey RSA_GenerateKey;
  RsaCheckKey, //CBD_RSA_CheckKey RSA_CheckKey;
  RsaPkcs1Sign, //CBD_RSA_Pkcs1Sign RSA_Pkcs1Sign;
  RsaPkcs1Verify, //CBD_RSA_Pkcs1Verify RSA_Pkcs1Verify;
  RsaGetPrivateKeyFromPem, //CBD_RSA_GetPrivateKeyFromPem RSA_GetPrivateKeyFromPem;
  RsaGetPublicKeyFromX509, //CBD_RSA_GetPublicKeyFromX509 RSA_GetPublicKeyFromX509;
  /// Sha protocol
  Sha1GetContextSize, //CBD_SHA1_GET_CONTEXT_SIZE SHA1_GET_CONTEXT_SIZE;
  Sha1Init, //CBD_SHA1_INIT SHA1_INIT;
  Sha1Duplicate, //CBD_SHA1_DUPLICATE SHA1_DUPLICATE;
  Sha1Update, //CBD_SHA1_UPDATE SHA1_UPDATE;
  Sha1Final, //CBD_SHA1_FINAL SHA1_FINAL;
  Sha1HashAll, //CBD_SHA1_HASH_ALL SHA1_HASH_ALL;
  Sha256GetContextSize, //CBD_SHA256_GET_CONTEXT_SIZE SHA256_GET_CONTEXT_SIZE;
  Sha256Init, //CBD_SHA256_INIT SHA256_INIT;
  Sha256Duplicate, //CBD_SHA256_DUPLICATE SHA256_DUPLICATE;
  Sha256Update, //CBD_SHA256_UPDATE SHA256_UPDATE;
  Sha256Final, //CBD_SHA256_FINAL SHA256_FINAL;
  Sha256HashAll, //CBD_SHA256_HASH_ALL SHA256_HASH_ALL;
  Sha384GetContextSize, //CBD_SHA384_GetContextSize SHA384_GetContextSize;
  Sha384Init, //CBD_SHA384_Init SHA384_Init;
  Sha384Duplicate, //CBD_SHA384_Duplicate SHA384_Duplicate;
  Sha384Update, //CBD_SHA384_Update SHA384_Update;
  Sha384Final, //CBD_SHA384_Final SHA384_Final;
  Sha384HashAll, //CBD_SHA384_HashAll SHA384_HashAll;
  Sha512GetContextSize, //CBD_SHA512_GetContextSize SHA512_GetContextSize;
  Sha512Init, //CBD_SHA512_Init SHA512_Init;
  Sha512Duplicate, //CBD_SHA512_Duplicate SHA512_Duplicate;
  Sha512Update, //CBD_SHA512_Update SHA512_Update;
  Sha512Final, //CBD_SHA512_Final SHA512_Final;
  Sha512HashAll, //CBD_SHA512_HashAll SHA512_HashAll;
  /// X509
  X509GetSubjectName, //CBD_X509_GET_SUBJECT_NAME X509_GET_SUBJECT_NAME;
  X509GetCommonName, //CBD_X509_GET_COMMON_NAME X509_GET_COMMON_NAME;
  X509GetOrganizationName, //CBD_X509_GET_ORGANIZATION_NAME X509_GET_ORGANIZATION_NAME;
  X509VerifyCert, //CBD_X509_VerifyCert X509_VerifyCert;
  X509ConstructCertificate, //CBD_X509_ConstructCertificate X509_ConstructCertificate;
  X509ConstructCertificateStack, //CBD_X509_ConstructCertificateStack X509_ConstructCertificateStack;
  X509Free, //CBD_X509_Free X509_Free;
  X509StackFree, //CBD_X509_StackFree X509_StackFree;
  X509GetTBSCert, //CBD_X509_GetTBSCert X509_GetTBSCert;
  /// TDES
  TdesGetContextSize, //CBD_TDES_GetContextSize TDES_GetContextSize;
  TdesInit, //CBD_TDES_Init TDES_Init;
  TdesEcbEncrypt, //CBD_TDES_EcbEncrypt TDES_EcbEncrypt;
  TdesEcbDecrypt, //CBD_TDES_EcbDecrypt TDES_EcbDecrypt;
  TdesCbcEncrypt, //CBD_TDES_CbcEncrypt TDES_CbcEncrypt;
  TdesCbcDecrypt, //CBD_TDES_CbcDecrypt TDES_CbcDecrypt;
  /// AES
  AesGetContextSize, //CBD_AES_GetContextSize AES_GetContextSize;
  AesInit, //CBD_AES_Init AES_Init;
  AesEcbEncrypt, //CBD_AES_EcbEncrypt AES_EcbEncrypt;
  AesEcbDecrypt, //CBD_AES_EcbDecrypt AES_EcbDecrypt;
  AesCbcEncrypt, //CBD_AES_CbcEncrypt AES_CbcEncrypt;
  AesCbcDecrypt, //CBD_AES_CbcDecrypt AES_CbcDecrypt;
  /// Arc4
  Arc4GetContextSize, //CBD_ARC4_GetContextSize ARC4_GetContextSize;
  Arc4Init, //CBD_ARC4_Init ARC4_Init;
  Arc4Encrypt, //CBD_ARC4_Encrypt ARC4_Encrypt;
  Arc4Decrypt, //CBD_ARC4_Decrypt ARC4_Decrypt;
  Arc4Reset //CBD_ARC4_Reset ARC4_Reset;
};