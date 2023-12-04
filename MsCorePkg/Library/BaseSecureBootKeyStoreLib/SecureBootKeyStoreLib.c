/** @file SecureBootKeyStoreLib.c


  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <UefiSecureBoot.h>

#include <Guid/ImageAuthentication.h>

#include <Library/SecureBootVariableLib.h>
#include <Library/PcdLib.h>

SECURE_BOOT_PAYLOAD_INFO  mSecureBootPayload[] = {
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

  *Keys     = mSecureBootPayload;
  *KeyCount = ARRAY_SIZE (mSecureBootPayload);

  return EFI_SUCCESS;
}
