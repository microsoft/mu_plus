/** @file
SharedCryptoLibSmm.c

Copyright (c) 2019, Microsoft Corporation

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
      Version = pCryptoProtocol->SharedCrypto_GetLowestSupportedVersion();
      if (Version != SHARED_CRYPTO_VERSION)
      {
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
VOID ProtocolFunctionNotFound (CHAR8* function_name)
{
  DEBUG((DEBUG_ERROR, "[SharedCryptoLibrary_SMM] This function was not found: %a\n",function_name));
  ASSERT_EFI_ERROR(EFI_UNSUPPORTED);
}