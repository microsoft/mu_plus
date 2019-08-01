
/** @file
  This function returns the version number that the shared crypto driver is
  This is compared against the library and if they do not match, the library asserts

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Protocol/SharedCrypto.h>

UINTN
EFIAPI
GetCryptoVersion (
  VOID
)
{
  return SHARED_CRYPTO_VERSION;
}