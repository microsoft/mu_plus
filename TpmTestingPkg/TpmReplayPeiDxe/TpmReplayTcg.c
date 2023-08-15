/** @file
  TPM Replay TCG - Main File

  Contains PI phase-common implementation for TCG related functionality needed to replay PCR measurements
  to a TPM.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TpmReplayTcg.h"
#include "TpmReplayTcgRegs.h"

#include <Pi/PiFirmwareFile.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/Tpm2CommandLib.h>

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
  )
{
  TPML_DIGEST_VALUES  *TempPackedValues;
  TPMT_HA             *TempPackedHa;
  UINT32              HaIndex;
  UINT32              AlgorithmSize;
  BOOLEAN             Unpacked;

  Unpacked = TRUE;
  ZeroMem (UnpackedValues, sizeof (*UnpackedValues));
  *PackedSize = 0;

  TempPackedValues      = (TPML_DIGEST_VALUES *)PackedValues;
  UnpackedValues->count = TempPackedValues->count;
  *PackedSize          += sizeof (TempPackedValues->count);
  TempPackedHa          = (TPMT_HA *)&TempPackedValues->digests[0];

  for (HaIndex = 0; HaIndex < UnpackedValues->count; HaIndex++) {
    UnpackedValues->digests[HaIndex].hashAlg = TempPackedHa->hashAlg;
    *PackedSize                             += sizeof (TempPackedHa->hashAlg);

    AlgorithmSize = GetHashSizeFromAlgo (TempPackedHa->hashAlg);
    if (AlgorithmSize == 0) {
      DEBUG ((
        DEBUG_ERROR,
        "[%a] Unrecognized algorithm 0x%X located at index %d!",
        __FUNCTION__,
        TempPackedHa->hashAlg,
        HaIndex
        ));
      Unpacked = FALSE;
      break;
    }

    CopyMem (&UnpackedValues->digests[HaIndex].digest, &TempPackedHa->digest, AlgorithmSize);
    *PackedSize += AlgorithmSize;
    TempPackedHa = (TPMT_HA *)((UINTN)&TempPackedHa->digest + AlgorithmSize);
  }

  return Unpacked;
}

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
  )
{
  UINT8    *TempPointer;
  VOID     *EventBuffer;
  UINT32   PackedDigestValuesSize;
  UINTN    TempPackedSize;
  BOOLEAN  Unpacked;

  ZeroMem (UnpackedEvent, sizeof (*UnpackedEvent));
  if (PackedSize != NULL) {
    *PackedSize = 0;
  }

  if (EventData != NULL) {
    *EventData = NULL;
  }

  TempPointer = (VOID *)PackedEvent;
  EventBuffer = NULL;

  UnpackedEvent->PCRIndex = *(TCG_PCRINDEX *)TempPointer;
  TempPointer            += sizeof (TCG_PCRINDEX);

  UnpackedEvent->EventType = *(TCG_EVENTTYPE *)TempPointer;
  TempPointer             += sizeof (TCG_EVENTTYPE);

  Unpacked = UnpackTpmlDigestValues ((PACKED_TPML_DIGEST_VALUES *)TempPointer, &UnpackedEvent->Digest, &PackedDigestValuesSize);

  if (Unpacked) {
    TempPointer += PackedDigestValuesSize;

    UnpackedEvent->EventSize = *(UINT32 *)TempPointer;
    TempPointer             += sizeof (UINT32);

    EventBuffer = AllocatePool (UnpackedEvent->EventSize);
    if (EventBuffer == NULL) {
      Unpacked = FALSE;
    } else {
      CopyMem (EventBuffer, (VOID *)TempPointer, UnpackedEvent->EventSize);
      TempPointer += UnpackedEvent->EventSize;
    }
  }

  if (Unpacked) {
    TempPackedSize = (UINTN)TempPointer - (UINTN)PackedEvent;
    if (TempPackedSize > MAX_UINT32) {
      Unpacked = FALSE;
    } else {
      if (PackedSize != NULL) {
        *PackedSize = (UINT32)TempPackedSize;
      }

      if (EventData != NULL) {
        *EventData = (UINT8 *)EventBuffer;
      }
    }
  }

  if ((EventBuffer != NULL) && (!Unpacked || (EventData == NULL))) {
    FreePool (EventBuffer);
    EventBuffer = NULL;
  }

  return Unpacked;
}

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
  )
{
  UINT32         AlgorithmIndex;
  CONST TPMT_HA  *FoundAlgorithm;

  FoundAlgorithm = NULL;
  for (AlgorithmIndex = 0; AlgorithmIndex < DigestValues->count; AlgorithmIndex++) {
    if (DigestValues->digests[AlgorithmIndex].hashAlg == HashAlg) {
      FoundAlgorithm = &DigestValues->digests[AlgorithmIndex];
      break;
    }
  }

  return FoundAlgorithm;
}

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
  )
{
  CONST PACKED_TCG_PCR_EVENT2  *Marker;
  CONST PACKED_TCG_PCR_EVENT2  *Match;
  TCG_PCR_EVENT2               CurrentEvent;
  UINT32                       PackedSize;

  Marker = StartEvent;
  Match  = NULL;

  if (!UnpackTcgPcrEvent2 (Marker, &CurrentEvent, &PackedSize, NULL)) {
    return NULL;
  }

  while (Marker != LastEvent) {
    Marker = (PACKED_TCG_PCR_EVENT2 *)((UINTN)Marker + PackedSize);
    if (EventIndex != NULL) {
      *EventIndex = *EventIndex + 1;
    }

    if (!UnpackTcgPcrEvent2 (Marker, &CurrentEvent, &PackedSize, NULL)) {
      return NULL;
    }

    if (CurrentEvent.PCRIndex == PcrIndex) {
      Match = Marker;
      break;
    }
  }

  return Match;
}

/**
  Returns the total size for a TCG EFI Spec ID Event.

  @param[in]  TcgEfiSpecIdEventStruct   The event to calculate the size for.

  @return The size in bytes of the event.

**/
UINTN
GetTcgEfiSpecIdEventStructSize (
  IN  CONST  TCG_EfiSpecIDEventStruct  *TcgEfiSpecIdEventStruct
  )
{
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  UINT8                            *VendorInfoSize;
  UINT32                           NumberOfAlgorithms;

  CopyMem (&NumberOfAlgorithms, TcgEfiSpecIdEventStruct + 1, sizeof (NumberOfAlgorithms));

  DigestSize     = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct + sizeof (*TcgEfiSpecIdEventStruct) + sizeof (NumberOfAlgorithms));
  VendorInfoSize = (UINT8 *)&DigestSize[NumberOfAlgorithms];
  return sizeof (TCG_EfiSpecIDEventStruct) + sizeof (UINT32) + (NumberOfAlgorithms * sizeof (TCG_EfiSpecIdEventAlgorithmSize)) + sizeof (UINT8) + (*VendorInfoSize);
}

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
  )
{
  UINTN                              StringSize;
  CONST TCG_EfiStartupLocalityEvent  *StartupLocalityEventPtr;

  if ((TcgPcrEventHdr == NULL) || (TcgPcrEventData == NULL)) {
    ASSERT (TcgPcrEventHdr != NULL);
    ASSERT (TcgPcrEventData != NULL);
    return FALSE;
  }

  StartupLocalityEventPtr = (TCG_EfiStartupLocalityEvent *)TcgPcrEventData;

  StringSize = AsciiStrnSizeS ((CONST CHAR8 *)StartupLocalityEventPtr, sizeof (TCG_EfiStartupLocalityEvent_SIGNATURE));
  if ((StringSize == sizeof (TCG_EfiStartupLocalityEvent_SIGNATURE)) &&
      (CompareMem (&StartupLocalityEventPtr->Signature[0], TCG_EfiStartupLocalityEvent_SIGNATURE, StringSize) == 0))
  {
    if ((StartupLocalityEventPtr->StartupLocality == 0) || (StartupLocalityEventPtr->StartupLocality == 3)) {
      return TRUE;
    }

    DEBUG ((DEBUG_ERROR, "[%a] - Unexpected locality found!\n", __FUNCTION__));
    ASSERT (StartupLocalityEventPtr->StartupLocality == LOCALITY_0_INDICATOR || StartupLocalityEventPtr->StartupLocality == LOCALITY_3_INDICATOR);
  }

  return FALSE;
}

/**
  Dumps debug information about an event.

  @param[in]  PackedEvent       A pointer to the event.

**/
VOID
DumpEvent (
  IN CONST  PACKED_TCG_PCR_EVENT2  *PackedEvent
  )
{
  DEBUG_CODE_BEGIN ();

  UINT16          DigestSize;
  UINT32          PackedEventSize;
  TCG_PCR_EVENT2  UnpackedEvent;
  UINTN           Index;
  UINTN           Index2;
  VOID            *UnpackedEventData;

  if (PackedEvent == NULL) {
    return;
  }

  if (!UnpackTcgPcrEvent2 (PackedEvent, &UnpackedEvent, &PackedEventSize, &UnpackedEventData)) {
    ASSERT (FALSE);
    return;
  }

  DEBUG ((DEBUG_ERROR, "[%a] - TPM Replay Event Info (@0x%p):\n", __FUNCTION__, PackedEvent));
  DEBUG ((DEBUG_ERROR, "[%a] -   PCR Index: %02d\n", __FUNCTION__, UnpackedEvent.PCRIndex));
  DEBUG ((DEBUG_ERROR, "[%a] -   Event Type: 0x%08x\n", __FUNCTION__, UnpackedEvent.EventType));
  DEBUG ((DEBUG_ERROR, "[%a] -   Event Data Size: 0x%08x\n", __FUNCTION__, UnpackedEvent.EventSize));
  DEBUG ((DEBUG_ERROR, "[%a] -   Digest Count: %d\n", __FUNCTION__, UnpackedEvent.Digest.count));

  for (Index = 0; Index < UnpackedEvent.Digest.count; Index++) {
    DEBUG ((DEBUG_ERROR, "[%a] -   Digest[%d]\n", __FUNCTION__, Index));

    DigestSize = GetHashSizeFromAlgo (UnpackedEvent.Digest.digests[Index].hashAlg);
    ASSERT (DigestSize != 0);

    if (DigestSize != 0) {
      DEBUG ((DEBUG_ERROR, "[%a] -     Size: 0x%04x\n", __FUNCTION__, DigestSize));
      DEBUG ((
        DEBUG_ERROR,
        "[%a] -     Algorithm: 0x%02x\n",
        __FUNCTION__,
        UnpackedEvent.Digest.digests[Index].hashAlg
        ));
      DEBUG ((DEBUG_ERROR, "[%a] -     Value = ", __FUNCTION__));

      for (Index2 = 0; Index2 < DigestSize; Index2++) {
        DEBUG ((DEBUG_ERROR, "%02x ", ((UINT8 *)&UnpackedEvent.Digest.digests[Index].digest)[Index2]));
      }

      DEBUG ((DEBUG_ERROR, "\n\n"));
    }
  }

  if ((UnpackedEventData != NULL) && (UnpackedEvent.EventSize > 0)) {
    // UnpackedEventData = (VOID *)((UINT8 *)UnpackedEventData + OFFSET_OF (TCG_PCR_EVENT2, Event));
    DEBUG ((DEBUG_ERROR, "[%a] -   Event Data:\n", __FUNCTION__));
    DUMP_HEX (DEBUG_ERROR, 0, UnpackedEventData, UnpackedEvent.EventSize, "[%a] -   ", __FUNCTION__);
    DEBUG ((DEBUG_ERROR, "\n"));
  }

  DEBUG_CODE_END ();
}

/**
  Returns the size of a TCG PCR Event 2 structure.

  @param[in]  TcgPcrEvent2     A pointer to a TCG PCR Event 2 structure.

  @return The size in bytes of the given event.

**/
UINTN
GetPcrEvent2Size (
  IN CONST  TCG_PCR_EVENT2  *TcgPcrEvent2
  )
{
  UINT32         DigestIndex;
  UINT32         DigestCount;
  TPMI_ALG_HASH  HashAlgo;
  UINT32         DigestSize;
  UINT8          *DigestBuffer;
  UINT32         EventSize;
  UINT8          *EventBuffer;

  DigestCount  = TcgPcrEvent2->Digest.count;
  HashAlgo     = TcgPcrEvent2->Digest.digests[0].hashAlg;
  DigestBuffer = (UINT8 *)&TcgPcrEvent2->Digest.digests[0].digest;
  for (DigestIndex = 0; DigestIndex < DigestCount; DigestIndex++) {
    DigestSize = GetHashSizeFromAlgo (HashAlgo);
    //
    // Prepare next
    //
    CopyMem (&HashAlgo, DigestBuffer + DigestSize, sizeof (TPMI_ALG_HASH));
    DigestBuffer = DigestBuffer + DigestSize + sizeof (TPMI_ALG_HASH);
  }

  DigestBuffer = DigestBuffer - sizeof (TPMI_ALG_HASH);

  CopyMem (&EventSize, DigestBuffer, sizeof (TcgPcrEvent2->EventSize));
  EventBuffer = DigestBuffer + sizeof (TcgPcrEvent2->EventSize);

  return (UINTN)EventBuffer + EventSize - (UINTN)TcgPcrEvent2;
}
