/** @file
SharedCryptoLibPei.c

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseCryptLib.h>
#include <Ppi/SharedCryptoPpi.h>
#include <SharedCryptoHelpers.h>

SHARED_CRYPTO_FUNCTIONS *GetProtocol()
{
  EFI_STATUS Status;
  UINTN Version;
  SHARED_CRYPTO_PPI *pCryptoProt = NULL;
  
  Status = PeiServicesLocatePpi(
      &gSharedCryptoPpiGuid,
      0,
      NULL,
      (VOID **)&pCryptoProt);
  if (EFI_ERROR(Status))
  {
    ProtocolNotFound(Status);
    pCryptoProt = NULL;
  }
  else {
    Version = pCryptoProt->GetVersion();
    if (Version != SHARED_CRYPTO_VERSION)
    {
      DEBUG((DEBUG_ERROR, "[SharedCryptoLibrary_PEI] Version mismatch. Version doesn't match expected %d. Current Version: %d\n", SHARED_CRYPTO_VERSION, Version));
      ProtocolNotFound(EFI_PROTOCOL_ERROR);
      pCryptoProt = NULL;
    }
  }

  return pCryptoProt;
}

VOID ProtocolNotFound (
  EFI_STATUS Status
)
{
  DEBUG((DEBUG_ERROR, "[SharedCryptoLibrary_PEI] Failed to locate Crypto Support Protocol. Status = %r\n", Status));
  ASSERT_EFI_ERROR(Status);
}

VOID ProtocolFunctionNotFound (CONST CHAR8* function_name)
{
  DEBUG((DEBUG_ERROR, "[SharedCryptoLibrary_PEI] This function was not found: %a\n",function_name));
  ASSERT_EFI_ERROR(EFI_UNSUPPORTED);
}
