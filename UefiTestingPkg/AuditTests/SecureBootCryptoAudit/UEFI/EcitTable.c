/** @file
  Bounded, platform-independent ECIT structural validation.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Guid/CryptoIndicatorTable.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#include "EcitTable.h"

STATIC CONST EFI_GUID  mSingletonFeatures[] = {
  EFI_ECIT_FEATURE_SB_DATABASE_UPDATE_AUTHORIZATION_GUID,
  EFI_ECIT_FEATURE_SB_IMAGE_AUTHORIZATION_GUID,
  EFI_ECIT_FEATURE_SB_IMAGE_VERIFICATION_GUID,
  EFI_ECIT_FEATURE_SB_DATABASE_UPDATE_VERIFICATION_GUID
};

EFI_STATUS
ValidateEcitTable (
  IN CONST VOID  *Buffer,
  IN UINTN       BufferSize
  )
{
  CONST EFI_CRYPTO_INDICATOR_TABLE  *Table;
  CONST EFI_CRYPTO_INDICATOR_ENTRY  *Entry;
  BOOLEAN                           Seen[ARRAY_SIZE (mSingletonFeatures)];
  UINTN                             Count;
  UINTN                             Index;
  UINTN                             Offset;
  UINTN                             Remaining;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize < sizeof (*Table)) {
    return EFI_COMPROMISED_DATA;
  }

  Table = Buffer;
  if ((Table->Length < sizeof (*Table)) ||
      (Table->Length > MAX_ECIT_LENGTH) ||
      (Table->Length > BufferSize))
  {
    return EFI_COMPROMISED_DATA;
  }

  if ((CompareMem (Table->Signature, "ECIT", sizeof (Table->Signature)) != 0) ||
      (Table->Version != EFI_CRYPTO_INDICATOR_TABLE_VERSION) ||
      !IsZeroBuffer (Table->Reserved, sizeof (Table->Reserved)))
  {
    return EFI_COMPROMISED_DATA;
  }

  if (CalculateSum8 (Buffer, Table->Length) != 0) {
    return EFI_CRC_ERROR;
  }

  ZeroMem (Seen, sizeof (Seen));
  Offset = sizeof (*Table);
  for (Count = 0; Count < Table->NumberOfEntries; Count++) {
    Remaining = Table->Length - Offset;
    if (Remaining < sizeof (*Entry)) {
      return EFI_COMPROMISED_DATA;
    }

    Entry = (CONST EFI_CRYPTO_INDICATOR_ENTRY *)((CONST UINT8 *)Buffer + Offset);
    if ((Entry->EntryLength < sizeof (*Entry)) ||
        ((Entry->EntryLength & 7) != 0) ||
        (Entry->EntryLength > Remaining) ||
        !IsZeroBuffer (Entry->Reserved, sizeof (Entry->Reserved)))
    {
      return EFI_COMPROMISED_DATA;
    }

    // Other features may repeat when their opaque payload supplies context.
    for (Index = 0; Index < ARRAY_SIZE (mSingletonFeatures); Index++) {
      if (CompareGuid (&Entry->FeatureIdentifier, &mSingletonFeatures[Index])) {
        if (Seen[Index]) {
          return EFI_COMPROMISED_DATA;
        }

        Seen[Index] = TRUE;
        break;
      }
    }

    Offset += Entry->EntryLength;
  }

  return (Offset == Table->Length) ? EFI_SUCCESS : EFI_COMPROMISED_DATA;
}
