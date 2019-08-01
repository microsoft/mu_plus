
/** @file
  This module is consumed by both DXE and SMM as well as PEI

Copyright (C) Microsoft Corporation. All rights reserved.. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Protocol/SharedCrypto.h>

#ifndef __SHARED_CRYPTO_VERSION_H__
#define __SHARED_CRYPTO_VERSION_H__

UINTN
EFIAPI GetCryptoVersion (
  VOID
);

#endif