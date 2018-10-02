/** @file
Pkcs5Support.c

This module provides the support for PKCS5.

Copyright (c) 2018, Microsoft Corporation

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

#include <PiDxe.h>

#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DfciPkcs5PasswordHash.h>


DFCI_PKCS5_PASSWORD_HASH_PROTOCOL mPkcs5PwHashProtocol;


/**
Pkcs5 wrapper function - This is basically a pass thru to the BaseCryptLib .  


**/
EFI_STATUS
EFIAPI
HashUsingPkcs5(
  IN CONST  DFCI_PKCS5_PASSWORD_HASH_PROTOCOL   *This,
  IN        UINTN                                     PasswordSize,
  IN CONST  CHAR8                                     *Password,
  IN        UINTN                                     SaltSize,
  IN CONST  UINT8                                     *Salt,
  IN        UINTN                                     IterationCount,
  IN        UINTN                                     DigestSize,
  IN        UINTN                                     OutputSize,
  OUT       UINT8                                     *Output
  )
{
  if (This != &mPkcs5PwHashProtocol)
  {
    DEBUG((DEBUG_ERROR, "%a - Invalid This pointer\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  return Pkcs5HashPassword(     PasswordSize,       // Size
                                Password,           // Password
                                SaltSize,           // SaltSize
                                Salt,               // Salt
                                IterationCount,     // IterationCount
                                DigestSize,         // DigestSize
                                OutputSize,         // OutputSize
                                Output );     
}




/**
Function to install Pkcs5 Protocol for other drivers to use

**/
EFI_STATUS
EFIAPI
InstallPkcs5Support(
IN EFI_HANDLE          ImageHandle
)
{
  mPkcs5PwHashProtocol.HashPassword = HashUsingPkcs5;

  return gBS->InstallMultipleProtocolInterfaces(
    &ImageHandle,
    &gDfciPKCS5PasswordHashProtocolGuid,
    &mPkcs5PwHashProtocol,
    NULL
    );
}

/**
Function to uninstall Pkcs5 Protocol

**/
EFI_STATUS
EFIAPI
UninstallPkcs5Support(
IN EFI_HANDLE          ImageHandle
)
{
  return EFI_SUCCESS;
}