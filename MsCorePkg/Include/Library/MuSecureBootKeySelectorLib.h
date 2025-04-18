/** @file MuSecureBootKeySelectorLib.h

  This header file provides functions to interact with platform supplied
  secure boot related keys through SecureBootKeyStoreLib.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MU_SB_KEY_SELECTOR_LIB_H_
#define MU_SB_KEY_SELECTOR_LIB_H_

#define MU_SB_CONFIG_UNKNOWN  MAX_UINT8 - 1 // This means the key does not match anything from key store
#define MU_SB_CONFIG_NONE     MAX_UINT8

/**
  Query the index of the actively used Secure Boot keys corresponds to the Secure Boot key store, if it
  can be determined.

  @retval     UINTN   Will return an index of key store or MU_SB_CONFIG_NONE if secure boot is not enabled,
                      or MU_SB_CONFIG_UNKOWN if the active key does not match anything in the key store.

**/
UINTN
EFIAPI
GetCurrentSecureBootConfig (
  VOID
  );

/**
  Returns the status of setting secure boot keys.

  @param  [in] Index  The index of key from key stores.

  @retval Will return the status of setting secure boot variables.

**/
EFI_STATUS
EFIAPI
SetSecureBootConfig (
  IN  UINT8  Index
  );

#endif //MU_SB_KEY_SELECTOR_LIB_H_
