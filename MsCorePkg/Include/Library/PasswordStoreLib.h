/** @file -- PasswordStoreLib.h

  Interfaces to the password store.

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PASSWORD_STORE_LIB_H_
#define _PASSWORD_STORE_LIB_H_

/**
  Public interface for determining whether the admin password is set.

  NOTE: Will initialize the Password Store if it doesn't exist

  @retval     TRUE    Password is set.
  @retval     FALSE   Password is not set, or an error occurred preventing
                      the check from completing successfully.

**/
BOOLEAN
EFIAPI
PasswordStoreIsPasswordSet (
  VOID
  );

/**
  Set the password variable.

  This is public for a UiApp that doesn't use SettingsAccessProtocol
  to set the password.  When UEFI is configured to use the SettingsAccessProtocol to
  set the password, only the PasswordProvider should call SetPassword.

  @param[in]  PasswordHash        Pointer to the password hash
  @param[in]  PasswordHashSize    Size of the password hash

  @retval     EFI_SUCCESS   Password stored successfully.
  @retval     Others        Something went wrong. Investigate further.
**/
EFI_STATUS
EFIAPI
PasswordStoreSetPassword (
  IN  CONST UINT8  *PasswordHash,
  IN        UINTN  PasswordHashSize
  );

/**
  Public interface for validating a password against the current password.

  If no password is currently set, will return FALSE.

  NOTE: This function does NOT perform string validation on the password
        being authenticated. This is to accommodate changing valid character sets.
        Will still make sure that string does not exceed max buffer size.

  @param[in]  Password  String being evaluated.

  @retval     TRUE      Password matches the stored password for Handle.
  @retval     TRUE      No password is currently set.
  @retval     TRUE      Password is NULL and Handle has previously authenticated successfully.
  @retval     FALSE     No Password is set for Handle.
  @retval     FALSE     Supplied Password does not match stored password for Handle.

**/
BOOLEAN
EFIAPI
PasswordStoreAuthenticatePassword (
  IN  CONST CHAR16  *Password
  );

#endif // _PASSWORD_STORE_LIB_H_
