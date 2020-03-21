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

/**
  A macro used to call a non-void service in an EDK II Crypto Protocol.
  If the protocol is NULL or the service in the protocol is NULL, then a debug
  message and assert is generated and an appropriate return value is returned.

  @param  Function          Name of the EDK II Crypto Protocol service to call.
  @param  Args              The argument list to pass to Function.
  @param  ErrorReturnValue  The value to return if the protocol is NULL or the
                            service in the protocol is NULL.

**/
#define CALL_CRYPTO_SERVICE(Function, Args, ErrorReturnValue)          \
  do {                                                                 \
    SHARED_CRYPTO_FUNCTIONS  *CryptoServices;                          \
                                                                       \
    CryptoServices = (SHARED_CRYPTO_FUNCTIONS *)GetProtocol ();        \
    if (CryptoServices == NULL) {                                      \
        ProtocolNotFound (EFI_NOT_FOUND);                              \
        return ErrorReturnValue;                                       \
    }                                                                  \
    if (CryptoServices != NULL && CryptoServices->Function != NULL) {  \
      return (CryptoServices->Function) Args;                          \
    }                                                                  \
    ProtocolFunctionNotFound (#Function);                              \
    return ErrorReturnValue;                                           \
  } while (FALSE)

/**
  A macro used to call a void service in an EDK II Crypto Protocol.
  If the protocol is NULL or the service in the protocol is NULL, then a debug
  message and assert is generated.

  @param  Function          Name of the EDK II Crypto Protocol service to call.
  @param  Args              The argument list to pass to Function.

**/
#define CALL_VOID_CRYPTO_SERVICE(Function, Args)                       \
  do {                                                                 \
    SHARED_CRYPTO_FUNCTIONS  *CryptoServices;                          \
                                                                       \
    CryptoServices = (SHARED_CRYPTO_FUNCTIONS *)GetProtocol ();        \
    if (CryptoServices == NULL) {                                      \
        ProtocolNotFound (EFI_NOT_FOUND);                              \
        return;                                                        \
    }                                                                  \
    if (CryptoServices != NULL && CryptoServices->Function != NULL) {  \
      (CryptoServices->Function) Args;                                 \
      return;                                                          \
    }                                                                  \
    ProtocolFunctionNotFound (#Function);                              \
    return;                                                            \
  } while (FALSE)

#endif