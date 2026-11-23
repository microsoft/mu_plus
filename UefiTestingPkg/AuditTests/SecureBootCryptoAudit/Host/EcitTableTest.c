/** @file
  Exercise the same ECIT validator used by the firmware audit.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Guid/CryptoIndicatorTable.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>

#include "../UEFI/EcitTable.h"

typedef struct {
  EFI_CRYPTO_INDICATOR_TABLE    Header;
  EFI_CRYPTO_INDICATOR_ENTRY    Entries[2];
} TEST_TABLE;

typedef enum {
  ValidTable,
  EmptyTable,
  RepeatedOpaqueFeature,
  RepeatedRevocationFeature,
  BadSignature,
  BadVersion,
  HeaderReserved,
  BadChecksum,
  TruncatedHeader,
  TruncatedBuffer,
  ShortDeclaredLength,
  TruncatedEntryHeader,
  ZeroEntryLength,
  ShortEntryLength,
  UnalignedEntryLength,
  EntryPastEnd,
  EntryReserved,
  TrailingBytes,
  TooManyEntries,
  DuplicateUpdateAuthorization,
  DuplicateImageAuthorization,
  DuplicateImageVerification,
  DuplicateUpdateVerification
} TABLE_MUTATION;

typedef struct {
  CHAR8             *Name;
  TABLE_MUTATION    Mutation;
  EFI_STATUS        ExpectedStatus;
} TABLE_TEST_CASE;

STATIC TABLE_TEST_CASE  mTestCases[] = {
  { "Valid table",                         ValidTable,                   EFI_SUCCESS          },
  { "Empty table",                         EmptyTable,                   EFI_SUCCESS          },
  { "Repeated opaque feature",             RepeatedOpaqueFeature,        EFI_SUCCESS          },
  { "Preserve repeated revocation policy", RepeatedRevocationFeature,    EFI_SUCCESS          },
  { "Bad signature",                       BadSignature,                 EFI_COMPROMISED_DATA },
  { "Bad version",                         BadVersion,                   EFI_COMPROMISED_DATA },
  { "Header reserved bytes",               HeaderReserved,               EFI_COMPROMISED_DATA },
  { "Bad checksum",                        BadChecksum,                  EFI_CRC_ERROR        },
  { "Truncated header allocation",         TruncatedHeader,              EFI_COMPROMISED_DATA },
  { "Declared length exceeds allocation",  TruncatedBuffer,              EFI_COMPROMISED_DATA },
  { "Declared length below header size",   ShortDeclaredLength,          EFI_COMPROMISED_DATA },
  { "Truncated entry header",              TruncatedEntryHeader,         EFI_COMPROMISED_DATA },
  { "Zero entry length",                   ZeroEntryLength,              EFI_COMPROMISED_DATA },
  { "Entry shorter than header",           ShortEntryLength,             EFI_COMPROMISED_DATA },
  { "Unaligned entry length",              UnalignedEntryLength,         EFI_COMPROMISED_DATA },
  { "Entry exceeds remaining table",       EntryPastEnd,                 EFI_COMPROMISED_DATA },
  { "Entry reserved bytes",                EntryReserved,                EFI_COMPROMISED_DATA },
  { "Trailing bytes after entries",        TrailingBytes,                EFI_COMPROMISED_DATA },
  { "Entry count exceeds table",           TooManyEntries,               EFI_COMPROMISED_DATA },
  { "Duplicate update authorization",      DuplicateUpdateAuthorization, EFI_COMPROMISED_DATA },
  { "Duplicate image authorization",       DuplicateImageAuthorization,  EFI_COMPROMISED_DATA },
  { "Duplicate image verification",        DuplicateImageVerification,   EFI_COMPROMISED_DATA },
  { "Duplicate update verification",       DuplicateUpdateVerification,  EFI_COMPROMISED_DATA }
};

STATIC CONST EFI_GUID  mSingletonFeatures[] = {
  EFI_ECIT_FEATURE_SB_DATABASE_UPDATE_AUTHORIZATION_GUID,
  EFI_ECIT_FEATURE_SB_IMAGE_AUTHORIZATION_GUID,
  EFI_ECIT_FEATURE_SB_IMAGE_VERIFICATION_GUID,
  EFI_ECIT_FEATURE_SB_DATABASE_UPDATE_VERIFICATION_GUID
};

STATIC
VOID
UpdateChecksum (
  IN OUT EFI_CRYPTO_INDICATOR_TABLE  *Table
  )
{
  Table->Checksum = 0;
  Table->Checksum = CalculateCheckSum8 ((CONST UINT8 *)Table, Table->Length);
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TestTable (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  CONST TABLE_TEST_CASE  *Case;
  TEST_TABLE             Table;
  UINTN                  BufferSize;
  VOID                   *Buffer;
  EFI_STATUS             Status;

  Case       = Context;
  BufferSize = sizeof (Table);
  ZeroMem (&Table, sizeof (Table));
  CopyMem (Table.Header.Signature, "ECIT", sizeof (Table.Header.Signature));
  Table.Header.Version         = EFI_CRYPTO_INDICATOR_TABLE_VERSION;
  Table.Header.Length          = sizeof (Table);
  Table.Header.NumberOfEntries = ARRAY_SIZE (Table.Entries);
  CopyGuid (&Table.Entries[0].FeatureIdentifier, &mSingletonFeatures[0]);
  CopyGuid (&Table.Entries[1].FeatureIdentifier, &mSingletonFeatures[1]);
  Table.Entries[0].EntryLength = sizeof (Table.Entries[0]);
  Table.Entries[1].EntryLength = sizeof (Table.Entries[1]);

  switch (Case->Mutation) {
    case ValidTable:
    case BadChecksum:
      break;
    case EmptyTable:
      Table.Header.Length          = sizeof (Table.Header);
      Table.Header.NumberOfEntries = 0;
      break;
    case RepeatedOpaqueFeature:
      ZeroMem (&Table.Entries[0].FeatureIdentifier, sizeof (EFI_GUID));
      ZeroMem (&Table.Entries[1].FeatureIdentifier, sizeof (EFI_GUID));
      break;
    case RepeatedRevocationFeature:
    {
      CONST EFI_GUID  Feature = EFI_ECIT_FEATURE_SB_IMAGE_REVOCATION_GUID;

      CopyGuid (&Table.Entries[0].FeatureIdentifier, &Feature);
      CopyGuid (&Table.Entries[1].FeatureIdentifier, &Feature);
      break;
    }
    case BadSignature:
      Table.Header.Signature[0] = 'X';
      break;
    case BadVersion:
      Table.Header.Version++;
      break;
    case HeaderReserved:
      Table.Header.Reserved[0] = 1;
      break;
    case TruncatedHeader:
      BufferSize = sizeof (Table.Header) - 1;
      break;
    case TruncatedBuffer:
      BufferSize--;
      break;
    case ShortDeclaredLength:
      Table.Header.Length = sizeof (Table.Header) - 1;
      break;
    case TruncatedEntryHeader:
      Table.Header.Length          = sizeof (Table.Header) + sizeof (Table.Entries[0]) - 1;
      Table.Header.NumberOfEntries = 1;
      break;
    case ZeroEntryLength:
      Table.Entries[0].EntryLength = 0;
      break;
    case ShortEntryLength:
      Table.Entries[0].EntryLength--;
      break;
    case UnalignedEntryLength:
      Table.Entries[0].EntryLength++;
      break;
    case EntryPastEnd:
      Table.Entries[0].EntryLength = sizeof (Table.Entries) + 8;
      break;
    case EntryReserved:
      Table.Entries[0].Reserved[0] = 1;
      break;
    case TrailingBytes:
      Table.Header.NumberOfEntries = 1;
      break;
    case TooManyEntries:
      Table.Header.NumberOfEntries++;
      break;
    case DuplicateUpdateAuthorization:
    case DuplicateImageAuthorization:
    case DuplicateImageVerification:
    case DuplicateUpdateVerification:
      CopyGuid (
        &Table.Entries[0].FeatureIdentifier,
        &mSingletonFeatures[Case->Mutation - DuplicateUpdateAuthorization]
        );
      CopyGuid (&Table.Entries[1].FeatureIdentifier, &Table.Entries[0].FeatureIdentifier);
      break;
    default:
      UT_ASSERT_TRUE (FALSE);
  }

  UpdateChecksum (&Table.Header);
  if (Case->Mutation == BadChecksum) {
    Table.Header.Checksum++;
  }

  Buffer = AllocateCopyPool (BufferSize, &Table);
  UT_ASSERT_NOT_NULL (Buffer);
  Status = ValidateEcitTable (Buffer, BufferSize);
  FreePool (Buffer);
  UT_ASSERT_STATUS_EQUAL (Status, Case->ExpectedStatus);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TestNullBuffer (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_STATUS_EQUAL (ValidateEcitTable (NULL, MAX_UINTN), EFI_INVALID_PARAMETER);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TestMaximumLength (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  CONST UINTN                 Lengths[] = { MAX_ECIT_LENGTH - 8, MAX_ECIT_LENGTH, MAX_ECIT_LENGTH + 8 };
  EFI_CRYPTO_INDICATOR_TABLE  *Table;
  EFI_CRYPTO_INDICATOR_ENTRY  *Entry;
  EFI_STATUS                  Status;
  UINTN                       Index;
  UINTN                       Offset;

  for (Index = 0; Index < ARRAY_SIZE (Lengths); Index++) {
    Table = AllocateZeroPool (Lengths[Index]);
    UT_ASSERT_NOT_NULL (Table);
    CopyMem (Table->Signature, "ECIT", sizeof (Table->Signature));
    Table->Version = EFI_CRYPTO_INDICATOR_TABLE_VERSION;
    Table->Length  = (UINT32)Lengths[Index];
    Offset         = sizeof (*Table);
    while (Offset < Table->Length) {
      Entry              = (EFI_CRYPTO_INDICATOR_ENTRY *)((UINT8 *)Table + Offset);
      Entry->EntryLength = (UINT16)MIN (MAX_UINT16 & ~7U, Table->Length - Offset);
      Offset            += Entry->EntryLength;
      Table->NumberOfEntries++;
    }

    UpdateChecksum (Table);
    Status = ValidateEcitTable (Table, Lengths[Index]);
    FreePool (Table);
    UT_ASSERT_STATUS_EQUAL (
      Status,
      (Lengths[Index] > MAX_ECIT_LENGTH) ? EFI_COMPROMISED_DATA : EFI_SUCCESS
      );
  }

  return UNIT_TEST_PASSED;
}

int
main (
  int   Argc,
  char  *Argv[]
  )
{
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      Suite;
  EFI_STATUS                  Status;
  UINTN                       Index;

  Framework = NULL;
  Status    = InitUnitTestFramework (&Framework, "ECIT table validation", gEfiCallerBaseName, "1.0");
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to initialize ECIT table tests: %r\n", Status));
    return (int)Status;
  }

  Status = CreateUnitTestSuite (&Suite, Framework, "ECIT table bounds and structure", "EcitTable", NULL, NULL);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  for (Index = 0; Index < ARRAY_SIZE (mTestCases); Index++) {
    Status = AddTestCase (Suite, mTestCases[Index].Name, "Structure", TestTable, NULL, NULL, &mTestCases[Index]);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
  }

  Status = AddTestCase (Suite, "NULL buffer", "Bounds", TestNullBuffer, NULL, NULL, NULL);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = AddTestCase (Suite, "One MiB length boundary", "Bounds", TestMaximumLength, NULL, NULL, NULL);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = RunAllTestSuites (Framework);

Exit:
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ECIT table tests failed: %r\n", Status));
  }

  FreeUnitTestFramework (Framework);
  return (int)Status;
}
