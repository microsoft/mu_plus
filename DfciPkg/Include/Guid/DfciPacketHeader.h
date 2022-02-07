/** @file
DfciPacketHeader.h

This file defines the Header for all DFCI Mailbox Packets

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_PACKET_HEADER_H__
#define __DFCI_PACKET_HEADER_H__

#define MAX_ALLOWABLE_DFCI_APPLY_VAR_SIZE    (1024 * 24) // 24kb
#define MAX_ALLOWABLE_DFCI_RESULT_VAR_SIZE   (1024 *  8) //  8kb
#define MAX_ALLOWABLE_DFCI_CURRENT_VAR_SIZE  (1024 *  8) //  8kb

#pragma pack (push, 1)

// Nameless union is used to address all of the bytes of a packet.
typedef struct {
  union {
    UINT32    Signature;         // (4)
    UINT8     Pkt[1];            // Packet Data
  } Hdr;                         //
  UINT8    Version;              // (1)
} DFCI_PACKET_SIGNATURE;         // (5)

typedef struct {
  DFCI_PACKET_SIGNATURE    Sig;                 // (5) UINT32 Signature and UINT8 Version
  UINT8                    Identity;            // (1) Identity for Identity Packets, rsvd1 for others
  UINT8                    rsvd2;               // (1) Not used - should be 0
  UINT8                    rsvd3;               // (1) Not used - should be 0
  UINT32                   SessionId;           // (4) Unique id for this attempt.  This is zero when hashed
  UINT16                   SystemMfgOffset;     // (2) Offset to Mfg string in this structure. From SmbiosSystemManufacturer
  UINT16                   SystemProductOffset; // (2) Offset to Product string in this structure. From SmbiosSystemProductName
  UINT16                   SystemSerialOffset;  // (2) Offset to Serial Number string in this structure.  From SmbiosSystemSerialNumber
  UINT16                   PayloadSize;         // (2) Xml Payload size  - This is TrustedCertSize for Identity packets
  UINT16                   PayloadOffset;       // (2) offset to Payload - This is TrustedCert for Identity packets
} DFCI_PACKET_HEADER;            // (22)

#pragma pack (pop)

#endif // __DFCI_PACKET_HEADER_H__
