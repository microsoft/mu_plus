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

@param This             Pointer to the MU_PKCS5_PASSWORD_HASH_PROTOCOL instance.
@param PasswordSize     Size of the password in bytes.
@param Password         Pointer to the password.
@param SaltSize         Size of the salt in bytes.
@param Salt             Pointer to the salt.
@param IterationCount   Number of iterations for the hash.
@param DigestSize       Size of the digest in bytes.
@param OutputSize       Size of the output buffer in bytes.
@param Output           Pointer to the output buffer.

@retval EFI_SUCCESS             The password was hashed successfully.
@retval EFI_INVALID_PARAMETER   The This pointer is invalid.
@retval EFI_ABORTED             The hashing operation failed.


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
  if (This != &mPkcs5PwHashProtocol) {
    DEBUG ((DEBUG_ERROR, "%a - Invalid This pointer\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  return Pkcs5HashPassword (
           PasswordSize,                            // Size
           Password,                                // Password
           SaltSize,                                // SaltSize
           Salt,                                    // Salt
           IterationCount,                          // IterationCount
           DigestSize,                              // DigestSize
           OutputSize,                              // OutputSize
           Output
           )? EFI_SUCCESS : EFI_ABORTED;
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
