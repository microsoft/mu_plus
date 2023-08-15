/** @file
  TPM Replay Event Log Definitions

  These definitions are common to any code that needs to access or inspect these
  structures.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TPM_REPLAY_EVENT_LOG_H_
#define TPM_REPLAY_EVENT_LOG_H_

#include <Uefi.h>

//
// UEFI Variable constants
//
#define TPM_REPLAY_UEFI_VARIABLE_NAME  L"TpmReplayEventLog"
//
// This GUID is used as the UEFI variable vendor GUID for any variable data
// associated with TPM Replay.
//
#define TPM_REPLAY_VENDOR_GUID  \
  { 0xc6d186ff, 0xd248, 0x48f3, { 0xbb, 0x9a, 0xd9, 0x11, 0x03, 0xbb, 0xdd, 0x63 } }

extern EFI_GUID  gTpmReplayVendorGuid;

#define TPM_REPLAY_EVENT_LOG_STRUCTURE_SIGNATURE  SIGNATURE_64  ('_', 'T', 'P', 'M', 'R', 'P', 'L', '_')

#pragma pack(push, 1)

typedef struct _CALCULATED_PCR_STATE {
  UINT32    PcrIndex;
  // The TPML_DIGEST_VALUES structures will be packed
  // as a byte stream, rather than Unions.
  // TPML_DIGEST_VALUES      Values;
} CALCULATED_PCR_STATE;

// `Revision` field details:
//
// - Considering the mask: `0xAAAABBCC`
//   - For now, `AAAA` should be considered reserved, but may eventually encode non-structural
//     information such as signature type or signature digest size.
//   - `BB` is the major structure number.
//   - `CC` is the minor structure number.
// - Any change to the structure that would still be parseable by existing code (e.g. adding new fields at the end)
//   should result in an update to the minor number.
// - Any change that would break backwards compatibility with older parsing code (e.g. removing the `FinalPcrs` field
//   and all related fields) should result in an update to the major number.

typedef struct _TPM_REPLAY_EVENT_LOG {
  UINT64      StructureSignature;
  UINT32      Revision;
  EFI_TIME    Timestamp;
  UINT32      StructureSize;
  UINT32      FinalPcrCount;
  UINT32      OffsetToFinalPcrs;
  UINT32      EventLogCount;
  UINT32      OffsetToEventLog;
  // These fields are arbitrarily sized, but will always
  // follow the header fields.
  // CALCULATED_PCR_STATE     FinalPcrs[FinalPcrCount];
  // TCG_PCR_EVENT2           EventLog[EventLogCount];
} TPM_REPLAY_EVENT_LOG;

#pragma pack(pop)

#endif
