/** @file
  This module installs Crypto protocols used by Project Mu Firmware

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Protocol/SharedCryptoProtocol.h>
#include <Library/SmmServicesTableLib.h>

extern const SHARED_CRYPTO_FUNCTIONS mSharedCryptoFunctions;

/**
  The module Entry Point of the Project Shared Crypto Dxe Driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
SharedCryptoSmmEntry(
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
)
{
  EFI_HANDLE     Handle = NULL;
  return gSmst->SmmInstallProtocolInterface (
      &Handle,
      &gSharedCryptoSmmProtocolGuid,
      EFI_NATIVE_INTERFACE,
      (SHARED_CRYPTO_PROTOCOL*)  &mSharedCryptoFunctions);

}