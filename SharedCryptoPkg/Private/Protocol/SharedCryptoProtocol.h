/** @file
This Protocol is for Shared Crypto (DXE, SMM)

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Protocol/SharedCrypto.h>

#ifndef __SHARED_CRYPTO_PROTOCOL_H__
#define __SHARED_CRYPTO_PROTOCOL_H__

typedef struct _SHARED_CRYPTO_FUNCTIONS SHARED_CRYPTO_PROTOCOL;

extern GUID gSharedCryptoSmmProtocolGuid;
extern GUID gSharedCryptoProtocolGuid;

#endif
