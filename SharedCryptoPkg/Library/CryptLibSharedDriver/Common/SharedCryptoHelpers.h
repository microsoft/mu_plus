/** @file
  Device Boot Manager library definition. A device can implement
  instances to support device specific behavior.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CRYPTO_BIN_LIB_HELPERS_H_
#define __CRYPTO_BIN_LIB_HELPERS_H_


#include <Protocol/SharedCrypto.h>

SHARED_CRYPTO_FUNCTIONS* GetProtocol();

// Called when the protocol cannot be found
VOID ProtocolNotFound (
  EFI_STATUS Status
);

//Called when a particular function cannot be called
VOID ProtocolFunctionNotFound(CONST CHAR8*);


#endif