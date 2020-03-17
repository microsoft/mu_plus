/** @file
SharedCryptoLibSmm.c

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>
#include "Common/SharedCryptoHelpers.h"
#include <Protocol/SharedCryptoProtocol.h>

SHARED_CRYPTO_PROTOCOL* pCryptoProtocol = NULL;

SHARED_CRYPTO_FUNCTIONS *GetProtocol()
{
  EFI_STATUS Status;
  UINTN Version;
  if (pCryptoProtocol == NULL)
  {
    Status = gSmst->SmmLocateProtocol(&gSharedCryptoSmmProtocolGuid, NULL, (VOID **)&pCryptoProtocol);
    if (EFI_ERROR(Status))
    {
      ProtocolNotFound(Status);
    }
    else {
      Version = pCryptoProtocol->GetVersion();
      if (Version != SHARED_CRYPTO_VERSION)
      {
        DEBUG((DEBUG_ERROR, "[SharedCryptoLibrary_SMM] Failed to locate Support Protocol. Version doesn't match expected %d. Current Version: %d\n", SHARED_CRYPTO_VERSION, Version));
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
  DEBUG((DEBUG_ERROR, "[SharedCryptoLibrary_SMM] Failed to locate Support Protocol. Status = %r\n", Status));
  ASSERT_EFI_ERROR(Status);
  pCryptoProtocol = NULL;
}
VOID ProtocolFunctionNotFound (CONST CHAR8* function_name)
{
  DEBUG((DEBUG_ERROR, "[SharedCryptoLibrary_SMM] This function was not found: %a\n",function_name));
  ASSERT_EFI_ERROR(EFI_UNSUPPORTED);
}

/**
  Constructor looks up the EDK II SMM Crypto Protocol and verifies that it is
  not NULL and has a high enough version value to support all the BaseCryptLib
  functions.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval  EFI_SUCCESS    The EDK II SMM Crypto Protocol was found.
  @retval  EFI_NOT_FOUND  The EDK II SMM Crypto Protocol was not found.
**/
EFI_STATUS
EFIAPI
SmmCryptLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Version;

  Status = gSmst->SmmLocateProtocol (
                    &gSharedCryptoSmmProtocolGuid,
                    NULL,
                    (VOID **)&pCryptoProtocol
                    );
  if (EFI_ERROR (Status) || pCryptoProtocol == NULL) {
    DEBUG((DEBUG_ERROR, "[SmmCryptLib] Failed to locate Crypto SMM Protocol. Status = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    ASSERT (pCryptoProtocol != NULL);
    pCryptoProtocol = NULL;
    return EFI_NOT_FOUND;
  }

  Version = pCryptoProtocol->GetVersion ();
  if (Version < SHARED_CRYPTO_VERSION) {
    DEBUG((DEBUG_ERROR, "[SmmCryptLib] Crypto SMM Protocol unsupported version %d\n", Version));
    ASSERT (Version >= SHARED_CRYPTO_VERSION);
    pCryptoProtocol = NULL;
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
