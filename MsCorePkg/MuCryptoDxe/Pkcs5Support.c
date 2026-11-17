/**
 *
Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

*/
#include <PiDxe.h>

#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/MuPkcs5PasswordHash.h>

MU_PKCS5_PASSWORD_HASH_PROTOCOL  mPkcs5PwHashProtocol;

/**
Pkcs5 wrapper function - This is basically a pass thru to the BaseCryptLib .

  @param[in]  This            Pointer to the MU_PKCS5_PASSWORD_HASH_PROTOCOL instance.
  @param[in]  PasswordSize    Size of input password in # of bytes.
  @param[in]  Password        Pointer to the char array for the password.
  @param[in]  SaltSize        Size of the SALT in bytes.
  @param[in]  Salt            Pointer to the SALT.
  @param[in]  IterationCount  Number of iterations to perform (work factor).
  @param[in]  DigestSize      Size of the hash digest to be used (eg. SHA256_DIGEST_SIZE).
  @param[in]  OutputSize      # of bytes to output.
  @param[out] Output          Pointer to the output buffer.

  @retval     EFI_SUCCESS             The hash is in the output buffer.
  @retval     EFI_INVALID_PARAMETER   This is not the installed protocol instance.
  @retval     EFI_ABORTED             Pkcs5HashPassword() failed.

**/
EFI_STATUS
EFIAPI
HashUsingPkcs5 (
  IN CONST  MU_PKCS5_PASSWORD_HASH_PROTOCOL  *This,
  IN        UINTN                            PasswordSize,
  IN CONST  CHAR8                            *Password,
  IN        UINTN                            SaltSize,
  IN CONST  UINT8                            *Salt,
  IN        UINTN                            IterationCount,
  IN        UINTN                            DigestSize,
  IN        UINTN                            OutputSize,
  OUT       UINT8                            *Output
  )
{
  BOOLEAN  HashSuccess;

  if (This != &mPkcs5PwHashProtocol) {
    DEBUG ((DEBUG_ERROR, "%a - Invalid This pointer\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  HashSuccess = Pkcs5HashPassword (
                  PasswordSize,              // Size
                  Password,                  // Password
                  SaltSize,                  // SaltSize
                  Salt,                      // Salt
                  IterationCount,            // IterationCount
                  DigestSize,                // DigestSize
                  OutputSize,                // OutputSize
                  Output
                  );
  if (!HashSuccess) {
    DEBUG ((DEBUG_ERROR, "%a - Pkcs5HashPassword failed\n", __func__));
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/**
Function to install Pkcs5 Protocol for other drivers to use

**/
EFI_STATUS
EFIAPI
InstallPkcs5Support (
  IN EFI_HANDLE  ImageHandle
  )
{
  mPkcs5PwHashProtocol.HashPassword = HashUsingPkcs5;

  return gBS->InstallMultipleProtocolInterfaces (
                &ImageHandle,
                &gMuPKCS5PasswordHashProtocolGuid,
                &mPkcs5PwHashProtocol,
                NULL
                );
}

/**
Function to uninstall Pkcs5 Protocol

**/
EFI_STATUS
EFIAPI
UninstallPkcs5Support (
  IN EFI_HANDLE  ImageHandle
  )
{
  return EFI_SUCCESS;
}
