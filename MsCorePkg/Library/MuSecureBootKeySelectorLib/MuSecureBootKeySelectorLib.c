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
#include <Library/UefiBootServicesTableLib.h>       // gBS
#include <Library/SecureBootVariableLib.h>          // Secure Boot Variables Operations
#include <Library/MuSecureBootKeySelectorLib.h>     // Our header
#include <Library/SecureBootKeyStoreLib.h>          // GetPlatformKeyStore
#include <Protocol/Hash2.h>                         // Hash2 Protocol
#include <Protocol/ServiceBinding.h>                // Service Binding Protocol

//
// These represent the SHA256 hashes of the Microsoft certificates that could potentially be
// used in the Secure Boot configuration. A platform may have any one or more of these
// certificates in the Secure Boot configuration depending on if the platform is carrying
// Legacy Microsoft keys or Microsoft Serviced keys.
//
UINT8  mMicrosoftCertificates[][sizeof (EFI_SHA256_HASH2)] = {
  { // Microsoft UEFI Certificate 2011
    0x48, 0xE9, 0x9B, 0x99, 0x1F, 0x57, 0xFC, 0x52, 0xF7, 0x61, 0x49, 0x59, 0x9B, 0xFF, 0x0A, 0x58,
    0xC4, 0x71, 0x54, 0x22, 0x9B, 0x9F, 0x8D, 0x60, 0x3A, 0xC4, 0x0D, 0x35, 0x00, 0x24, 0x85, 0x07
  },
  { // Microsoft Windows First Party Certificate 2011
    0xE8, 0xE9, 0x5F, 0x07, 0x33, 0xA5, 0x5E, 0x8B, 0xAD, 0x7B, 0xE0, 0xA1, 0x41, 0x3E, 0xE2, 0x3C,
    0x51, 0xFC, 0xEA, 0x64, 0xB3, 0xC8, 0xFA, 0x6A, 0x78, 0x69, 0x35, 0xFD, 0xDC, 0xC7, 0x19, 0x61
  },
  { // Microsoft Option ROM Certificate 2023
    0xE5, 0xBE, 0x3E, 0x64, 0xC6, 0xE6, 0x6A, 0x28, 0x14, 0x57, 0xEC, 0xDE, 0xCE, 0x0D, 0x6D, 0x07,
    0x87, 0x57, 0x7A, 0xAD, 0x2A, 0x3A, 0x01, 0x44, 0x26, 0x2C, 0x10, 0xC1, 0x4B, 0xA8, 0xD8, 0xF1
  },
  { // Microsoft UEFI Certificate 2023
    0xF6, 0x12, 0x4E, 0x34, 0x12, 0x5B, 0xEE, 0x3F, 0xE6, 0xD7, 0x9A, 0x57, 0x4E, 0xAA, 0x7B, 0x91,
    0xC0, 0xE7, 0xBD, 0x9D, 0x92, 0x9C, 0x1A, 0x32, 0x11, 0x78, 0xEF, 0xD6, 0x11, 0xDA, 0xD9, 0x01
  },
  { // Microsoft Windows First Party Certificate 2023
    0x07, 0x6F, 0x1F, 0xEA, 0x90, 0xAC, 0x29, 0x15, 0x5E, 0xBF, 0x77, 0xC1, 0x76, 0x82, 0xF7, 0x5F,
    0x1F, 0xDD, 0x1B, 0xE1, 0x96, 0xDA, 0x30, 0x2D, 0xC8, 0x46, 0x1E, 0x35, 0x0A, 0x9A, 0xE3, 0x30
  }
};

/**
  Determine if the Secure Boot Keys in the provided database (DbVar) correspond to
  Microsoft-serviced keys.

  This function checks the certificates in the provided Secure Boot database (DbVar)
  against a predefined list of Microsoft certificates.
  If all certificates in the database match the Microsoft-serviced certificates,
  the machine is considered `Microsoft Serviced.` If any certificate does not match,
  the machine is considered `CUSTOM,` and the function returns an error.

  @param[in]  DbVar               Pointer to the Secure Boot database (db) variable.
  @param[in]  DbVarSize           Size of the Secure Boot database in bytes.
  @param[out] IsMsServicedConfig  Pointer to a BOOLEAN that will be set to TRUE if
                                  the database matches Microsoft-serviced keys, or
                                  FALSE otherwise.

  @retval EFI_SUCCESS             The operation completed successfully, and the result
                                  is stored in IsMsServicedConfig.
  @retval EFI_NOT_FOUND           A certificate in the database does not match any
                                  Microsoft-serviced certificates.
  @retval EFI_UNSUPPORTED         The hashing protocol or service binding protocol
                                  required for the operation is not available.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed during the operation.
  @retval Other                   An error occurred during the operation.

**/
EFI_STATUS
EFIAPI
IsMsftServicedConfig (
  UINT8    *DbVar,
  UINTN    DbVarSize,
  BOOLEAN  *IsMsServicedConfig
  )
{
  EFI_STATUS                    Status;
  EFI_SIGNATURE_LIST            *CertList;
  EFI_SIGNATURE_DATA            *Cert;
  UINTN                         CertCount;
  UINT8                         *RootCert;
  UINTN                         RootCertSize;
  UINTN                         Index;
  UINTN                         CertIndex;
  EFI_SERVICE_BINDING_PROTOCOL  *Hash2ServiceBinding;
  EFI_HASH2_PROTOCOL            *Hash2Protocol;
  EFI_HASH2_OUTPUT              HashOutput;
  EFI_HANDLE                    mHash2ServiceHandle;
  BOOLEAN                       IsMsftCertificate;

  mHash2ServiceHandle = NULL;
  IsMsftCertificate   = FALSE;

  //
  // Check if the input parameters are valid.
  ASSERT (IsMsServicedConfig != NULL && DbVar != NULL && DbVarSize > 0);
  if ((IsMsServicedConfig == NULL) || (DbVar == NULL) || (DbVarSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Attempt to bind and create an instance of the hash protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiHash2ServiceBindingProtocolGuid,
                  NULL,
                  (VOID **)&Hash2ServiceBinding
                  );
  if (!EFI_ERROR (Status) && (Hash2ServiceBinding != NULL) && (Hash2ServiceBinding->CreateChild != NULL)) {
    //
    // Create an instance of the hash protocol for this controller.
    //
    Status = Hash2ServiceBinding->CreateChild (Hash2ServiceBinding, &mHash2ServiceHandle);
    if (EFI_ERROR (Status)) {
      Status = EFI_UNSUPPORTED;
      goto Exit;
    }

    //
    // Attempt to open the Hash2 protocol on the newly created instance.
    //
    Status = gBS->OpenProtocol (
                    mHash2ServiceHandle,
                    &gEfiHash2ProtocolGuid,
                    (VOID **)&Hash2Protocol,
                    NULL,
                    mHash2ServiceHandle,
                    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      Status = EFI_UNSUPPORTED;
      goto Exit;
    }
  } else {
    //
    // If binding and creating the child fails, attempt to locate the protocol directly.
    //
    Status = gBS->LocateProtocol (&gEfiHash2ProtocolGuid, NULL, (VOID **)&Hash2Protocol);
    if (EFI_ERROR (Status)) {
      Status = EFI_UNSUPPORTED;
      goto Exit;
    }
  }

  CertList = (EFI_SIGNATURE_LIST *)DbVar;
  while ((DbVarSize > 0) && (DbVarSize >= (UINTN)CertList->SignatureListSize)) {
    //
    // Make sure the signature type is X.509.
    //
    if (CompareGuid (&CertList->SignatureType, &gEfiCertX509Guid)) {
      Cert      = (EFI_SIGNATURE_DATA *)((UINT8 *)CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
      CertCount = (CertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;

      IsMsftCertificate = FALSE;
      for (Index = 0; Index < CertCount; Index++) {
        //
        // Iterate each Signature Data Node within this CertList for verify.
        //
        RootCert     = Cert->SignatureData;
        RootCertSize = CertList->SignatureSize - sizeof (EFI_GUID);
        Status       = Hash2Protocol->HashInit (Hash2Protocol, &gEfiHashAlgorithmSha256Guid);
        if (EFI_ERROR (Status)) {
          goto Exit;
        }

        Status = Hash2Protocol->HashUpdate (Hash2Protocol, RootCert, RootCertSize);
        if (EFI_ERROR (Status)) {
          goto Exit;
        }

        Status = Hash2Protocol->HashFinal (Hash2Protocol, &HashOutput);
        if (EFI_ERROR (Status)) {
          goto Exit;
        }

        //
        // Compare the hash value with the MSFT certificate hashes.
        //
        for (CertIndex = 0; CertIndex < sizeof (mMicrosoftCertificates) / sizeof (mMicrosoftCertificates[0]); CertIndex++) {
          if (CompareMem (HashOutput.Sha256Hash, mMicrosoftCertificates[CertIndex], sizeof (HashOutput.Sha256Hash)) == 0) {
            //
            // Found a match, this is a Microsoft certificate
            //
            IsMsftCertificate = TRUE;
            break;
          }
        }

        //
        // If no match was found, the machine is "CUSTOM"
        //
        if (!IsMsftCertificate) {
          DEBUG ((DEBUG_ERROR, "[%a] - Certificate does not match any MSFT certificates. Machine is CUSTOM.\n", __FUNCTION__));
          Status = EFI_NOT_FOUND;
          goto Exit;
        }

        Cert = (EFI_SIGNATURE_DATA *)((UINT8 *)Cert + CertList->SignatureSize);
      }
    }

    DbVarSize -= CertList->SignatureListSize;
    CertList   = (EFI_SIGNATURE_LIST *)((UINT8 *)CertList + CertList->SignatureListSize);
  }

  //
  // If we get here than we can assume that the current db matches some configuration of MS Serviced Keys.
  //
  if (IsMsServicedConfig != NULL) {
    *IsMsServicedConfig = TRUE;
  }

Exit:
  //
  // Destroy the Hash2ServiceBinding instance if it is created.
  //
  if (mHash2ServiceHandle != NULL) {
    Status = gBS->LocateProtocol (
                    &gEfiHash2ServiceBindingProtocolGuid,
                    NULL,
                    (VOID **)&Hash2ServiceBinding
                    );
    if (EFI_ERROR (Status) || (Hash2ServiceBinding == NULL) || (Hash2ServiceBinding->DestroyChild == NULL)) {
      return EFI_UNSUPPORTED;
    }

    //
    // Destroy the instance of the hashing protocol for this controller.
    //
    Status = Hash2ServiceBinding->DestroyChild (Hash2ServiceBinding, mHash2ServiceHandle);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    mHash2ServiceHandle = NULL;
  }

  return Status;
}

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
  BOOLEAN                   IsMsServiced           = FALSE;

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

  //
  // If the current db doesn't match any of the stored dbs, check if it's a Microsoft Serviced config.
  if (Config == MU_SB_CONFIG_UNKNOWN) {
    Status = IsMsftServicedConfig (DbVar, DbVarSize, &IsMsServiced);
    if ((Status == EFI_SUCCESS) && IsMsServiced) {
      Config = MU_SB_CONFIG_MS_SERVICED_KEY;
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
