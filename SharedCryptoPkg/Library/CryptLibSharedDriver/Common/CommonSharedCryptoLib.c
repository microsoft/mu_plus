/** @file
SharedCryptoLib.c

This defines the implementation of BaseCryptLib and calls out to the protocol
published by SharedCryptoDriver. The ProtocolFunctionNotFound and GetProtocol
funtion is defined by the flavor of the library.

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
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <SharedCryptoHelpers.h>


//=====================================================================================
//    One-Way Cryptographic Hash Primitives
//=====================================================================================

/**
  Retrieves the size, in bytes, of the context buffer required for MD4 hash operations.

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for MD4 hash operations.
  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
Md4GetContextSize (
    VOID)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->MD4_GetContextSize == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return 0;
  }
  return prot->MD4_GetContextSize();
}

/**
  Initializes user-supplied memory pointed by Md4Context as MD4 hash context for
  subsequent use.

  If Md4Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  Md4Context  Pointer to MD4 context being initialized.

  @retval TRUE   MD4 context initialization succeeded.
  @retval FALSE  MD4 context initialization failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md4Init (
    OUT VOID *Md4Context)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->MD4_Init == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return 0;
  }
  return prot->MD4_Init(Md4Context);
}

/**
  Makes a copy of an existing MD4 context.

  If Md4Context is NULL, then return FALSE.
  If NewMd4Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Md4Context     Pointer to MD4 context being copied.
  @param[out] NewMd4Context  Pointer to new MD4 context.

  @retval TRUE   MD4 context copy succeeded.
  @retval FALSE  MD4 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md4Duplicate (
    IN CONST VOID *Md4Context,
    OUT VOID *NewMd4Context)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->MD4_Duplicate == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->MD4_Duplicate(Md4Context, NewMd4Context);
}

/**
  Digests the input data and updates MD4 context.

  This function performs MD4 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  MD4 context should be already correctly initialized by Md4Init(), and should not be finalized
  by Md4Final(). Behavior with invalid context is undefined.

  If Md4Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Md4Context  Pointer to the MD4 context.
  @param[in]       Data        Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize    Size of Data buffer in bytes.

  @retval TRUE   MD4 data digest succeeded.
  @retval FALSE  MD4 data digest failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md4Update (
    IN OUT VOID *Md4Context,
    IN CONST VOID *Data,
    IN UINTN DataSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->MD4_Update == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->MD4_Update(Md4Context, Data, DataSize);
}

/**
  Completes computation of the MD4 digest value.

  This function completes MD4 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the MD4 context cannot
  be used again.
  MD4 context should be already correctly initialized by Md4Init(), and should not be
  finalized by Md4Final(). Behavior with invalid MD4 context is undefined.

  If Md4Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Md4Context  Pointer to the MD4 context.
  @param[out]      HashValue   Pointer to a buffer that receives the MD4 digest
                               value (16 bytes).

  @retval TRUE   MD4 digest computation succeeded.
  @retval FALSE  MD4 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md4Final (
    IN OUT VOID *Md4Context,
    OUT UINT8 *HashValue)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->MD4_Final == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->MD4_Final(Md4Context, HashValue);
}

/**
  Computes the MD4 message digest of a input data buffer.

  This function performs the MD4 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the MD4 digest
                           value (16 bytes).

  @retval TRUE   MD4 digest computation succeeded.
  @retval FALSE  MD4 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md4HashAll (
    IN CONST VOID *Data,
    IN UINTN DataSize,
    OUT UINT8 *HashValue)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->MD4_HashAll == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->MD4_HashAll(Data, DataSize, HashValue);
}

/**
  Retrieves the size, in bytes, of the context buffer required for MD5 hash operations.

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for MD5 hash operations.
  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
Md5GetContextSize (
    VOID)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return 0;

  if (prot->MD5_GetContextSize == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return 0;
  }

  return prot->MD5_GetContextSize();
}

/**
  Initializes user-supplied memory pointed by Md5Context as MD5 hash context for
  subsequent use.

  If Md5Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  Md5Context  Pointer to MD5 context being initialized.

  @retval TRUE   MD5 context initialization succeeded.
  @retval FALSE  MD5 context initialization failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md5Init (
    OUT VOID *Md5Context)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return FALSE;

  if (prot->MD5_Init == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->MD5_Init(Md5Context);
}

/**
  Makes a copy of an existing MD5 context.

  If Md5Context is NULL, then return FALSE.
  If NewMd5Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Md5Context     Pointer to MD5 context being copied.
  @param[out] NewMd5Context  Pointer to new MD5 context.

  @retval TRUE   MD5 context copy succeeded.
  @retval FALSE  MD5 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md5Duplicate (
    IN CONST VOID *Md5Context,
    OUT VOID *NewMd5Context)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return FALSE;

  if (prot->MD5_Update == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->MD5_Duplicate(Md5Context, NewMd5Context);
}

/**
  Digests the input data and updates MD5 context.

  This function performs MD5 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  MD5 context should be already correctly initialized by Md5Init(), and should not be finalized
  by Md5Final(). Behavior with invalid context is undefined.

  If Md5Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Md5Context  Pointer to the MD5 context.
  @param[in]       Data        Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize    Size of Data buffer in bytes.

  @retval TRUE   MD5 data digest succeeded.
  @retval FALSE  MD5 data digest failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md5Update (
    IN OUT VOID *Md5Context,
    IN CONST VOID *Data,
    IN UINTN DataSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return FALSE;

  if (prot->MD5_Update == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->MD5_Update(Md5Context, Data, DataSize);
}

/**
  Completes computation of the MD5 digest value.

  This function completes MD5 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the MD5 context cannot
  be used again.
  MD5 context should be already correctly initialized by Md5Init(), and should not be
  finalized by Md5Final(). Behavior with invalid MD5 context is undefined.

  If Md5Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Md5Context  Pointer to the MD5 context.
  @param[out]      HashValue   Pointer to a buffer that receives the MD5 digest
                               value (16 bytes).

  @retval TRUE   MD5 digest computation succeeded.
  @retval FALSE  MD5 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md5Final (
    IN OUT VOID *Md5Context,
    OUT UINT8 *HashValue)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return FALSE;

  if (prot->MD5_Final == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->MD5_Final(Md5Context, HashValue);
}

/**
  Computes the MD5 message digest of a input data buffer.

  This function performs the MD5 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the MD5 digest
                           value (16 bytes).

  @retval TRUE   MD5 digest computation succeeded.
  @retval FALSE  MD5 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Md5HashAll (
    IN CONST VOID *Data,
    IN UINTN DataSize,
    OUT UINT8 *HashValue)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return FALSE;

  if (prot->MD5_HashAll == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->MD5_HashAll(Data, DataSize, HashValue);
}

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-1 hash operations.

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for SHA-1 hash operations.
  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
Sha1GetContextSize (
    VOID)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return 0;
  if (prot->SHA1_GET_CONTEXT_SIZE == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return 0;
  }

  return prot->SHA1_GET_CONTEXT_SIZE();
}

/**
  Initializes user-supplied memory pointed by Sha1Context as SHA-1 hash context for
  subsequent use.

  If Sha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  Sha1Context  Pointer to SHA-1 context being initialized.

  @retval TRUE   SHA-1 context initialization succeeded.
  @retval FALSE  SHA-1 context initialization failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha1Init (
    OUT VOID *Sha1Context)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return FALSE;
  if (prot->SHA1_INIT == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->SHA1_INIT(Sha1Context);
}

/**
  Makes a copy of an existing SHA-1 context.

  If Sha1Context is NULL, then return FALSE.
  If NewSha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Sha1Context     Pointer to SHA-1 context being copied.
  @param[out] NewSha1Context  Pointer to new SHA-1 context.

  @retval TRUE   SHA-1 context copy succeeded.
  @retval FALSE  SHA-1 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha1Duplicate (
    IN CONST VOID *Sha1Context,
    OUT VOID *NewSha1Context)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return FALSE;
  if (prot->SHA1_DUPLICATE == NULL)
    return FALSE;

  return prot->SHA1_DUPLICATE(Sha1Context, NewSha1Context);
}

/**
  Digests the input data and updates SHA-1 context.

  This function performs SHA-1 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SHA-1 context should be already correctly initialized by Sha1Init(), and should not be finalized
  by Sha1Final(). Behavior with invalid context is undefined.

  If Sha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Sha1Context  Pointer to the SHA-1 context.
  @param[in]       Data         Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize     Size of Data buffer in bytes.

  @retval TRUE   SHA-1 data digest succeeded.
  @retval FALSE  SHA-1 data digest failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha1Update (
    IN OUT VOID *Sha1Context,
    IN CONST VOID *Data,
    IN UINTN DataSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return FALSE;
  if (prot->SHA1_UPDATE == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->SHA1_UPDATE(Sha1Context, Data, DataSize);
}

/**
  Completes computation of the SHA-1 digest value.

  This function completes SHA-1 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SHA-1 context cannot
  be used again.
  SHA-1 context should be already correctly initialized by Sha1Init(), and should not be
  finalized by Sha1Final(). Behavior with invalid SHA-1 context is undefined.

  If Sha1Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Sha1Context  Pointer to the SHA-1 context.
  @param[out]      HashValue    Pointer to a buffer that receives the SHA-1 digest
                                value (20 bytes).

  @retval TRUE   SHA-1 digest computation succeeded.
  @retval FALSE  SHA-1 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha1Final (
    IN OUT VOID *Sha1Context,
    OUT UINT8 *HashValue)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return FALSE;
  if (prot->SHA1_FINAL == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->SHA1_FINAL(Sha1Context, HashValue);
}

/**
  Computes the SHA-1 message digest of a input data buffer.

  This function performs the SHA-1 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the SHA-1 digest
                           value (20 bytes).

  @retval TRUE   SHA-1 digest computation succeeded.
  @retval FALSE  SHA-1 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha1HashAll (
    IN CONST VOID *Data,
    IN UINTN DataSize,
    OUT UINT8 *HashValue)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->SHA1_HASH_ALL == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->SHA1_HASH_ALL(Data, DataSize, HashValue);
}

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-256 hash operations.

  @return  The size, in bytes, of the context buffer required for SHA-256 hash operations.

**/
UINTN
EFIAPI
Sha256GetContextSize (
    VOID)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return 0;
  if (prot->SHA256_GET_CONTEXT_SIZE == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return 0;
  }

  return prot->SHA256_GET_CONTEXT_SIZE();
}

/**
  Initializes user-supplied memory pointed by Sha256Context as SHA-256 hash context for
  subsequent use.

  If Sha256Context is NULL, then return FALSE.

  @param[out]  Sha256Context  Pointer to SHA-256 context being initialized.

  @retval TRUE   SHA-256 context initialization succeeded.
  @retval FALSE  SHA-256 context initialization failed.

**/
BOOLEAN
EFIAPI
Sha256Init (
    OUT VOID *Sha256Context)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return FALSE;
  if (prot->SHA256_INIT == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->SHA256_INIT(Sha256Context);
}

/**
  Makes a copy of an existing SHA-256 context.

  If Sha256Context is NULL, then return FALSE.
  If NewSha256Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Sha256Context     Pointer to SHA-256 context being copied.
  @param[out] NewSha256Context  Pointer to new SHA-256 context.

  @retval TRUE   SHA-256 context copy succeeded.
  @retval FALSE  SHA-256 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha256Duplicate (
    IN CONST VOID *Sha256Context,
    OUT VOID *NewSha256Context)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return FALSE;
  if (prot->SHA256_DUPLICATE == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->SHA256_DUPLICATE(Sha256Context, NewSha256Context);
}

/**
  Digests the input data and updates SHA-256 context.

  This function performs SHA-256 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SHA-256 context should be already correctly initialized by Sha256Init(), and should not be finalized
  by Sha256Final(). Behavior with invalid context is undefined.

  If Sha256Context is NULL, then return FALSE.

  @param[in, out]  Sha256Context  Pointer to the SHA-256 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize       Size of Data buffer in bytes.

  @retval TRUE   SHA-256 data digest succeeded.
  @retval FALSE  SHA-256 data digest failed.

**/
BOOLEAN
EFIAPI
Sha256Update (
    IN OUT VOID *Sha256Context,
    IN CONST VOID *Data,
    IN UINTN DataSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return FALSE;
  if (prot->SHA256_UPDATE == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->SHA256_UPDATE(Sha256Context, Data, DataSize);
}

/**
  Completes computation of the SHA-256 digest value.

  This function completes SHA-256 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SHA-256 context cannot
  be used again.
  SHA-256 context should be already correctly initialized by Sha256Init(), and should not be
  finalized by Sha256Final(). Behavior with invalid SHA-256 context is undefined.

  If Sha256Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Sha256Context  Pointer to the SHA-256 context.
  @param[out]      HashValue      Pointer to a buffer that receives the SHA-256 digest
                                  value (32 bytes).

  @retval TRUE   SHA-256 digest computation succeeded.
  @retval FALSE  SHA-256 digest computation failed.

**/
BOOLEAN
EFIAPI
Sha256Final (
    IN OUT VOID *Sha256Context,
    OUT UINT8 *HashValue)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL)
    return FALSE;
  if (prot->SHA256_FINAL == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->SHA256_FINAL(Sha256Context, HashValue);
}

/**
  Computes the SHA-256 message digest of a input data buffer.

  This function performs the SHA-256 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the SHA-256 digest
                           value (32 bytes).

  @retval TRUE   SHA-256 digest computation succeeded.
  @retval FALSE  SHA-256 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha256HashAll (
    IN CONST VOID *Data,
    IN UINTN DataSize,
    OUT UINT8 *HashValue)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->SHA256_HASH_ALL == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->SHA256_HASH_ALL(Data, DataSize, HashValue);
}

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-384 hash operations.

  @return  The size, in bytes, of the context buffer required for SHA-384 hash operations.

**/
UINTN
EFIAPI
Sha384GetContextSize (
    VOID)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->SHA384_GetContextSize == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return 0;
  }
  return prot->SHA384_GetContextSize();
}

/**
  Initializes user-supplied memory pointed by Sha384Context as SHA-384 hash context for
  subsequent use.

  If Sha384Context is NULL, then return FALSE.

  @param[out]  Sha384Context  Pointer to SHA-384 context being initialized.

  @retval TRUE   SHA-384 context initialization succeeded.
  @retval FALSE  SHA-384 context initialization failed.

**/
BOOLEAN
EFIAPI
Sha384Init (
    OUT VOID *Sha384Context)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->SHA384_Init == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->SHA384_Init(Sha384Context);
}

/**
  Makes a copy of an existing SHA-384 context.

  If Sha384Context is NULL, then return FALSE.
  If NewSha384Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Sha384Context     Pointer to SHA-384 context being copied.
  @param[out] NewSha384Context  Pointer to new SHA-384 context.

  @retval TRUE   SHA-384 context copy succeeded.
  @retval FALSE  SHA-384 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha384Duplicate (
    IN CONST VOID *Sha384Context,
    OUT VOID *NewSha384Context)
{
 SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->SHA384_Duplicate == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->SHA384_Duplicate(Sha384Context, NewSha384Context);
}

/**
  Digests the input data and updates SHA-384 context.

  This function performs SHA-384 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SHA-384 context should be already correctly initialized by Sha384Init(), and should not be finalized
  by Sha384Final(). Behavior with invalid context is undefined.

  If Sha384Context is NULL, then return FALSE.

  @param[in, out]  Sha384Context  Pointer to the SHA-384 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize       Size of Data buffer in bytes.

  @retval TRUE   SHA-384 data digest succeeded.
  @retval FALSE  SHA-384 data digest failed.

**/
BOOLEAN
EFIAPI
Sha384Update (
    IN OUT VOID *Sha384Context,
    IN CONST VOID *Data,
    IN UINTN DataSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->SHA384_Update == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->SHA384_Update(Sha384Context, Data, DataSize);
}

/**
  Completes computation of the SHA-384 digest value.

  This function completes SHA-384 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SHA-384 context cannot
  be used again.
  SHA-384 context should be already correctly initialized by Sha384Init(), and should not be
  finalized by Sha384Final(). Behavior with invalid SHA-384 context is undefined.

  If Sha384Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Sha384Context  Pointer to the SHA-384 context.
  @param[out]      HashValue      Pointer to a buffer that receives the SHA-384 digest
                                  value (48 bytes).

  @retval TRUE   SHA-384 digest computation succeeded.
  @retval FALSE  SHA-384 digest computation failed.

**/
BOOLEAN
EFIAPI
Sha384Final (
    IN OUT VOID *Sha384Context,
    OUT UINT8 *HashValue)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->SHA384_Final == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->SHA384_Final(Sha384Context, HashValue);
}

/**
  Computes the SHA-384 message digest of a input data buffer.

  This function performs the SHA-384 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the SHA-384 digest
                           value (48 bytes).

  @retval TRUE   SHA-384 digest computation succeeded.
  @retval FALSE  SHA-384 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha384HashAll (
    IN CONST VOID *Data,
    IN UINTN DataSize,
    OUT UINT8 *HashValue)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->SHA384_HashAll == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->SHA384_HashAll(Data, DataSize, HashValue);
}

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-512 hash operations.

  @return  The size, in bytes, of the context buffer required for SHA-512 hash operations.

**/
UINTN
EFIAPI
Sha512GetContextSize (
    VOID)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->SHA512_GetContextSize == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
     return 0;
   }
  return prot->SHA512_GetContextSize();
}

/**
  Initializes user-supplied memory pointed by Sha512Context as SHA-512 hash context for
  subsequent use.

  If Sha512Context is NULL, then return FALSE.

  @param[out]  Sha512Context  Pointer to SHA-512 context being initialized.

  @retval TRUE   SHA-512 context initialization succeeded.
  @retval FALSE  SHA-512 context initialization failed.

**/
BOOLEAN
EFIAPI
Sha512Init (
    OUT VOID *Sha512Context)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->SHA512_Init == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->SHA512_Init(Sha512Context);
}

/**
  Makes a copy of an existing SHA-512 context.

  If Sha512Context is NULL, then return FALSE.
  If NewSha512Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Sha512Context     Pointer to SHA-512 context being copied.
  @param[out] NewSha512Context  Pointer to new SHA-512 context.

  @retval TRUE   SHA-512 context copy succeeded.
  @retval FALSE  SHA-512 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha512Duplicate (
    IN CONST VOID *Sha512Context,
    OUT VOID *NewSha512Context)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->SHA512_Duplicate == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->SHA512_Duplicate(Sha512Context, NewSha512Context);
}

/**
  Digests the input data and updates SHA-512 context.

  This function performs SHA-512 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SHA-512 context should be already correctly initialized by Sha512Init(), and should not be finalized
  by Sha512Final(). Behavior with invalid context is undefined.

  If Sha512Context is NULL, then return FALSE.

  @param[in, out]  Sha512Context  Pointer to the SHA-512 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize       Size of Data buffer in bytes.

  @retval TRUE   SHA-512 data digest succeeded.
  @retval FALSE  SHA-512 data digest failed.

**/
BOOLEAN
EFIAPI
Sha512Update (
    IN OUT VOID *Sha512Context,
    IN CONST VOID *Data,
    IN UINTN DataSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->SHA512_Update == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->SHA512_Update(Sha512Context, Data, DataSize);
}

/**
  Completes computation of the SHA-512 digest value.

  This function completes SHA-512 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SHA-512 context cannot
  be used again.
  SHA-512 context should be already correctly initialized by Sha512Init(), and should not be
  finalized by Sha512Final(). Behavior with invalid SHA-512 context is undefined.

  If Sha512Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Sha512Context  Pointer to the SHA-512 context.
  @param[out]      HashValue      Pointer to a buffer that receives the SHA-512 digest
                                  value (64 bytes).

  @retval TRUE   SHA-512 digest computation succeeded.
  @retval FALSE  SHA-512 digest computation failed.

**/
BOOLEAN
EFIAPI
Sha512Final (
    IN OUT VOID *Sha512Context,
    OUT UINT8 *HashValue)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->SHA512_Final == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->SHA512_Final(Sha512Context, HashValue);
}

/**
  Computes the SHA-512 message digest of a input data buffer.

  This function performs the SHA-512 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the SHA-512 digest
                           value (64 bytes).

  @retval TRUE   SHA-512 digest computation succeeded.
  @retval FALSE  SHA-512 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha512HashAll (
    IN CONST VOID *Data,
    IN UINTN DataSize,
    OUT UINT8 *HashValue
)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->SHA512_HashAll == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->SHA512_HashAll(Data, DataSize, HashValue);
}

//=====================================================================================
//    MAC (Message Authentication Code) Primitive
//=====================================================================================

/**
  Retrieves the size, in bytes, of the context buffer required for HMAC-MD5 operations.
  (NOTE: This API is deprecated.
         Use HmacMd5New() / HmacMd5Free() for HMAC-MD5 Context operations.)

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for HMAC-MD5 operations.
  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
HmacMd5GetContextSize (
    VOID
)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_MD5_GetContextSize == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
     return 0;
   }
  return prot->HMAC_MD5_GetContextSize();
}

/**
  Allocates and initializes one HMAC_CTX context for subsequent HMAC-MD5 use.

  If this interface is not supported, then return NULL.

  @return  Pointer to the HMAC_CTX context that has been initialized.
           If the allocations fails, HmacMd5New() returns NULL.
  @retval  NULL  This interface is not supported.

**/
VOID *
EFIAPI
HmacMd5New (
  VOID
)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_MD5_New == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->HMAC_MD5_New();
}

/**
  Release the specified HMAC_CTX context.

  If this interface is not supported, then do nothing.

  @param[in]  HmacMd5Ctx  Pointer to the HMAC_CTX context to be released.

**/
VOID
EFIAPI
HmacMd5Free (
  IN VOID *HmacMd5Ctx
)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_MD5_Free == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
     return;
   }
  prot->HMAC_MD5_Free(HmacMd5Ctx);
}

/**
  Initializes user-supplied memory pointed by HmacMd5Context as HMAC-MD5 context for
  subsequent use.

  If HmacMd5Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  HmacMd5Context  Pointer to HMAC-MD5 context being initialized.
  @param[in]   Key             Pointer to the user-supplied key.
  @param[in]   KeySize         Key size in bytes.

  @retval TRUE   HMAC-MD5 context initialization succeeded.
  @retval FALSE  HMAC-MD5 context initialization failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacMd5Init (
    OUT VOID *HmacMd5Context,
    IN CONST UINT8 *Key,
    IN UINTN KeySize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_MD5_Init == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->HMAC_MD5_Init(HmacMd5Context, Key, KeySize);
}

/**
  Makes a copy of an existing HMAC-MD5 context.

  If HmacMd5Context is NULL, then return FALSE.
  If NewHmacMd5Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  HmacMd5Context     Pointer to HMAC-MD5 context being copied.
  @param[out] NewHmacMd5Context  Pointer to new HMAC-MD5 context.

  @retval TRUE   HMAC-MD5 context copy succeeded.
  @retval FALSE  HMAC-MD5 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacMd5Duplicate (
    IN CONST VOID *HmacMd5Context,
    OUT VOID *NewHmacMd5Context)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_MD5_Duplicate == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->HMAC_MD5_Duplicate(HmacMd5Context, NewHmacMd5Context);
}

/**
  Digests the input data and updates HMAC-MD5 context.

  This function performs HMAC-MD5 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-MD5 context should be already correctly initialized by HmacMd5Init(), and should not be
  finalized by HmacMd5Final(). Behavior with invalid context is undefined.

  If HmacMd5Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HmacMd5Context  Pointer to the HMAC-MD5 context.
  @param[in]       Data            Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize        Size of Data buffer in bytes.

  @retval TRUE   HMAC-MD5 data digest succeeded.
  @retval FALSE  HMAC-MD5 data digest failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacMd5Update (
    IN OUT VOID *HmacMd5Context,
    IN CONST VOID *Data,
    IN UINTN DataSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_MD5_Update == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->HMAC_MD5_Update(HmacMd5Context, Data, DataSize);
}

/**
  Completes computation of the HMAC-MD5 digest value.

  This function completes HMAC-MD5 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-MD5 context cannot
  be used again.
  HMAC-MD5 context should be already correctly initialized by HmacMd5Init(), and should not be
  finalized by HmacMd5Final(). Behavior with invalid HMAC-MD5 context is undefined.

  If HmacMd5Context is NULL, then return FALSE.
  If HmacValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HmacMd5Context  Pointer to the HMAC-MD5 context.
  @param[out]      HmacValue       Pointer to a buffer that receives the HMAC-MD5 digest
                                   value (16 bytes).

  @retval TRUE   HMAC-MD5 digest computation succeeded.
  @retval FALSE  HMAC-MD5 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacMd5Final (
    IN OUT VOID *HmacMd5Context,
    OUT UINT8 *HmacValue)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_MD5_Final == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->HMAC_MD5_Final(HmacMd5Context, HmacValue);
}

/**
  Retrieves the size, in bytes, of the context buffer required for HMAC-SHA1 operations.
  (NOTE: This API is deprecated.
         Use HmacSha1New() / HmacSha1Free() for HMAC-SHA1 Context operations.)

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for HMAC-SHA1 operations.
  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
HmacSha1GetContextSize (
    VOID)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_SHA1_GetContextSize == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return 0;
  }

  return prot->HMAC_SHA1_GetContextSize();
}

/**
  Allocates and initializes one HMAC_CTX context for subsequent HMAC-SHA1 use.

  If this interface is not supported, then return NULL.

  @return  Pointer to the HMAC_CTX context that has been initialized.
           If the allocations fails, HmacSha1New() returns NULL.
  @return  NULL   This interface is not supported.

**/
VOID *
EFIAPI
HmacSha1New (
  VOID
)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL||prot->HMAC_SHA1_New == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return NULL;
  }

  return prot->HMAC_SHA1_New();
}

/**
  Release the specified HMAC_CTX context.

  If this interface is not supported, then do nothing.

  @param[in]  HmacSha1Ctx  Pointer to the HMAC_CTX context to be released.

**/
VOID
    EFIAPI
    HmacSha1Free (
        IN VOID *HmacSha1Ctx)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_SHA1_Free == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return;
  }

  prot->HMAC_SHA1_Free(HmacSha1Ctx);
}

/**
  Initializes user-supplied memory pointed by HmacSha1Context as HMAC-SHA1 context for
  subsequent use.

  If HmacSha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  HmacSha1Context  Pointer to HMAC-SHA1 context being initialized.
  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          Key size in bytes.

  @retval TRUE   HMAC-SHA1 context initialization succeeded.
  @retval FALSE  HMAC-SHA1 context initialization failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha1Init (
    OUT VOID *HmacSha1Context,
    IN CONST UINT8 *Key,
    IN UINTN KeySize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_SHA1_Init == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->HMAC_SHA1_Init(HmacSha1Context, Key, KeySize);
}

/**
  Makes a copy of an existing HMAC-SHA1 context.

  If HmacSha1Context is NULL, then return FALSE.
  If NewHmacSha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  HmacSha1Context     Pointer to HMAC-SHA1 context being copied.
  @param[out] NewHmacSha1Context  Pointer to new HMAC-SHA1 context.

  @retval TRUE   HMAC-SHA1 context copy succeeded.
  @retval FALSE  HMAC-SHA1 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha1Duplicate (
    IN CONST VOID *HmacSha1Context,
    OUT VOID *NewHmacSha1Context)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_SHA1_Duplicate == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->HMAC_SHA1_Duplicate(HmacSha1Context, NewHmacSha1Context);
}

/**
  Digests the input data and updates HMAC-SHA1 context.

  This function performs HMAC-SHA1 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-SHA1 context should be already correctly initialized by HmacSha1Init(), and should not
  be finalized by HmacSha1Final(). Behavior with invalid context is undefined.

  If HmacSha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HmacSha1Context Pointer to the HMAC-SHA1 context.
  @param[in]       Data            Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize        Size of Data buffer in bytes.

  @retval TRUE   HMAC-SHA1 data digest succeeded.
  @retval FALSE  HMAC-SHA1 data digest failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha1Update (
    IN OUT VOID *HmacSha1Context,
    IN CONST VOID *Data,
    IN UINTN DataSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_SHA1_Update == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->HMAC_SHA1_Update(HmacSha1Context, Data, DataSize);
}

/**
  Completes computation of the HMAC-SHA1 digest value.

  This function completes HMAC-SHA1 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-SHA1 context cannot
  be used again.
  HMAC-SHA1 context should be already correctly initialized by HmacSha1Init(), and should
  not be finalized by HmacSha1Final(). Behavior with invalid HMAC-SHA1 context is undefined.

  If HmacSha1Context is NULL, then return FALSE.
  If HmacValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HmacSha1Context  Pointer to the HMAC-SHA1 context.
  @param[out]      HmacValue        Pointer to a buffer that receives the HMAC-SHA1 digest
                                    value (20 bytes).

  @retval TRUE   HMAC-SHA1 digest computation succeeded.
  @retval FALSE  HMAC-SHA1 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha1Final (
    IN OUT VOID *HmacSha1Context,
    OUT UINT8 *HmacValue)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_SHA1_Final == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->HMAC_SHA1_Final(HmacSha1Context, HmacValue);
}

/**
  Retrieves the size, in bytes, of the context buffer required for HMAC-SHA256 operations.
  (NOTE: This API is deprecated.
         Use HmacSha256New() / HmacSha256Free() for HMAC-SHA256 Context operations.)

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for HMAC-SHA256 operations.
  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
HmacSha256GetContextSize (
    VOID)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_SHA256_GetContextSize == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return 0;
  }

  return prot->HMAC_SHA256_GetContextSize();
}

/**
  Allocates and initializes one HMAC_CTX context for subsequent HMAC-SHA256 use.

  @return  Pointer to the HMAC_CTX context that has been initialized.
           If the allocations fails, HmacSha256New() returns NULL.

**/
VOID *
    EFIAPI
        HmacSha256New (
            VOID)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_SHA256_New == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return NULL;
  }

  return prot->HMAC_SHA256_New();
}

/**
  Release the specified HMAC_CTX context.

  @param[in]  HmacSha256Ctx  Pointer to the HMAC_CTX context to be released.

**/
VOID
    EFIAPI
    HmacSha256Free (
        IN VOID *HmacSha256Ctx)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_SHA256_Free == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return;
  }

  prot->HMAC_SHA256_Free(HmacSha256Ctx);
}

/**
  Initializes user-supplied memory pointed by HmacSha256Context as HMAC-SHA256 context for
  subsequent use.

  If HmacSha256Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  HmacSha256Context  Pointer to HMAC-SHA256 context being initialized.
  @param[in]   Key                Pointer to the user-supplied key.
  @param[in]   KeySize            Key size in bytes.

  @retval TRUE   HMAC-SHA256 context initialization succeeded.
  @retval FALSE  HMAC-SHA256 context initialization failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha256Init (
    OUT VOID *HmacSha256Context,
    IN CONST UINT8 *Key,
    IN UINTN KeySize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_SHA256_Free == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->HMAC_SHA256_Init(HmacSha256Context, Key, KeySize);
}

/**
  Makes a copy of an existing HMAC-SHA256 context.

  If HmacSha256Context is NULL, then return FALSE.
  If NewHmacSha256Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  HmacSha256Context     Pointer to HMAC-SHA256 context being copied.
  @param[out] NewHmacSha256Context  Pointer to new HMAC-SHA256 context.

  @retval TRUE   HMAC-SHA256 context copy succeeded.
  @retval FALSE  HMAC-SHA256 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha256Duplicate (
    IN CONST VOID *HmacSha256Context,
    OUT VOID *NewHmacSha256Context)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_SHA256_Duplicate == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->HMAC_SHA256_Duplicate(HmacSha256Context, NewHmacSha256Context);
}

/**
  Digests the input data and updates HMAC-SHA256 context.

  This function performs HMAC-SHA256 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-SHA256 context should be already correctly initialized by HmacSha256Init(), and should not
  be finalized by HmacSha256Final(). Behavior with invalid context is undefined.

  If HmacSha256Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HmacSha256Context Pointer to the HMAC-SHA256 context.
  @param[in]       Data              Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize          Size of Data buffer in bytes.

  @retval TRUE   HMAC-SHA256 data digest succeeded.
  @retval FALSE  HMAC-SHA256 data digest failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha256Update (
    IN OUT VOID *HmacSha256Context,
    IN CONST VOID *Data,
    IN UINTN DataSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_SHA256_Update == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->HMAC_SHA256_Update(HmacSha256Context, Data, DataSize);
}

/**
  Completes computation of the HMAC-SHA256 digest value.

  This function completes HMAC-SHA256 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-SHA256 context cannot
  be used again.
  HMAC-SHA256 context should be already correctly initialized by HmacSha256Init(), and should
  not be finalized by HmacSha256Final(). Behavior with invalid HMAC-SHA256 context is undefined.

  If HmacSha256Context is NULL, then return FALSE.
  If HmacValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HmacSha256Context  Pointer to the HMAC-SHA256 context.
  @param[out]      HmacValue          Pointer to a buffer that receives the HMAC-SHA256 digest
                                      value (32 bytes).

  @retval TRUE   HMAC-SHA256 digest computation succeeded.
  @retval FALSE  HMAC-SHA256 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha256Final (
    IN OUT VOID *HmacSha256Context,
    OUT UINT8 *HmacValue)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->HMAC_SHA256_Final == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->HMAC_SHA256_Final(HmacSha256Context, HmacValue);
}

//=====================================================================================
//    Symmetric Cryptography Primitive
//=====================================================================================

/**
  Retrieves the size, in bytes, of the context buffer required for TDES operations.

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for TDES operations.
  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
TdesGetContextSize (
    VOID)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->TDES_GetContextSize == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return 0;
  }
  return prot->TDES_GetContextSize();
}

/**
  Initializes user-supplied memory as TDES context for subsequent use.

  This function initializes user-supplied memory pointed by TdesContext as TDES context.
  In addition, it sets up all TDES key materials for subsequent encryption and decryption
  operations.
  There are 3 key options as follows:
  KeyLength = 64,  Keying option 1: K1 == K2 == K3 (Backward compatibility with DES)
  KeyLength = 128, Keying option 2: K1 != K2 and K3 = K1 (Less Security)
  KeyLength = 192  Keying option 3: K1 != K2 != K3 (Strongest)

  If TdesContext is NULL, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeyLength is not valid, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  TdesContext  Pointer to TDES context being initialized.
  @param[in]   Key          Pointer to the user-supplied TDES key.
  @param[in]   KeyLength    Length of TDES key in bits.

  @retval TRUE   TDES context initialization succeeded.
  @retval FALSE  TDES context initialization failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
TdesInit (
    OUT VOID *TdesContext,
    IN CONST UINT8 *Key,
    IN UINTN KeyLength)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->TDES_Init == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->TDES_Init(TdesContext, Key, KeyLength);
}

/**
  Performs TDES encryption on a data buffer of the specified size in ECB mode.

  This function performs TDES encryption on data buffer pointed by Input, of specified
  size of InputSize, in ECB mode.
  InputSize must be multiple of block size (8 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  TdesContext should be already correctly initialized by TdesInit(). Behavior with
  invalid TDES context is undefined.

  If TdesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (8 bytes), then return FALSE.
  If Output is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[out]  Output       Pointer to a buffer that receives the TDES encryption output.

  @retval TRUE   TDES encryption succeeded.
  @retval FALSE  TDES encryption failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
TdesEcbEncrypt (
    IN VOID *TdesContext,
    IN CONST UINT8 *Input,
    IN UINTN InputSize,
    OUT UINT8 *Output)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->TDES_EcbEncrypt == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->TDES_EcbEncrypt(TdesContext, Input, InputSize, Output);
}

/**
  Performs TDES decryption on a data buffer of the specified size in ECB mode.

  This function performs TDES decryption on data buffer pointed by Input, of specified
  size of InputSize, in ECB mode.
  InputSize must be multiple of block size (8 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  TdesContext should be already correctly initialized by TdesInit(). Behavior with
  invalid TDES context is undefined.

  If TdesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (8 bytes), then return FALSE.
  If Output is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be decrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[out]  Output       Pointer to a buffer that receives the TDES decryption output.

  @retval TRUE   TDES decryption succeeded.
  @retval FALSE  TDES decryption failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
TdesEcbDecrypt (
    IN VOID *TdesContext,
    IN CONST UINT8 *Input,
    IN UINTN InputSize,
    OUT UINT8 *Output)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->TDES_EcbDecrypt == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->TDES_EcbDecrypt(TdesContext, Input, InputSize, Output);
}

/**
  Performs TDES encryption on a data buffer of the specified size in CBC mode.

  This function performs TDES encryption on data buffer pointed by Input, of specified
  size of InputSize, in CBC mode.
  InputSize must be multiple of block size (8 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  Initialization vector should be one block size (8 bytes).
  TdesContext should be already correctly initialized by TdesInit(). Behavior with
  invalid TDES context is undefined.

  If TdesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (8 bytes), then return FALSE.
  If Ivec is NULL, then return FALSE.
  If Output is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[in]   Ivec         Pointer to initialization vector.
  @param[out]  Output       Pointer to a buffer that receives the TDES encryption output.

  @retval TRUE   TDES encryption succeeded.
  @retval FALSE  TDES encryption failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
TdesCbcEncrypt (
    IN VOID *TdesContext,
    IN CONST UINT8 *Input,
    IN UINTN InputSize,
    IN CONST UINT8 *Ivec,
    OUT UINT8 *Output)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->TDES_CbcEncrypt == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->TDES_CbcEncrypt(TdesContext, Input, InputSize, Ivec, Output);
}

/**
  Performs TDES decryption on a data buffer of the specified size in CBC mode.

  This function performs TDES decryption on data buffer pointed by Input, of specified
  size of InputSize, in CBC mode.
  InputSize must be multiple of block size (8 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  Initialization vector should be one block size (8 bytes).
  TdesContext should be already correctly initialized by TdesInit(). Behavior with
  invalid TDES context is undefined.

  If TdesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (8 bytes), then return FALSE.
  If Ivec is NULL, then return FALSE.
  If Output is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[in]   Ivec         Pointer to initialization vector.
  @param[out]  Output       Pointer to a buffer that receives the TDES encryption output.

  @retval TRUE   TDES decryption succeeded.
  @retval FALSE  TDES decryption failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
TdesCbcDecrypt (
    IN VOID *TdesContext,
    IN CONST UINT8 *Input,
    IN UINTN InputSize,
    IN CONST UINT8 *Ivec,
    OUT UINT8 *Output)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->TDES_CbcDecrypt == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->TDES_CbcDecrypt(TdesContext, Input, InputSize, Ivec, Output);
}

/**
  Retrieves the size, in bytes, of the context buffer required for AES operations.

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for AES operations.
  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
AesGetContextSize (
    VOID)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->AES_GetContextSize == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return 0;
  }
  return prot->AES_GetContextSize();
}

/**
  Initializes user-supplied memory as AES context for subsequent use.

  This function initializes user-supplied memory pointed by AesContext as AES context.
  In addition, it sets up all AES key materials for subsequent encryption and decryption
  operations.
  There are 3 options for key length, 128 bits, 192 bits, and 256 bits.

  If AesContext is NULL, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeyLength is not valid, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  AesContext  Pointer to AES context being initialized.
  @param[in]   Key         Pointer to the user-supplied AES key.
  @param[in]   KeyLength   Length of AES key in bits.

  @retval TRUE   AES context initialization succeeded.
  @retval FALSE  AES context initialization failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
AesInit (
    OUT VOID *AesContext,
    IN CONST UINT8 *Key,
    IN UINTN KeyLength)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->AES_Init == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->AES_Init(AesContext, Key, KeyLength);
}

/**
  Performs AES encryption on a data buffer of the specified size in ECB mode.

  This function performs AES encryption on data buffer pointed by Input, of specified
  size of InputSize, in ECB mode.
  InputSize must be multiple of block size (16 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  AesContext should be already correctly initialized by AesInit(). Behavior with
  invalid AES context is undefined.

  If AesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (16 bytes), then return FALSE.
  If Output is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[out]  Output      Pointer to a buffer that receives the AES encryption output.

  @retval TRUE   AES encryption succeeded.
  @retval FALSE  AES encryption failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
AesEcbEncrypt (
    IN VOID *AesContext,
    IN CONST UINT8 *Input,
    IN UINTN InputSize,
    OUT UINT8 *Output)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->AES_EcbEncrypt == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->AES_EcbEncrypt(AesContext, Input, InputSize, Output);
}

/**
  Performs AES decryption on a data buffer of the specified size in ECB mode.

  This function performs AES decryption on data buffer pointed by Input, of specified
  size of InputSize, in ECB mode.
  InputSize must be multiple of block size (16 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  AesContext should be already correctly initialized by AesInit(). Behavior with
  invalid AES context is undefined.

  If AesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (16 bytes), then return FALSE.
  If Output is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be decrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[out]  Output      Pointer to a buffer that receives the AES decryption output.

  @retval TRUE   AES decryption succeeded.
  @retval FALSE  AES decryption failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
AesEcbDecrypt (
    IN VOID *AesContext,
    IN CONST UINT8 *Input,
    IN UINTN InputSize,
    OUT UINT8 *Output)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->AES_EcbDecrypt == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->AES_EcbDecrypt(AesContext, Input, InputSize, Output);
}

/**
  Performs AES encryption on a data buffer of the specified size in CBC mode.

  This function performs AES encryption on data buffer pointed by Input, of specified
  size of InputSize, in CBC mode.
  InputSize must be multiple of block size (16 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  Initialization vector should be one block size (16 bytes).
  AesContext should be already correctly initialized by AesInit(). Behavior with
  invalid AES context is undefined.

  If AesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (16 bytes), then return FALSE.
  If Ivec is NULL, then return FALSE.
  If Output is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[in]   Ivec        Pointer to initialization vector.
  @param[out]  Output      Pointer to a buffer that receives the AES encryption output.

  @retval TRUE   AES encryption succeeded.
  @retval FALSE  AES encryption failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
AesCbcEncrypt (
    IN VOID *AesContext,
    IN CONST UINT8 *Input,
    IN UINTN InputSize,
    IN CONST UINT8 *Ivec,
    OUT UINT8 *Output)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->AES_CbcEncrypt == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->AES_CbcEncrypt(AesContext, Input, InputSize, Ivec, Output);
}

/**
  Performs AES decryption on a data buffer of the specified size in CBC mode.

  This function performs AES decryption on data buffer pointed by Input, of specified
  size of InputSize, in CBC mode.
  InputSize must be multiple of block size (16 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  Initialization vector should be one block size (16 bytes).
  AesContext should be already correctly initialized by AesInit(). Behavior with
  invalid AES context is undefined.

  If AesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (16 bytes), then return FALSE.
  If Ivec is NULL, then return FALSE.
  If Output is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[in]   Ivec        Pointer to initialization vector.
  @param[out]  Output      Pointer to a buffer that receives the AES encryption output.

  @retval TRUE   AES decryption succeeded.
  @retval FALSE  AES decryption failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
AesCbcDecrypt (
    IN VOID *AesContext,
    IN CONST UINT8 *Input,
    IN UINTN InputSize,
    IN CONST UINT8 *Ivec,
    OUT UINT8 *Output)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->AES_CbcDecrypt == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->AES_CbcDecrypt(AesContext, Input, InputSize, Ivec, Output);
}

/**
  Retrieves the size, in bytes, of the context buffer required for ARC4 operations.

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for ARC4 operations.
  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
Arc4GetContextSize (
    VOID)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->MD4_Update == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return 0;
  }
  return prot->ARC4_GetContextSize();
}

/**
  Initializes user-supplied memory as ARC4 context for subsequent use.

  This function initializes user-supplied memory pointed by Arc4Context as ARC4 context.
  In addition, it sets up all ARC4 key materials for subsequent encryption and decryption
  operations.

  If Arc4Context is NULL, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeySize does not in the range of [5, 256] bytes, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  Arc4Context  Pointer to ARC4 context being initialized.
  @param[in]   Key          Pointer to the user-supplied ARC4 key.
  @param[in]   KeySize      Size of ARC4 key in bytes.

  @retval TRUE   ARC4 context initialization succeeded.
  @retval FALSE  ARC4 context initialization failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Arc4Init (
    OUT VOID *Arc4Context,
    IN CONST UINT8 *Key,
    IN UINTN KeySize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->ARC4_Init == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->ARC4_Init(Arc4Context, Key, KeySize);
}

/**
  Performs ARC4 encryption on a data buffer of the specified size.

  This function performs ARC4 encryption on data buffer pointed by Input, of specified
  size of InputSize.
  Arc4Context should be already correctly initialized by Arc4Init(). Behavior with
  invalid ARC4 context is undefined.

  If Arc4Context is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If Output is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Arc4Context  Pointer to the ARC4 context.
  @param[in]       Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]       InputSize    Size of the Input buffer in bytes.
  @param[out]      Output       Pointer to a buffer that receives the ARC4 encryption output.

  @retval TRUE   ARC4 encryption succeeded.
  @retval FALSE  ARC4 encryption failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Arc4Encrypt (
    IN OUT VOID *Arc4Context,
    IN CONST UINT8 *Input,
    IN UINTN InputSize,
    OUT UINT8 *Output)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->ARC4_Encrypt == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->ARC4_Encrypt(Arc4Context, Input, InputSize, Output);
}

/**
  Performs ARC4 decryption on a data buffer of the specified size.

  This function performs ARC4 decryption on data buffer pointed by Input, of specified
  size of InputSize.
  Arc4Context should be already correctly initialized by Arc4Init(). Behavior with
  invalid ARC4 context is undefined.

  If Arc4Context is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If Output is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Arc4Context  Pointer to the ARC4 context.
  @param[in]       Input        Pointer to the buffer containing the data to be decrypted.
  @param[in]       InputSize    Size of the Input buffer in bytes.
  @param[out]      Output       Pointer to a buffer that receives the ARC4 decryption output.

  @retval TRUE   ARC4 decryption succeeded.
  @retval FALSE  ARC4 decryption failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Arc4Decrypt (
    IN OUT VOID *Arc4Context,
    IN UINT8 *Input,
    IN UINTN InputSize,
    OUT UINT8 *Output
)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->ARC4_Decrypt == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->ARC4_Decrypt(Arc4Context, Input, InputSize, Output);
}

/**
  Resets the ARC4 context to the initial state.

  The function resets the ARC4 context to the state it had immediately after the
  ARC4Init() function call.
  Contrary to ARC4Init(), Arc4Reset() requires no secret key as input, but ARC4 context
  should be already correctly initialized by ARC4Init().

  If Arc4Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Arc4Context  Pointer to the ARC4 context.

  @retval TRUE   ARC4 reset succeeded.
  @retval FALSE  ARC4 reset failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Arc4Reset (
  IN OUT VOID *Arc4Context
)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->ARC4_Reset == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->ARC4_Reset(Arc4Context);
}

//=====================================================================================
//    Asymmetric Cryptography Primitive
//=====================================================================================

/**
  Allocates and initializes one RSA context for subsequent use.

  @return  Pointer to the RSA context that has been initialized.
           If the allocations fails, RsaNew() returns NULL.

**/
VOID *
EFIAPI
RsaNew (
  VOID
)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->RSA_New == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return NULL;
  }
  return prot->RSA_New();
}

/**
  Release the specified RSA context.

  If RsaContext is NULL, then return FALSE.

  @param[in]  RsaContext  Pointer to the RSA context to be released.

**/
VOID
EFIAPI
RsaFree (
    IN VOID *RsaContext
)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->RSA_Free == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return;
  }

  prot->RSA_Free(RsaContext);
}

/**
  Sets the tag-designated key component into the established RSA context.

  This function sets the tag-designated RSA key component into the established
  RSA context from the user-specified non-negative integer (octet string format
  represented in RSA PKCS#1).
  If BigNumber is NULL, then the specified key component in RSA context is cleared.

  If RsaContext is NULL, then return FALSE.

  @param[in, out]  RsaContext  Pointer to RSA context being set.
  @param[in]       KeyTag      Tag of RSA key component being set.
  @param[in]       BigNumber   Pointer to octet integer buffer.
                               If NULL, then the specified key component in RSA
                               context is cleared.
  @param[in]       BnSize      Size of big number buffer in bytes.
                               If BigNumber is NULL, then it is ignored.

  @retval  TRUE   RSA key component was set successfully.
  @retval  FALSE  Invalid RSA key component tag.

**/
BOOLEAN
EFIAPI
RsaSetKey (
    IN OUT VOID *RsaContext,
    IN RSA_KEY_TAG KeyTag,
    IN CONST UINT8 *BigNumber,
    IN UINTN BnSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->RSA_SetKey == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->RSA_SetKey(RsaContext, KeyTag, BigNumber, BnSize);
}

/**
  Gets the tag-designated RSA key component from the established RSA context.

  This function retrieves the tag-designated RSA key component from the
  established RSA context as a non-negative integer (octet string format
  represented in RSA PKCS#1).
  If specified key component has not been set or has been cleared, then returned
  BnSize is set to 0.
  If the BigNumber buffer is too small to hold the contents of the key, FALSE
  is returned and BnSize is set to the required buffer size to obtain the key.

  If RsaContext is NULL, then return FALSE.
  If BnSize is NULL, then return FALSE.
  If BnSize is large enough but BigNumber is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  RsaContext  Pointer to RSA context being set.
  @param[in]       KeyTag      Tag of RSA key component being set.
  @param[out]      BigNumber   Pointer to octet integer buffer.
  @param[in, out]  BnSize      On input, the size of big number buffer in bytes.
                               On output, the size of data returned in big number buffer in bytes.

  @retval  TRUE   RSA key component was retrieved successfully.
  @retval  FALSE  Invalid RSA key component tag.
  @retval  FALSE  BnSize is too small.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
RsaGetKey (
    IN OUT VOID *RsaContext,
    IN RSA_KEY_TAG KeyTag,
    OUT UINT8 *BigNumber,
    IN OUT UINTN *BnSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->RSA_GetKey == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->RSA_GetKey(RsaContext, KeyTag, BigNumber, BnSize);
}

/**
  Generates RSA key components.

  This function generates RSA key components. It takes RSA public exponent E and
  length in bits of RSA modulus N as input, and generates all key components.
  If PublicExponent is NULL, the default RSA public exponent (0x10001) will be used.

  Before this function can be invoked, pseudorandom number generator must be correctly
  initialized by RandomSeed().

  If RsaContext is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  RsaContext           Pointer to RSA context being set.
  @param[in]       ModulusLength        Length of RSA modulus N in bits.
  @param[in]       PublicExponent       Pointer to RSA public exponent.
  @param[in]       PublicExponentSize   Size of RSA public exponent buffer in bytes.

  @retval  TRUE   RSA key component was generated successfully.
  @retval  FALSE  Invalid RSA key component tag.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
RsaGenerateKey (
    IN OUT VOID *RsaContext,
    IN UINTN ModulusLength,
    IN CONST UINT8 *PublicExponent,
    IN UINTN PublicExponentSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->RSA_GenerateKey == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->RSA_GenerateKey(RsaContext, ModulusLength, PublicExponent, PublicExponentSize);
}

/**
  Validates key components of RSA context.
  NOTE: This function performs integrity checks on all the RSA key material, so
        the RSA key structure must contain all the private key data.

  This function validates key components of RSA context in following aspects:
  - Whether p is a prime
  - Whether q is a prime
  - Whether n = p * q
  - Whether d*e = 1  mod lcm(p-1,q-1)

  If RsaContext is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  RsaContext  Pointer to RSA context to check.

  @retval  TRUE   RSA key components are valid.
  @retval  FALSE  RSA key components are not valid.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
RsaCheckKey (
    IN VOID *RsaContext)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->RSA_CheckKey == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->RSA_CheckKey(RsaContext);
}

/**
  Carries out the RSA-SSA signature generation with EMSA-PKCS1-v1_5 encoding scheme.

  This function carries out the RSA-SSA signature generation with EMSA-PKCS1-v1_5 encoding scheme defined in
  RSA PKCS#1.
  If the Signature buffer is too small to hold the contents of signature, FALSE
  is returned and SigSize is set to the required buffer size to obtain the signature.

  If RsaContext is NULL, then return FALSE.
  If MessageHash is NULL, then return FALSE.
  If HashSize is not equal to the size of MD5, SHA-1 or SHA-256 digest, then return FALSE.
  If SigSize is large enough but Signature is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      RsaContext   Pointer to RSA context for signature generation.
  @param[in]      MessageHash  Pointer to octet message hash to be signed.
  @param[in]      HashSize     Size of the message hash in bytes.
  @param[out]     Signature    Pointer to buffer to receive RSA PKCS1-v1_5 signature.
  @param[in, out] SigSize      On input, the size of Signature buffer in bytes.
                               On output, the size of data returned in Signature buffer in bytes.

  @retval  TRUE   Signature successfully generated in PKCS1-v1_5.
  @retval  FALSE  Signature generation failed.
  @retval  FALSE  SigSize is too small.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
RsaPkcs1Sign (
    IN VOID *RsaContext,
    IN CONST UINT8 *MessageHash,
    IN UINTN HashSize,
    OUT UINT8 *Signature,
    IN OUT UINTN *SigSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->RSA_Pkcs1Sign == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->RSA_Pkcs1Sign(RsaContext, MessageHash, HashSize, Signature, SigSize);
}

/**
  Verifies the RSA-SSA signature with EMSA-PKCS1-v1_5 encoding scheme defined in
  RSA PKCS#1.

  If RsaContext is NULL, then return FALSE.
  If MessageHash is NULL, then return FALSE.
  If Signature is NULL, then return FALSE.
  If HashSize is not equal to the size of MD5, SHA-1, SHA-256 digest, then return FALSE.

  @param[in]  RsaContext   Pointer to RSA context for signature verification.
  @param[in]  MessageHash  Pointer to octet message hash to be checked.
  @param[in]  HashSize     Size of the message hash in bytes.
  @param[in]  Signature    Pointer to RSA PKCS1-v1_5 signature to be verified.
  @param[in]  SigSize      Size of signature in bytes.

  @retval  TRUE   Valid signature encoded in PKCS1-v1_5.
  @retval  FALSE  Invalid signature or invalid RSA context.

**/
BOOLEAN
EFIAPI
RsaPkcs1Verify (
    IN VOID *RsaContext,
    IN CONST UINT8 *MessageHash,
    IN UINTN HashSize,
    IN CONST UINT8 *Signature,
    IN UINTN SigSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  EFI_STATUS Status;
  if (prot == NULL || prot->RSA_Pkcs1Verify == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  Status = prot->RSA_Pkcs1Verify(RsaContext, MessageHash, HashSize, Signature, SigSize);
  if (EFI_ERROR(Status))
    return FALSE;
  return TRUE;
}

/**
  Retrieve the RSA Private Key from the password-protected PEM key data.

  If PemData is NULL, then return FALSE.
  If RsaContext is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  PemData      Pointer to the PEM-encoded key data to be retrieved.
  @param[in]  PemSize      Size of the PEM key data in bytes.
  @param[in]  Password     NULL-terminated passphrase used for encrypted PEM key data.
  @param[out] RsaContext   Pointer to new-generated RSA context which contain the retrieved
                           RSA private key component. Use RsaFree() function to free the
                           resource.

  @retval  TRUE   RSA Private Key was retrieved successfully.
  @retval  FALSE  Invalid PEM key data or incorrect password.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
RsaGetPrivateKeyFromPem (
    IN CONST UINT8 *PemData,
    IN UINTN PemSize,
    IN CONST CHAR8 *Password,
    OUT VOID **RsaContext)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->RSA_GetPrivateKeyFromPem == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->RSA_GetPrivateKeyFromPem(PemData, PemSize, Password, RsaContext);
}

/**
  Retrieve the RSA Public Key from one DER-encoded X509 certificate.

  If Cert is NULL, then return FALSE.
  If RsaContext is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]  CertSize     Size of the X509 certificate in bytes.
  @param[out] RsaContext   Pointer to new-generated RSA context which contain the retrieved
                           RSA public key component. Use RsaFree() function to free the
                           resource.

  @retval  TRUE   RSA Public Key was retrieved successfully.
  @retval  FALSE  Fail to retrieve RSA public key from X509 certificate.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
RsaGetPublicKeyFromX509 (
    IN CONST UINT8 *Cert,
    IN UINTN CertSize,
    OUT VOID **RsaContext)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  EFI_STATUS Status;
  if (prot == NULL || prot->RSA_GetPublicKeyFromX509 == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  Status = prot->RSA_GetPublicKeyFromX509(Cert, CertSize, RsaContext);
  if (EFI_ERROR(Status))
    return FALSE;
  return TRUE;
}

/**
  Retrieve the subject bytes from one X.509 certificate.

  If Cert is NULL, then return FALSE.
  If SubjectSize is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[out]     CertSubject  Pointer to the retrieved certificate subject bytes.
  @param[in, out] SubjectSize  The size in bytes of the CertSubject buffer on input,
                               and the size of buffer returned CertSubject on output.

  @retval  TRUE   The certificate subject retrieved successfully.
  @retval  FALSE  Invalid certificate, or the SubjectSize is too small for the result.
                  The SubjectSize will be updated with the required size.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
X509GetSubjectName (
    IN CONST UINT8 *Cert,
    IN UINTN CertSize,
    OUT UINT8 *CertSubject,
    IN OUT UINTN *SubjectSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  EFI_STATUS Status;
  if (prot == NULL || prot->X509_GET_SUBJECT_NAME == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  Status = prot->X509_GET_SUBJECT_NAME(Cert, CertSize, CertSubject, SubjectSize);
  if (EFI_ERROR(Status))
    return FALSE;
  return TRUE;
}

/**
  Retrieve the common name (CN) string from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     CommonName       Buffer to contain the retrieved certificate common
                                   name string (UTF8). At most CommonNameSize bytes will be
                                   written and the string will be null terminated. May be
                                   NULL in order to determine the size buffer needed.
  @param[in,out]  CommonNameSize   The size in bytes of the CommonName buffer on input,
                                   and the size of buffer returned CommonName on output.
                                   If CommonName is NULL then the amount of space needed
                                   in buffer (including the final null) is returned.

  @retval RETURN_SUCCESS           The certificate CommonName retrieved successfully.
  @retval RETURN_INVALID_PARAMETER If Cert is NULL.
                                   If CommonNameSize is NULL.
                                   If CommonName is not NULL and *CommonNameSize is 0.
                                   If Certificate is invalid.
  @retval RETURN_NOT_FOUND         If no CommonName entry exists.
  @retval RETURN_BUFFER_TOO_SMALL  If the CommonName is NULL. The required buffer size
                                   (including the final null) is returned in the
                                   CommonNameSize parameter.
  @retval RETURN_UNSUPPORTED       The operation is not supported.

**/
RETURN_STATUS
EFIAPI
X509GetCommonName (
    IN CONST UINT8 *Cert,
    IN UINTN CertSize,
    OUT CHAR8 *CommonName,
    OPTIONAL IN OUT UINTN *CommonNameSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->X509_GET_COMMON_NAME == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return EFI_PROTOCOL_UNREACHABLE;
  }

  return prot->X509_GET_COMMON_NAME(Cert, CertSize, CommonName, CommonNameSize);
}

// MSCHANGE [Begin]
/**
  Retrieve the organization name (O) string from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     NameBuffer       Buffer to contain the retrieved certificate organization
                                   name string. At most NameBufferSize bytes will be
                                   written and the string will be null terminated. May be
                                   NULL in order to determine the size buffer needed.
  @param[in,out]  NameBufferSiz e  The size in bytes of the Name buffer on input,
                                   and the size of buffer returned Name on output.
                                   If NameBuffer is NULL then the amount of space needed
                                   in buffer (including the final null) is returned.

  @retval RETURN_SUCCESS           The certificate Organization Name retrieved successfully.
  @retval RETURN_INVALID_PARAMETER If Cert is NULL.
                                   If NameBufferSize is NULL.
                                   If NameBuffer is not NULL and *CommonNameSize is 0.
                                   If Certificate is invalid.
  @retval RETURN_NOT_FOUND         If no Organization Name entry exists.
  @retval RETURN_BUFFER_TOO_SMALL  If the NameBuffer is NULL. The required buffer size
                                   (including the final null) is returned in the
                                   CommonNameSize parameter.
  @retval RETURN_UNSUPPORTED       The operation is not supported.

**/
RETURN_STATUS
EFIAPI
X509GetOrganizationName (
    IN CONST UINT8 *Cert,
    IN UINTN CertSize,
    OUT CHAR8 *NameBuffer,
    OPTIONAL IN OUT UINTN *NameBufferSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->X509_GET_ORGANIZATION_NAME == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return EFI_PROTOCOL_UNREACHABLE;
  }

  return prot->X509_GET_ORGANIZATION_NAME(Cert, CertSize, NameBuffer, NameBufferSize);
}

/**
  Verify one X509 certificate was issued by the trusted CA.

  If Cert is NULL, then return FALSE.
  If CACert is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate to be verified.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[in]      CACert       Pointer to the DER-encoded trusted CA certificate.
  @param[in]      CACertSize   Size of the CA Certificate in bytes.

  @retval  TRUE   The certificate was issued by the trusted CA.
  @retval  FALSE  Invalid certificate or the certificate was not issued by the given
                  trusted CA.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
X509VerifyCert (
    IN CONST UINT8 *Cert,
    IN UINTN CertSize,
    IN CONST UINT8 *CACert,
    IN UINTN CACertSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->MD4_Update == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->X509_VerifyCert(Cert, CertSize, CACert, CACertSize);
}

/**
  Construct a X509 object from DER-encoded certificate data.

  If Cert is NULL, then return FALSE.
  If SingleX509Cert is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Cert            Pointer to the DER-encoded certificate data.
  @param[in]  CertSize        The size of certificate data in bytes.
  @param[out] SingleX509Cert  The generated X509 object.

  @retval     TRUE            The X509 object generation succeeded.
  @retval     FALSE           The operation failed.
  @retval     FALSE           This interface is not supported.

**/
BOOLEAN
EFIAPI
X509ConstructCertificate (
    IN CONST UINT8 *Cert,
    IN UINTN CertSize,
    OUT UINT8 **SingleX509Cert)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->X509_ConstructCertificate == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->X509_ConstructCertificate(Cert, CertSize, SingleX509Cert);
}

/**
  Construct a X509 stack object from a list of DER-encoded certificate data.

  If X509Stack is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  X509Stack  On input, pointer to an existing or NULL X509 stack object.
                              On output, pointer to the X509 stack object with new
                              inserted X509 certificate.
  @param           ...        A list of DER-encoded single certificate data followed
                              by certificate size. A NULL terminates the list. The
                              pairs are the arguments to X509ConstructCertificate().

  @retval     TRUE            The X509 stack construction succeeded.
  @retval     FALSE           The construction operation failed.
  @retval     FALSE           This interface is not supported.

**/
BOOLEAN
EFIAPI
X509ConstructCertificateStack (
    IN OUT UINT8 **X509Stack,
    ...)
{
  ProtocolFunctionNotFound(__FUNCTION__);
  return FALSE;
  /*
  //This function doesn't work because we don't have a good way to pass the VA_LIST without a function that takes it directly
  //https://stackoverflow.com/questions/150543/forward-an-invocation-of-a-variadic-function-in-c
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol(); //FIX
  VA_LIST args;
  va_start(args, X509Stack);
  if (prot == NULL || prot->X509_ConstructCertificateStack == NULL) {

    return FALSE;
  }
  return prot->X509_ConstructCertificateStack(X509Stack, args);
  */
}

/**
  Release the specified X509 object.

  If the interface is not supported, then ASSERT().

  @param[in]  X509Cert  Pointer to the X509 object to be released.

**/
VOID
EFIAPI
X509Free (
  IN VOID *X509Cert
)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->X509_Free == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return;
  }
  prot->X509_Free(X509Cert);
}

/**
  Release the specified X509 stack object.

  If the interface is not supported, then ASSERT().

  @param[in]  X509Stack  Pointer to the X509 stack object to be released.

**/
VOID
EFIAPI
X509StackFree (
    IN VOID *X509Stack
 )
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->X509_StackFree == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
  }
  prot->X509_StackFree(X509Stack);
}

/**
  Retrieve the TBSCertificate from one given X.509 certificate.

  @param[in]      Cert         Pointer to the given DER-encoded X509 certificate.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[out]     TBSCert      DER-Encoded To-Be-Signed certificate.
  @param[out]     TBSCertSize  Size of the TBS certificate in bytes.

  If Cert is NULL, then return FALSE.
  If TBSCert is NULL, then return FALSE.
  If TBSCertSize is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @retval  TRUE   The TBSCertificate was retrieved successfully.
  @retval  FALSE  Invalid X.509 certificate.

**/
BOOLEAN
EFIAPI
X509GetTBSCert (
    IN CONST UINT8 *Cert,
    IN UINTN CertSize,
    OUT UINT8 **TBSCert,
    OUT UINTN *TBSCertSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->X509_GetTBSCert == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->X509_GetTBSCert(Cert, CertSize, TBSCert, TBSCertSize);
}

/**
  Derives a key from a password using a salt and iteration count, based on PKCS#5 v2.0
  password based encryption key derivation function PBKDF2, as specified in RFC 2898.

  If Password or Salt or OutKey is NULL, then return FALSE.
  If the hash algorithm could not be determined, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  PasswordLength  Length of input password in bytes.
  @param[in]  Password        Pointer to the array for the password.
  @param[in]  SaltLength      Size of the Salt in bytes.
  @param[in]  Salt            Pointer to the Salt.
  @param[in]  IterationCount  Number of iterations to perform. Its value should be
                              greater than or equal to 1.
  @param[in]  DigestSize      Size of the message digest to be used (eg. SHA256_DIGEST_SIZE).
                              NOTE: DigestSize will be used to determine the hash algorithm.
                                    Only SHA1_DIGEST_SIZE or SHA256_DIGEST_SIZE is supported.
  @param[in]  KeyLength       Size of the derived key buffer in bytes.
  @param[out] OutKey          Pointer to the output derived key buffer.

  @retval  TRUE   A key was derived successfully.
  @retval  FALSE  One of the pointers was NULL or one of the sizes was too large.
  @retval  FALSE  The hash algorithm could not be determined from the digest size.
  @retval  FALSE  The key derivation operation failed.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Pkcs5HashPassword (
    IN UINTN PasswordLength,
    IN CONST CHAR8 *Password,
    IN UINTN SaltLength,
    IN CONST UINT8 *Salt,
    IN UINTN IterationCount,
    IN UINTN DigestSize,
    IN UINTN KeyLength,
    OUT UINT8 *OutKey)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  EFI_STATUS Status;
  if (prot == NULL || prot->PKCS5_PW_HASH == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  Status = prot->PKCS5_PW_HASH(PasswordLength, Password, SaltLength, Salt, IterationCount, DigestSize, KeyLength, OutKey);
  if (EFI_ERROR(Status))
    return FALSE;
  return TRUE;
}

// MSChange [BEGIN] - Add a wrapper function for the PKCS1v2 RSAES-OAEP PKI encryption
//                    functionality provided by OpenSSL.
/**
  Encrypts a blob using PKCS1v2 (RSAES-OAEP) schema. On success, will return the encrypted message in
  in a newly allocated buffer.

  Things that can cause a failure include:
  - X509 key size does not match any known key size.
  - Fail to parse X509 certificate.
  - Fail to allocate an intermediate buffer.
  - NULL pointer provided for a non-optional parameter.
  - Data size is too large for the provided key size (max size is a function of key size and hash digest size).

  @param[in]  PublicKey     A pointer to the DER-encoded X509 certificate that will be used to encrypt the data.
  @param[in]  PublicKeySize Size of the X509 cert buffer.
  @param[in]  InData        Data to be encrypted.
  @param[in]  InDataSize    Size of the data buffer.
  @param[in]  PrngSeed      [Optional] If provided, a pointer to a random seed buffer to be used when initializing the PRNG. NULL otherwise.
  @param[in]  PrngSeedSize  [Optional] If provided, size of the random seed buffer. 0 otherwise.
  @param[out] EncryptedData       Pointer to an allocated buffer containing the encrypted message.
  @param[out] EncryptedDataSize   Size of the encrypted message buffer.

  @retval     TRUE  Encryption was successful.
  @retval     FALSE Encryption failed.

**/
BOOLEAN
Pkcs1v2Encrypt (
    IN CONST UINT8 *PublicKey,
    IN UINTN PublicKeySize,
    IN UINT8 *InData,
    IN UINTN InDataSize,
    IN CONST UINT8 *PrngSeed OPTIONAL,
    IN UINTN PrngSeedSize OPTIONAL,
    OUT UINT8 **EncryptedData,
    OUT UINTN *EncryptedDataSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->PKCS1_ENCRYPT_V2 == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->PKCS1_ENCRYPT_V2(PublicKey, PublicKeySize, InData, InDataSize, PrngSeed, PrngSeedSize, EncryptedData, EncryptedDataSize);
}

/**
  Get the signer's certificates from PKCS#7 signed data as described in "PKCS #7:
  Cryptographic Message Syntax Standard". The input signed data could be wrapped
  in a ContentInfo structure.

  If P7Data, CertStack, StackLength, TrustedCert or CertLength is NULL, then
  return FALSE. If P7Length overflow, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  P7Data       Pointer to the PKCS#7 message to verify.
  @param[in]  P7Length     Length of the PKCS#7 message in bytes.
  @param[out] CertStack    Pointer to Signer's certificates retrieved from P7Data.
                           It's caller's responsibility to free the buffer with
                           Pkcs7FreeSigners().
                           This data structure is EFI_CERT_STACK type.
  @param[out] StackLength  Length of signer's certificates in bytes.
  @param[out] TrustedCert  Pointer to a trusted certificate from Signer's certificates.
                           It's caller's responsibility to free the buffer with
                           Pkcs7FreeSigners().
  @param[out] CertLength   Length of the trusted certificate in bytes.

  @retval  TRUE            The operation is finished successfully.
  @retval  FALSE           Error occurs during the operation.
  @retval  FALSE           This interface is not supported.

**/
BOOLEAN
EFIAPI
Pkcs7GetSigners (
    IN CONST UINT8 *P7Data,
    IN UINTN P7Length,
    OUT UINT8 **CertStack,
    OUT UINTN *StackLength,
    OUT UINT8 **TrustedCert,
    OUT UINTN *CertLength)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->PKCS7_GetSigners == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->PKCS7_GetSigners(P7Data, P7Length, CertStack, StackLength, TrustedCert, CertLength);
}

/**
  Wrap function to use free() to free allocated memory for certificates.

  If this interface is not supported, then ASSERT().

  @param[in]  Certs        Pointer to the certificates to be freed.

**/
VOID
EFIAPI
Pkcs7FreeSigners (
  IN UINT8 *Certs
)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->PKCS7_FreeSigners == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
  }
  prot->PKCS7_FreeSigners(Certs);
}

/**
  Retrieves all embedded certificates from PKCS#7 signed data as described in "PKCS #7:
  Cryptographic Message Syntax Standard", and outputs two certificate lists chained and
  unchained to the signer's certificates.
  The input signed data could be wrapped in a ContentInfo structure.

  @param[in]  P7Data            Pointer to the PKCS#7 message.
  @param[in]  P7Length          Length of the PKCS#7 message in bytes.
  @param[out] SignerChainCerts  Pointer to the certificates list chained to signer's
                                certificate. It's caller's responsibility to free the buffer
                                with Pkcs7FreeSigners().
                                This data structure is EFI_CERT_STACK type.
  @param[out] ChainLength       Length of the chained certificates list buffer in bytes.
  @param[out] UnchainCerts      Pointer to the unchained certificates lists. It's caller's
                                responsibility to free the buffer with Pkcs7FreeSigners().
                                This data structure is EFI_CERT_STACK type.
  @param[out] UnchainLength     Length of the unchained certificates list buffer in bytes.

  @retval  TRUE         The operation is finished successfully.
  @retval  FALSE        Error occurs during the operation.

**/
BOOLEAN
EFIAPI
Pkcs7GetCertificatesList (
    IN CONST UINT8 *P7Data,
    IN UINTN P7Length,
    OUT UINT8 **SignerChainCerts,
    OUT UINTN *ChainLength,
    OUT UINT8 **UnchainCerts,
    OUT UINTN *UnchainLength)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->PKCS7_GetCertificatesList == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->PKCS7_GetCertificatesList(P7Data, P7Length, SignerChainCerts, ChainLength, UnchainCerts, UnchainLength);
}

/**
  Creates a PKCS#7 signedData as described in "PKCS #7: Cryptographic Message
  Syntax Standard, version 1.5". This interface is only intended to be used for
  application to perform PKCS#7 functionality validation.

  If this interface is not supported, then return FALSE.

  @param[in]  PrivateKey       Pointer to the PEM-formatted private key data for
                               data signing.
  @param[in]  PrivateKeySize   Size of the PEM private key data in bytes.
  @param[in]  KeyPassword      NULL-terminated passphrase used for encrypted PEM
                               key data.
  @param[in]  InData           Pointer to the content to be signed.
  @param[in]  InDataSize       Size of InData in bytes.
  @param[in]  SignCert         Pointer to signer's DER-encoded certificate to sign with.
  @param[in]  OtherCerts       Pointer to an optional additional set of certificates to
                               include in the PKCS#7 signedData (e.g. any intermediate
                               CAs in the chain).
  @param[out] SignedData       Pointer to output PKCS#7 signedData. It's caller's
                               responsibility to free the buffer with FreePool().
  @param[out] SignedDataSize   Size of SignedData in bytes.

  @retval     TRUE             PKCS#7 data signing succeeded.
  @retval     FALSE            PKCS#7 data signing failed.
  @retval     FALSE            This interface is not supported.

**/
BOOLEAN
EFIAPI
Pkcs7Sign (
    IN CONST UINT8 *PrivateKey,
    IN UINTN PrivateKeySize,
    IN CONST UINT8 *KeyPassword,
    IN UINT8 *InData,
    IN UINTN InDataSize,
    IN UINT8 *SignCert,
    IN UINT8 *OtherCerts OPTIONAL,
    OUT UINT8 **SignedData,
    OUT UINTN *SignedDataSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->PKCS7_Sign == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->PKCS7_Sign(PrivateKey, PrivateKeySize, KeyPassword, InData, InDataSize, SignCert, OtherCerts, SignedData, SignedDataSize);
}

/**
  Verifies the validity of a PKCS#7 signed data as described in "PKCS #7:
  Cryptographic Message Syntax Standard". The input signed data could be wrapped
  in a ContentInfo structure.

  If P7Data, TrustedCert or InData is NULL, then return FALSE.
  If P7Length, CertLength or DataLength overflow, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  P7Data       Pointer to the PKCS#7 message to verify.
  @param[in]  P7Length     Length of the PKCS#7 message in bytes.
  @param[in]  TrustedCert  Pointer to a trusted/root certificate encoded in DER, which
                           is used for certificate chain verification.
  @param[in]  CertLength   Length of the trusted certificate in bytes.
  @param[in]  InData       Pointer to the content to be verified.
  @param[in]  DataLength   Length of InData in bytes.

  @retval  TRUE  The specified PKCS#7 signed data is valid.
  @retval  FALSE Invalid PKCS#7 signed data.
  @retval  FALSE This interface is not supported.

**/
BOOLEAN
EFIAPI
Pkcs7Verify (
    IN CONST UINT8 *P7Data,
    IN UINTN P7Length,
    IN CONST UINT8 *TrustedCert,
    IN UINTN CertLength,
    IN CONST UINT8 *InData,
    IN UINTN DataLength)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->PKCS7_VERIFY == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->PKCS7_VERIFY(P7Data, P7Length, TrustedCert, CertLength, InData, DataLength);
}

// MS_CHANGE [BEGIN] - Add a function that will verify Extended Key Usages
//                     are present in the end-entity (leaf) signer certificate
//                     in a PKCS7 formatted signature.

/**
  VerifyEKUsInPkcs7Signature()

  This function receives a PKCS7 formatted signature, and then verifies that
  the specified Enhanced or Extended Key Usages (EKU's) are present in the end-entity
  leaf signing certificate.

  Note that this function does not validate the certificate chain.

  Applications for custom EKU's are quite flexible.  For example, a policy EKU
  may be present in an Issuing Certificate Authority (CA), and any sub-ordinate
  certificate issued might also contain this EKU, thus constraining the
  sub-ordinate certificate.  Other applications might allow a certificate
  embedded in a device to specify that other Object Identifiers (OIDs) are
  present which contains binary data specifying custom capabilities that
  the device is able to do.

  @param[in]  Pkcs7Signature     - The PKCS#7 signed information content block. An array
                                   containing the content block with both the signature,
                                   the signer's certificate, and any necessary intermediate
                                   certificates.

  @param[in]  Pkcs7SignatureSize - Number of bytes in Pkcs7Signature.

  @param[in]  RequiredEKUs       - Array of null-terminated strings listing OIDs of
                                   required EKUs that must be present in the signature.

  @param[in]  RequiredEKUsSize   - Number of elements in the RequiredEKUs string array.

  @param[in]  RequireAllPresent  - If this is TRUE, then all of the specified EKU's
                                   must be present in the leaf signer.  If it is
                                   FALSE, then we will succeed if we find any
                                   of the specified EKU's.

  @retval EFI_SUCCESS            - The required EKUs were found in the signature.
  @retval EFI_INVALID_PARAMETER  - A parameter was invalid.
  @retval EFI_NOT_FOUND          - One or more EKU's were not found in the signature.

**/
EFI_STATUS
EFIAPI
VerifyEKUsInPkcs7Signature (
    IN CONST UINT8 *Pkcs7Signature,
    IN CONST UINT32 SignatureSize,
    IN CONST CHAR8 *RequiredEKUs[],
    IN CONST UINT32 RequiredEKUsSize,
    IN BOOLEAN RequireAllPresent)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->PKCS7_VERIFY_EKU == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->PKCS7_VERIFY_EKU(Pkcs7Signature, SignatureSize, RequiredEKUs, RequiredEKUsSize, RequireAllPresent);
}

// MS_CHANGE [END]

/**
  Extracts the attached content from a PKCS#7 signed data if existed. The input signed
  data could be wrapped in a ContentInfo structure.

  If P7Data, Content, or ContentSize is NULL, then return FALSE. If P7Length overflow,
  then return FALSE. If the P7Data is not correctly formatted, then return FALSE.

  Caution: This function may receive untrusted input. So this function will do
           basic check for PKCS#7 data structure.

  @param[in]   P7Data       Pointer to the PKCS#7 signed data to process.
  @param[in]   P7Length     Length of the PKCS#7 signed data in bytes.
  @param[out]  Content      Pointer to the extracted content from the PKCS#7 signedData.
                            It's caller's responsibility to free the buffer with FreePool().
  @param[out]  ContentSize  The size of the extracted content in bytes.

  @retval     TRUE          The P7Data was correctly formatted for processing.
  @retval     FALSE         The P7Data was not correctly formatted for processing.

**/
BOOLEAN
EFIAPI
Pkcs7GetAttachedContent (
    IN CONST UINT8 *P7Data,
    IN UINTN P7Length,
    OUT VOID **Content,
    OUT UINTN *ContentSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->PKCS7_GetAttachedContent == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->PKCS7_GetAttachedContent(P7Data, P7Length, Content, ContentSize);
}

/**
  Verifies the validity of a PE/COFF Authenticode Signature as described in "Windows
  Authenticode Portable Executable Signature Format".

  If AuthData is NULL, then return FALSE.
  If ImageHash is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  AuthData     Pointer to the Authenticode Signature retrieved from signed
                           PE/COFF image to be verified.
  @param[in]  DataSize     Size of the Authenticode Signature in bytes.
  @param[in]  TrustedCert  Pointer to a trusted/root certificate encoded in DER, which
                           is used for certificate chain verification.
  @param[in]  CertSize     Size of the trusted certificate in bytes.
  @param[in]  ImageHash    Pointer to the original image file hash value. The procedure
                           for calculating the image hash value is described in Authenticode
                           specification.
  @param[in]  HashSize     Size of Image hash value in bytes.

  @retval  TRUE   The specified Authenticode Signature is valid.
  @retval  FALSE  Invalid Authenticode Signature.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
AuthenticodeVerify (
    IN CONST UINT8 *AuthData,
    IN UINTN DataSize,
    IN CONST UINT8 *TrustedCert,
    IN UINTN CertSize,
    IN CONST UINT8 *ImageHash,
    IN UINTN HashSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->Authenticode_Verify == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->Authenticode_Verify(AuthData, DataSize, TrustedCert, CertSize, ImageHash, HashSize);
}

/**
  Verifies the validity of a RFC3161 Timestamp CounterSignature embedded in PE/COFF Authenticode
  signature.

  If AuthData is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  AuthData     Pointer to the Authenticode Signature retrieved from signed
                           PE/COFF image to be verified.
  @param[in]  DataSize     Size of the Authenticode Signature in bytes.
  @param[in]  TsaCert      Pointer to a trusted/root TSA certificate encoded in DER, which
                           is used for TSA certificate chain verification.
  @param[in]  CertSize     Size of the trusted certificate in bytes.
  @param[out] SigningTime  Return the time of timestamp generation time if the timestamp
                           signature is valid.

  @retval  TRUE   The specified Authenticode includes a valid RFC3161 Timestamp CounterSignature.
  @retval  FALSE  No valid RFC3161 Timestamp CounterSignature in the specified Authenticode data.

**/
BOOLEAN
EFIAPI
ImageTimestampVerify (
    IN CONST UINT8 *AuthData,
    IN UINTN DataSize,
    IN CONST UINT8 *TsaCert,
    IN UINTN CertSize,
    OUT EFI_TIME *SigningTime)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->MD4_Update == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->Image_TimestampVerify(AuthData, DataSize, TsaCert, CertSize, SigningTime);
}

//=====================================================================================
//    DH Key Exchange Primitive
//=====================================================================================

/**
  Allocates and Initializes one Diffie-Hellman Context for subsequent use.

  @return  Pointer to the Diffie-Hellman Context that has been initialized.
           If the allocations fails, DhNew() returns NULL.
           If the interface is not supported, DhNew() returns NULL.

**/
VOID *
EFIAPI
DhNew (
  VOID
)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->DH_New == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return NULL;
  }
  return prot->DH_New();
}

/**
  Release the specified DH context.

  If the interface is not supported, then ASSERT().

  @param[in]  DhContext  Pointer to the DH context to be released.

**/
VOID
EFIAPI
DhFree (
  IN VOID *DhContext
)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->DH_Free == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
  }
  prot->DH_Free(DhContext);
}

/**
  Generates DH parameter.

  Given generator g, and length of prime number p in bits, this function generates p,
  and sets DH context according to value of g and p.

  Before this function can be invoked, pseudorandom number generator must be correctly
  initialized by RandomSeed().

  If DhContext is NULL, then return FALSE.
  If Prime is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  DhContext    Pointer to the DH context.
  @param[in]       Generator    Value of generator.
  @param[in]       PrimeLength  Length in bits of prime to be generated.
  @param[out]      Prime        Pointer to the buffer to receive the generated prime number.

  @retval TRUE   DH parameter generation succeeded.
  @retval FALSE  Value of Generator is not supported.
  @retval FALSE  PRNG fails to generate random prime number with PrimeLength.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DhGenerateParameter (
    IN OUT VOID *DhContext,
    IN UINTN Generator,
    IN UINTN PrimeLength,
    OUT UINT8 *Prime)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->DH_GenerateParameter == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->DH_GenerateParameter(DhContext, Generator, PrimeLength, Prime);
}

/**
  Sets generator and prime parameters for DH.

  Given generator g, and prime number p, this function and sets DH
  context accordingly.

  If DhContext is NULL, then return FALSE.
  If Prime is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  DhContext    Pointer to the DH context.
  @param[in]       Generator    Value of generator.
  @param[in]       PrimeLength  Length in bits of prime to be generated.
  @param[in]       Prime        Pointer to the prime number.

  @retval TRUE   DH parameter setting succeeded.
  @retval FALSE  Value of Generator is not supported.
  @retval FALSE  Value of Generator is not suitable for the Prime.
  @retval FALSE  Value of Prime is not a prime number.
  @retval FALSE  Value of Prime is not a safe prime number.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DhSetParameter (
  IN OUT VOID *DhContext,
  IN UINTN Generator,
  IN UINTN PrimeLength,
  IN CONST UINT8 *Prime
)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->DH_SetParameter == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->DH_SetParameter(DhContext, Generator, PrimeLength, Prime);
}

/**
  Generates DH public key.

  This function generates random secret exponent, and computes the public key, which is
  returned via parameter PublicKey and PublicKeySize. DH context is updated accordingly.
  If the PublicKey buffer is too small to hold the public key, FALSE is returned and
  PublicKeySize is set to the required buffer size to obtain the public key.

  If DhContext is NULL, then return FALSE.
  If PublicKeySize is NULL, then return FALSE.
  If PublicKeySize is large enough but PublicKey is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  DhContext      Pointer to the DH context.
  @param[out]      PublicKey      Pointer to the buffer to receive generated public key.
  @param[in, out]  PublicKeySize  On input, the size of PublicKey buffer in bytes.
                                 On output, the size of data returned in PublicKey buffer in bytes.

  @retval TRUE   DH public key generation succeeded.
  @retval FALSE  DH public key generation failed.
  @retval FALSE  PublicKeySize is not large enough.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DhGenerateKey (
  IN OUT VOID *DhContext,
  OUT UINT8 *PublicKey,
  IN OUT UINTN *PublicKeySize
)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->DH_GenerateKey == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->DH_GenerateKey(DhContext, PublicKey, PublicKeySize);
}

/**
  Computes exchanged common key.

  Given peer's public key, this function computes the exchanged common key, based on its own
  context including value of prime modulus and random secret exponent.

  If DhContext is NULL, then return FALSE.
  If PeerPublicKey is NULL, then return FALSE.
  If KeySize is NULL, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeySize is not large enough, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  DhContext          Pointer to the DH context.
  @param[in]       PeerPublicKey      Pointer to the peer's public key.
  @param[in]       PeerPublicKeySize  Size of peer's public key in bytes.
  @param[out]      Key                Pointer to the buffer to receive generated key.
  @param[in, out]  KeySize            On input, the size of Key buffer in bytes.
                                     On output, the size of data returned in Key buffer in bytes.

  @retval TRUE   DH exchanged key generation succeeded.
  @retval FALSE  DH exchanged key generation failed.
  @retval FALSE  KeySize is not large enough.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DhComputeKey (
    IN OUT VOID *DhContext,
    IN CONST UINT8 *PeerPublicKey,
    IN UINTN PeerPublicKeySize,
    OUT UINT8 *Key,
    IN OUT UINTN *KeySize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->DH_ComputeKey == NULL) {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }
  return prot->DH_ComputeKey(DhContext, PeerPublicKey, PeerPublicKeySize, Key, KeySize);
}

//=====================================================================================
//    Pseudo-Random Generation Primitive
//=====================================================================================

/**
  Sets up the seed value for the pseudorandom number generator.

  This function sets up the seed value for the pseudorandom number generator.
  If Seed is not NULL, then the seed passed in is used.
  If Seed is NULL, then default seed is used.
  If this interface is not supported, then return FALSE.

  @param[in]  Seed      Pointer to seed value.
                        If NULL, default seed is used.
  @param[in]  SeedSize  Size of seed value.
                        If Seed is NULL, this parameter is ignored.

  @retval TRUE   Pseudorandom number generator has enough entropy for random generation.
  @retval FALSE  Pseudorandom number generator does not have enough entropy for random generation.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
RandomSeed (
    IN CONST UINT8 *Seed OPTIONAL,
    IN UINTN SeedSize)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->RANDOM_Seed == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->RANDOM_Seed(Seed, SeedSize);
}

/**
  Generates a pseudorandom byte stream of the specified size.

  If Output is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  Output  Pointer to buffer to receive random value.
  @param[in]   Size    Size of random bytes to generate.

  @retval TRUE   Pseudorandom byte stream generated successfully.
  @retval FALSE  Pseudorandom number generator fails to generate due to lack of entropy.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
RandomBytes (
    OUT UINT8 *Output,
    IN UINTN Size)
{
  SHARED_CRYPTO_FUNCTIONS *prot = GetProtocol();
  if (prot == NULL || prot->RANDOM_Bytes == NULL)
  {
    ProtocolFunctionNotFound(__FUNCTION__);
    return FALSE;
  }

  return prot->RANDOM_Bytes(Output, Size);
}