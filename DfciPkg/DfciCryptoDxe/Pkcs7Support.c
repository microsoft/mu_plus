/** @file
Pkcs7Support.c

This module provides the support for PKCS7.

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

#include <Protocol/DfciPkcs7.h>


DFCI_PKCS7_PROTOCOL mPkcsProt;


/**
Pkcs7 Verify function - This is basically a pass thru to the BaseCryptLib .  


**/
EFI_STATUS
EFIAPI
VerifyFunc
(
IN  CONST DFCI_PKCS7_PROTOCOL     *This,
IN  CONST UINT8                         *P7Data,
IN  UINTN                               P7DataLength,
IN  CONST UINT8                         *TrustedCert,
IN  UINTN                               TrustedCertLength,
IN  CONST UINT8                         *Data,
IN  UINTN                               DataLength
)
{
  if (This != &mPkcsProt)
  {
    DEBUG((DEBUG_ERROR, "%a - Invalid This pointer\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  if ((P7Data == NULL) || (TrustedCert == NULL) || (Data == NULL))
  {
    DEBUG((DEBUG_ERROR, "%a - Invalid input parameter.  Pointer can not be NULL\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  if (Pkcs7Verify(P7Data, P7DataLength, TrustedCert, TrustedCertLength, Data, DataLength))
  {
    DEBUG((DEBUG_INFO, "%a - Data was validated successfully.\n", __FUNCTION__));
    return EFI_SUCCESS;
  } 

  DEBUG((DEBUG_INFO, "%a - Data did not validate.\n", __FUNCTION__));
  return EFI_SECURITY_VIOLATION;
}




/**
Function to install Pkcs7 Protocol for other drivers to use

**/
EFI_STATUS
EFIAPI
InstallPkcs7Support(
IN EFI_HANDLE          ImageHandle
)
{
  mPkcsProt.Verify = VerifyFunc;

  return gBS->InstallMultipleProtocolInterfaces(
    &ImageHandle,
    &gDfciPKCS7ProtocolGuid,
    &mPkcsProt,
    NULL
    );
}

/**
Function to uninstall Pkcs7 Protocol

**/
EFI_STATUS
EFIAPI
UninstallPkcs7Support(
IN EFI_HANDLE          ImageHandle
)
{
  return gBS->UninstallMultipleProtocolInterfaces(
    ImageHandle,
    &gDfciPKCS7ProtocolGuid,
    &mPkcsProt,
    NULL
    );

}