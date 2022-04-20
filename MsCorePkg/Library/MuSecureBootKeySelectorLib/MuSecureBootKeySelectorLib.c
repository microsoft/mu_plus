/** @file MuSecureBootKeySelectorLib.c

  This library implements functions to interact with platform supplied
  secure boot related keys through SecureBootKeyStoreLib.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>                               // This has to be here so Protocol/FirmwareVolume2.h doesn't cause errors.
#include <UefiSecureBoot.h>                      // SECURE_BOOT_PAYLOAD_INFO, etc

#include <Guid/ImageAuthentication.h>           // EFI_SIGNATURE_LIST, etc.

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>                  // CopyMem, etc.
#include <Library/MemoryAllocationLib.h>            // AllocateZeroPool, etc.
#include <Library/DebugLib.h>                       // Tracing
#include <Library/UefiRuntimeServicesTableLib.h>    // gRT
#include <Library/SecureBootVariableLib.h>          // Secure Boot Variables Operations
#include <Library/MuSecureBootKeySelectorLib.h>     // Our header
#include <Library/SecureBootKeyStoreLib.h>          // GetPlatformKeyStore

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
  )
{
  EFI_STATUS                Status;
  UINTN                     Config = MU_SB_CONFIG_NONE;       // Default to "None"
  UINTN                     DbVarSize;
  UINT8                     *DbVar = NULL;
  UINTN                     Index;
  UINT8                     SecureBootPayloadCount = 0;
  SECURE_BOOT_PAYLOAD_INFO  *SecureBootPayload     = NULL;

  Status = GetPlatformKeyStore (&SecureBootPayload, &SecureBootPayloadCount);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Determine whether the PK is set.
  // If it's not set, we'll indicate that we're in NONE regardless of db state.
  // NOTE: We don't care about getting the variable, we just want to see if it exists.
  DbVarSize = 0;
  Status    = gRT->GetVariable (
                     EFI_PLATFORM_KEY_NAME,
                     &gEfiGlobalVariableGuid,
                     NULL,
                     &DbVarSize,
                     DbVar
                     );
  if (Status == EFI_NOT_FOUND) {
    return MU_SB_CONFIG_NONE;
  }

  //
  // Load the current db.
  DbVarSize = 0;
  Status    = gRT->GetVariable (
                     EFI_IMAGE_SECURITY_DATABASE,
                     &gEfiImageSecurityDatabaseGuid,
                     NULL,
                     &DbVarSize,
                     DbVar
                     );
  // Only proceed if the error was buffer too small.
  if (Status == EFI_BUFFER_TOO_SMALL) {
    DbVar = AllocatePool (DbVarSize);
    if (DbVar != NULL) {
      Status = gRT->GetVariable (
                      EFI_IMAGE_SECURITY_DATABASE,
                      &gEfiImageSecurityDatabaseGuid,
                      NULL,
                      &DbVarSize,
                      DbVar
                      );
    }
  }
  // If it's missing, there are no keys installed.
  else if (Status == EFI_NOT_FOUND) {
    Config = MU_SB_CONFIG_NONE;
  }

  //
  // Compare the current db to the stored dbs and determine whether either matches.
  if (!EFI_ERROR (Status)) {
    Config = MU_SB_CONFIG_UNKNOWN;
    for (Index = 0; Index < SecureBootPayloadCount; Index++) {
      if ((DbVarSize == SecureBootPayload[Index].DbSize) && (CompareMem (DbVar, SecureBootPayload[Index].DbPtr, DbVarSize) == 0)) {
        Config = Index;
        break;
      }
    }
  }

  // Clean up if necessary.
  if (DbVar != NULL) {
    FreePool (DbVar);
  }

  return Config;
}

/**
  Returns the status of setting secure boot keys.

  @param  [in] Index  The index of key from key stores.

  @retval Will return the status of setting secure boot variables.

**/
EFI_STATUS
EFIAPI
SetSecureBootConfig (
  IN  UINT8  Index
  )
{
  EFI_STATUS                Status;
  UINT8                     SecureBootPayloadCount = 0;
  SECURE_BOOT_PAYLOAD_INFO  *SecureBootPayload     = NULL;

  Status = GetPlatformKeyStore (&SecureBootPayload, &SecureBootPayloadCount);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Index >= SecureBootPayloadCount) {
    return EFI_INVALID_PARAMETER;
  }

  return SetSecureBootVariablesToDefault (&SecureBootPayload[Index]);
}
