/** @file
  TPM Replay - TCG Definitions

  Definitions needed for TCG operations.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patents

**/

#ifndef TPM_REPLAY_TCG_H_
#define TPM_REPLAY_TCG_H_

#include <Base.h>
#include "TpmReplayTcgRegs.h"

#define TCG_PCR_0  0
#define TCG_PCR_1  1
#define TCG_PCR_2  2
#define TCG_PCR_3  3
#define TCG_PCR_4  4
#define TCG_PCR_5  5
#define TCG_PCR_6  6
#define TCG_PCR_7  7

typedef  UINT8  *PACKED_TPML_DIGEST_VALUES;
typedef  UINT8  *PACKED_TCG_PCR_EVENT2;

/**
  Dumps debug information about an event.

  @param[in]  PackedEvent       A pointer to the event.

**/
VOID
DumpEvent (
  IN CONST  PACKED_TCG_PCR_EVENT2  *PackedEvent
  );

/**
  Finds the algorithm offset in a list of digests.

  @param[in]   DigestValues     A pointer to a list of digest values.
  @param[out]  HashAlg          The algorithm to find.

  @return A pointer to the algorithm offset in the digest values structure or NULL
          if not found.

**/
CONST TPMT_HA *
FindSelectedAlgorithm (
  IN CONST TPML_DIGEST_VALUES  *DigestValues,
  IN       TPMI_ALG_HASH       HashAlg
  );

/**
  Finds the next matching event for a given PCR index.

  @param[in]  PcrIndex        The PCR index.
  @param[in]  StartEvent      The starting event to base the search on.
  @param[in]  LastEvent       The last event to base the search on.
  @param[in]  EventIndex      An optional pointer to a buffer to hold the event index.

  @return A pointer to the next event or NULL if an event is not found.

**/
CONST PACKED_TCG_PCR_EVENT2 *
GetNextMatchingEvent (
  IN UINT32                       PcrIndex,
  IN CONST PACKED_TCG_PCR_EVENT2  *StartEvent,
  IN CONST PACKED_TCG_PCR_EVENT2  *LastEvent,
  IN OUT   UINT32                 *EventIndex OPTIONAL
  );

/**
  Returns the size of a TCG PCR Event 2 structure.

  @param[in]  TcgPcrEvent2     A pointer to a TCG PCR Event 2 structure.

  @return The size in bytes of the given event.

**/
UINTN
GetPcrEvent2Size (
  IN CONST  TCG_PCR_EVENT2  *TcgPcrEvent2
  );

/**
  Returns the total size for a TCG EFI Spec ID Event.

  @param[in]  TcgEfiSpecIdEventStruct   The event to calculate the size for.

  @return The size in bytes of the event.

**/
UINTN
GetTcgEfiSpecIdEventStructSize (
  IN  CONST  TCG_EfiSpecIDEventStruct  *TcgEfiSpecIdEventStruct
  );

/**
  Returns whether an event is the Startup Locality Event.

  @param[in]  TcgPcrEventHdr    The event header.
  @param[in]  TcgPcrEventData   The event data.

  @return TRUE if the event is a Startup Locality Event, otherwise FALSE.

**/
BOOLEAN
IsStartupLocalityEvent (
  IN  CONST TCG_PCR_EVENT2_HDR  *TcgPcrEventHdr,
  IN  CONST VOID                *TcgPcrEventData
  );

/**
  Unpacks a TCG PCR Event 2 event.

  @param[in]   PackedEvent    A pointer to a packed event.
  @param[out]  UnpackedEvent  A pointer to a buffer to hold the unpacked event.
  @param[out]  PackedSize     The event size.
  @param[out]  EventData      A pointer to the event data.

  @return TRUE if the event unpacked successfully, otherwise FALSE.

**/
BOOLEAN
UnpackTcgPcrEvent2 (
  IN CONST PACKED_TCG_PCR_EVENT2  *PackedEvent,
  OUT      TCG_PCR_EVENT2         *UnpackedEvent,
  OUT      UINT32                 *PackedSize OPTIONAL,
  OUT      VOID                   **EventData OPTIONAL
  );

/**
  Unpacks TPM digest values.

  @param[in]   PackedValue     A pointer to packed TPM digest values.
  @param[out]  UnpackedValues  A pointer to a buffer to hold the unpacked values.
  @param[out]  PackedSize      A pointer to a buffer to hold the size of the data.

  @return TRUE if the values unpacked successfully, otherwise FALSE.

**/
BOOLEAN
UnpackTpmlDigestValues (
  IN CONST PACKED_TPML_DIGEST_VALUES  *PackedValues,
  OUT      TPML_DIGEST_VALUES         *UnpackedValues,
  OUT      UINT32                     *PackedSize
  );

#endif
