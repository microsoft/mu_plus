/** @file
SharedCryptoLibDxe.c

The version of the SharedCrypto wrapper Library for DXE

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <SharedCryptoHelpers.h>
#include <Protocol/SharedCryptoProtocol.h>

SHARED_CRYPTO_PROTOCOL* pCryptoProtocol = NULL;

SHARED_CRYPTO_FUNCTIONS* GetProtocol ()
{
  EFI_STATUS Status;
  UINTN Version;
  if (pCryptoProtocol == NULL)
  {
    Status = gBS->LocateProtocol(&gSharedCryptoProtocolGuid, NULL, (VOID **)&pCryptoProtocol);
    if (EFI_ERROR(Status))
    {
      ProtocolNotFound(Status);
    }
    else {
      Version = pCryptoProtocol->SharedCrypto_GetLowestSupportedVersion();
      if (Version != SHARED_CRYPTO_VERSION)
      {
        DEBUG((DEBUG_ERROR, "[SharedCryptoLibrary_DXE] Failed to locate Support Protocol. Version doesn't match expected %d. Current Version: %d\n", SHARED_CRYPTO_VERSION, Version));
        ProtocolNotFound(EFI_PROTOCOL_ERROR);
      }
    }
  }
  return pCryptoProtocol;
}
VOID ProtocolNotFound (
  EFI_STATUS Status
)
{
  DEBUG((DEBUG_ERROR, "[SharedCryptoLibrary_DXE] Failed to locate Support Protocol. Status = %r\n", Status));
  ASSERT_EFI_ERROR(Status);
  pCryptoProtocol = NULL;
}

VOID ProtocolFunctionNotFound (CONST CHAR8* function_name)
{
  DEBUG((DEBUG_ERROR, "[SharedCryptoLibrary_DXE] This function was not found: %a\n",function_name));
  ASSERT_EFI_ERROR(EFI_UNSUPPORTED);
}