/** @file
  Minimal image used by the Secure Boot image-verification audit.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/UefiLib.h>

EFI_STATUS
EFIAPI
HelloUefiAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  Print (L"Hello from the ML-DSA Secure Boot audit\r\n");
  return EFI_SUCCESS;
}
