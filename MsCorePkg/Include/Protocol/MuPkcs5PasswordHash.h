/** @file
This driver supports Pkcs 5 crypto operations


Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MU_PKCS5_PASSWORD_HASH_H__
#define __MU_PKCS5_PASSWORD_HASH_H__

typedef struct _MU_PKCS5_PASSWORD_HASH_PROTOCOL  MU_PKCS5_PASSWORD_HASH_PROTOCOL;


/**
  Uses PKCS5 Defined algorithm to Hash a password string

  @param[in]  PasswordSize    Size of input password in # of bytes.
  @param[in]  Password        Pointer to the char array for the password.
  @param[in]  SaltSize        Size of the SALT in bytes.
  @param[in]  Salt            Pointer to the SALT.
  @param[in]  IterationCount  Number of iterations to perform (work factor).
  @param[in]  DigestSize      Size of the hash digest to be used (eg. SHA256_DIGEST_SIZE).
                              NOTE: DigestSize will be used to determine the hash algorithm
                                    and must correspond to a known hash digest size. Use standards.
  @param[in]  OutputSize      # of bytes to output.
  @param[out] Output          Pointer to the output buffer.

  @retval     EFI_SUCCESS             Congratulations! Your hash is in the output buffer.
  @retval     EFI_INVALID_PARAMETER   One of the pointers was NULL or one of the sizes was too large.
  @retval     EFI_INVALID_PARAMETER   The hash algorithm could not be determined from the digest size.
  @retval     EFI_ABORTED             An error occurred in the OpenSSL subroutines.
**/
typedef
EFI_STATUS
(EFIAPI *MU_PKCS5_PW_HASH) (
  IN CONST MU_PKCS5_PASSWORD_HASH_PROTOCOL   *This,   
  IN UINTN                                      PasswordSize,
  IN CONST  CHAR8                              *Password,
  IN UINTN                                      SaltSize,
  IN CONST  UINT8                              *Salt,
  IN UINTN                                      IterationCount,
  IN UINTN                                      DigestSize,
  IN UINTN                                      OutputSize,
  OUT UINT8                                    *Output
  );

///
/// PKCS5 protocol
///
struct _MU_PKCS5_PASSWORD_HASH_PROTOCOL
{
  MU_PKCS5_PW_HASH HashPassword;
};

extern EFI_GUID gMuPKCS5PasswordHashProtocolGuid;

#endif // __MU_PKCS5_PW_HASH_H__
