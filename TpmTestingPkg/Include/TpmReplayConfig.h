/** @file
  TPM Replay Configuration Structure

  Defines structures used to configure the TPM Replay feature.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TPM_REPLAY_CONFIG_H__
#define TPM_REPLAY_CONFIG_H__

#define TPM_REPLAY_CONFIG_SIGNATURE       SIGNATURE_64 ('_', 'T', 'R', '_', 'C', 'F', 'G', '_')
#define TPM_REPLAY_CONFIG_STRUCT_VERSION  0x00000001

#pragma pack(push, 1)

typedef union {
  UINT32    Data;
  struct {
    UINT32    Pcr0     : 1;       ///< 0 - PCR0
    UINT32    Pcr1     : 1;       ///< 1 - PCR1
    UINT32    Pcr2     : 1;       ///< 2 - PCR2
    UINT32    Pcr3     : 1;       ///< 3 - PCR3
    UINT32    Pcr4     : 1;       ///< 4 - PCR4
    UINT32    Pcr5     : 1;       ///< 5 - PCR5
    UINT32    Pcr6     : 1;       ///< 6 - PCR6
    UINT32    Pcr7     : 1;       ///< 7 - PCR7
    UINT32    Reserved : 24;      ///< 31:8 - Reserved
  } Pcrs;
} ACTIVE_PCRS;

typedef struct {
  UINT64         Signature;                     // Structure signature - TPM_REPLAY_CONFIG_SIGNATURE
  UINT32         StructureVersion;              // Structure version - Updates must be backward compatible
  UINT32         HeaderLength;                  // Length of this header in bytes
  ACTIVE_PCRS    ActivePcrs;                    // PCRs that are actively used by the TPM Replay feature
                                                // If a PCR is active, it will be cleared except for values
                                                // explicitly defined in a given TPM Replay event log.
} TPM_REPLAY_CONFIG;

#pragma pack(pop)

extern EFI_GUID  gTpmReplayConfigHobGuid;

#endif
