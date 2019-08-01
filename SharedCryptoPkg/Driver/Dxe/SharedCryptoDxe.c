/** @file
  This module installs Crypto protocol used by Project Mu Firmware

  Specifically, it installs the DXE protocol listed in the SharedCryptoPkg.dec

  See Readme.md in the root of SharedCryptoPkg for more information

Copyright (C) Microsoft Corporation. All rights reserved.. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SharedCryptoProtocol.h>

extern SHARED_CRYPTO_FUNCTIONS mSharedCryptoFunctions;

/**
  The module Entry Point of the Project Shared Crypto Dxe Driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
SharedCryptoDxeEntry(
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  return gBS->InstallMultipleProtocolInterfaces(
    &ImageHandle,
    &gSharedCryptoProtocolGuid,
    (SHARED_CRYPTO_PROTOCOL*) &mSharedCryptoFunctions,
    NULL
  );
}