# Crypto Bin Package

## Copyright


## Version 20190329.0.4

Copyright (c) 2019, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions are
met:
1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
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
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

## About

The purpose of this package is to provide a wrapper around BaseCryptLib without having to compile the EFI. In theory, linking against BaseCryptLib should give you a higher rate of compression since it will inline code, but in practice the linker gets clever and messes things up.

It is comprised of a Library Instance that is called SharedCryptoLib. It is contains many of the functions included by BaseCryptLib as well as a few more (mainly X509 related stuff). The library simply locates the protocol and then calls the protocol version of BaseCryptLib (SharedCryptoProtocol). It should be a drop in replacement for the BaseCryptLib. Alternatively, you call the protocol directly.

The protocol is installed by a prebuilt EFI that gets downloaded via nuget. The EFI is packaged for consumption via an INF that is loaded and installed.

![Diagram showing dependencies](SharedCryptoPkg.png "Diagram")


## Building SharedCryptoPkg

There are two pieces to be build: the library and the driver. The library is to be included in your project

There are 24 different versions of the prebuild driver. One for each arch (X86, AARCH64, ARM, X64) and for each phase (DXE, PEI, SMM) and for mode (DEBUG, RELEASE). The nuget system should take care of downloading the correct version for your project.

## Supported Functions

+ Pkcs
  + PKCS1_ENCRYPT_V2  _Encrypts a blob using PKCS1v2 (RSAES-OAEP) schema. On success, will return the encrypted message in a newly allocated buffer._
  + PKCS5_PW_HASH _Hashes a password using Pkcs5_
  + PKCS7_VERIFY_EKU  _Receives a PKCS7 formatted signature, and then verifies that the specified EKU's are present in the end-entity leaf signing certificate_
  + PKCS7_VERIFY  _Verifies the validility of a PKCS7 signed data. The data can be wrappedin a ContentInfo structure._
+ RSA
  + RSA_VERIFY_PKCS1  _Verifies the RSA-SSA signature with EMSA-PKCS1-v1_5 encoding scheme defined in RSA PKCS1_
  + RSA_FREE  _Release the specified RSA context_
  + RSA_GET_PUBLIC_KEY_FROM_X509  _Retrieve the RSA Public Key from one DER-encoded X509 certificate._
+ SHA
  + SHA1_GET_CONTEXT_SIZE  _Retrieves the size, in bytes, of the context buffer required for SHA-1 hash operations._
  + SHA1_INIT  _Initializes user-supplied memory pointed by Sha1Context as SHA-1 hash context for subsequent use._
  + SHA1_DUPLICATE  _Makes a copy of an existing SHA1 context_
  + SHA1_UPDATE  _Digests input and updates SHA content_
  + SHA1_FINAL  _Completes computation of SHA1 digest values_
  + SHA1_HASH_ALL  _Computes the SHA1 message digest of an data buffer_
  + SHA256_GET_CONTEXT_SIZE  _Retrieves the size, in bytes, of the context buffer required for SHA-256 hash operation_
  + SHA256_INIT  _Initializes user-supplied memory pointed by Sha1Context as SHA-256 hash context for subsequent use._
  + SHA256_DUPLICATE  _Makes a copy of an existing SHA256 context_
  + SHA256_UPDATE  _Digests input and updates SHA content_
  + SHA256_FINAL  _Completes computation of SHA256 digest values_
  + SHA256_HASH_ALL  _Computes the SHA256 message digest of an data buffer_
+ X509
  + X509_GET_SUBJECT_NAME  _Retrive the subject btyes from one X.509 cert_
  + X509_GET_COMMON_NAME  _Retrieve the common name (CN) string from one X.509 certificate._
  + X509_GET_ORGANIZATION_NAME  _Retrieve the organization name (O) string from one X.509 certificate._
+ MD5
  +
+ Hmac
  + HMAC_SHA256_GetContextSize
  + HMAC_SHA256_New
  + HMAC_SHA256_Free
  + HMAC_SHA256_Init
  + HMAC_SHA256_Duplicate
  + HMAC_SHA256_Update
  + HMAC_SHA256_Final
  + HMAC_SHA1_GetContextSize
  + HMAC_SHA1_New
  + HMAC_SHA1_Free
  + HMAC_SHA1_Init
  + HMAC_SHA1_Duplicate
  + HMAC_SHA1_Update
  + HMAC_SHA1_Final
+ Random
  + RANDOM_Bytes  _Generates a pseudorandom byte stream of the specified size_
  + RANDOM_Seed  _Sets up the seed value for the pseudorandom number generator._

## Supported Architectures
This currently supports x86, x64, AARCH64, and ARM.

# Including in your platform

There are two ways to include this in your project: getting the protocol directly or using SharedCryptoLib. SharedCryptoLib is a wrapper that is a super set of the API for BaseCryptLib so you can just drop it in.



## Sample DSC change

This would replace where you would normally include BaseCryptLib.

```
[LibraryClasses.X64]
    BaseCryptLib|SharedCryptoPkg/Library/SharedCryptoLib/SharedCryptoLibDxe.inf
    ...

[LibraryClasses.IA32.PEIM]
    BaseCryptLib|SharedCryptoPkg/Library/SharedCryptoLib/SharedCryptoLibPei.inf
    ...

[LibraryClasses.DXE_SMM]
    BaseCryptLib|SharedCryptoPkg/Library/SharedCryptoLib/SharedCryptoLibSmm.inf
    ...
```

## Sample FDF change

TODO: re-evluate this once nuget dependency is live
Include this file in your FV and the module will get loaded.

```
[FV.FVDXE]
    INF  SharedCryptoPkg/Package/SharedCryptoPkgDxe.inf
    ...
    ...
```
