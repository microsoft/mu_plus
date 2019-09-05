/** @file PasswordStoreLibNull.c

Provides the hashing algorithms for password hashes.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/PasswordStoreLib.h>
/**
  Set the password variable.

  @param[in]  PasswordHash        Pointer to the password hash
  @param[in]  PasswordHashSize    Size of the password hash

  @retval     EFI_SUCCESS   Password stored successfully.
  @retval     Others        Something went wrong. Investigate further.
**/
EFI_STATUS
EFIAPI
PasswordStoreSetPassword (
  IN  CONST UINT8     *PasswordHashValue,
  IN        UINTN      PasswordHashSize
)
{
  return EFI_UNSUPPORTED;
}


/**
  Public interface for determining whether a given password is set.

  NOTE: Will initialize the Password Store for the given handle if it doesn't exist

  @param[in]  Handle  PW_HANDLE for the password being queried.

  @retval     TRUE    Password is set.
  @retval     FALSE   Password is not set, or an error occurred preventing
                      the check from completing successfully.

**/
BOOLEAN
EFIAPI
PasswordStoreIsPasswordSet (
  VOID
  )
{
  return FALSE;
} // PasswordStoreIsPasswordSet()


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
  IN  CONST CHAR16   *Password
  )
{

  return FALSE;
} // PasswordStoreAuthenticatePassword()



/**
The constructor function initializes the Lib for Dxe.

This constructor is only needed for MsSettingsManager support.
The design is to have the PCD false fall all modules except the 1 that should support the MsSettingsManager.  Because this
is a build time PCD

The constructor function publishes Performance and PerformanceEx protocol, allocates memory to log DXE performance
and merges PEI performance data to DXE performance log.
It will ASSERT() if one of these operations fails and it will always return EFI_SUCCESS.

@param  ImageHandle   The firmware allocated handle for the EFI image.
@param  SystemTable   A pointer to the EFI System Table.

@retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PasswordStoreLibConstructor
(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{


  return EFI_SUCCESS;
}
