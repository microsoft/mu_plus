/** @file PlatformKeyLib.c

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <UefiSecureBoot.h>
#include <Guid/ImageAuthentication.h>
#include <Library/SecureBootVariableLib.h>

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#define PLATFORM_SECURE_BOOT_KEY_COUNT  2

SECURE_BOOT_PAYLOAD_INFO  *gSecureBootPayload     = NULL;
UINT8                     gSecureBootPayloadCount = 0;

UINT8                     mSecureBootPayloadCount                            = PLATFORM_SECURE_BOOT_KEY_COUNT;
SECURE_BOOT_PAYLOAD_INFO  mSecureBootPayload[PLATFORM_SECURE_BOOT_KEY_COUNT] = {
  {
    .SecureBootKeyName = L"Microsoft Only",
    .KekPtr            = (CONST UINT8 *)FixedPcdGetPtr (PcdDefaultKek),
    .KekSize           = (CONST UINT32)FixedPcdGetSize (PcdDefaultKek),
    .DbPtr             = (CONST UINT8 *)FixedPcdGetPtr (PcdDefaultDb),
    .DbSize            = (CONST UINT32)FixedPcdGetSize (PcdDefaultDb),
    .DbxPtr            = (CONST UINT8 *)FixedPcdGetPtr (PcdDefaultDbx),
    .DbxSize           = (CONST UINT32)FixedPcdGetSize (PcdDefaultDbx),
    .PkPtr             = (CONST UINT8 *)FixedPcdGetPtr (PcdDefaultPk),
    .PkSize            = (CONST UINT32)FixedPcdGetSize (PcdDefaultPk),
    .DbtPtr            = NULL,
    .DbtSize           = 0,
  },
  {
    .SecureBootKeyName = L"Microsoft Plus 3rd Party",
    .KekPtr            = (CONST UINT8 *)FixedPcdGetPtr (PcdDefaultKek),
    .KekSize           = (CONST UINT32)FixedPcdGetSize (PcdDefaultKek),
    .DbPtr             = (CONST UINT8 *)FixedPcdGetPtr (PcdDefault3PDb),
    .DbSize            = (CONST UINT32)FixedPcdGetSize (PcdDefault3PDb),
    .DbxPtr            = (CONST UINT8 *)FixedPcdGetPtr (PcdDefaultDbx),
    .DbxSize           = (CONST UINT32)FixedPcdGetSize (PcdDefaultDbx),
    .PkPtr             = (CONST UINT8 *)FixedPcdGetPtr (PcdDefaultPk),
    .PkSize            = (CONST UINT32)FixedPcdGetSize (PcdDefaultPk),
    .DbtPtr            = NULL,
    .DbtSize           = 0,
  }
};

/**
  Interface to fetch platform Secure Boot Certificates, each payload
  corresponds to a designated set of db, dbx, dbt, KEK, PK.

  @param[in]  Keys        Pointer to hold the returned sets of keys. The
                          returned buffer will be treated as CONST and
                          permanent pointer. The consumer will NOT free
                          the buffer after use.
  @param[in]  KeyCount    The number of sets available in the returned Keys.

  @retval     EFI_SUCCESS             The Keys are properly fetched.
  @retval     EFI_INVALID_PARAMETER   Inputs have NULL pointers.
  @retval     Others                  Something went wrong. Investigate further.
**/
EFI_STATUS
EFIAPI
GetPlatformKeyStore (
  OUT SECURE_BOOT_PAYLOAD_INFO  **Keys,
  OUT UINT8                     *KeyCount
  )
{
  if ((Keys == NULL) || (KeyCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Keys     = gSecureBootPayload;
  *KeyCount = gSecureBootPayloadCount;

  return EFI_SUCCESS;
}

/**
  The constructor gets the secure boot platform keys populated.

  @retval EFI_SUCCESS     The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SecureBootKeyStoreLibConstructor (
  VOID
  )
{
  gSecureBootPayload      = mSecureBootPayload;
  gSecureBootPayloadCount = mSecureBootPayloadCount;

  return EFI_SUCCESS;
}
