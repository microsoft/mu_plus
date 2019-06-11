/** @file
DfciPacketHeader.h

This file defines the Header for all DFCI Mailbox Packets

Copyright (c), Microsoft Corporation

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#ifndef __DFCI_PACKET_HEADER_H__
#define __DFCI_PACKET_HEADER_H__

#define MAX_ALLOWABLE_DFCI_APPLY_VAR_SIZE   (1024 * 24) // 24kb
#define MAX_ALLOWABLE_DFCI_RESULT_VAR_SIZE  (1024 *  8) //  8kb
#define MAX_ALLOWABLE_DFCI_CURRENT_VAR_SIZE (1024 *  8) //  8kb

#pragma pack (push, 1)

//Nameless union is used to address all of the bytes of a packet.
typedef struct {
    union {
        UINT32 Signature;        // (4)
        UINT8  Pkt[1];           // Packet Data
    } Hdr;                       //
    UINT8  Version;              // (1)
} DFCI_PACKET_SIGNATURE;         // (5)

typedef struct {
    DFCI_PACKET_SIGNATURE Sig;   // (5) UINT32 Signature and UINT8 Version
    UINT8  Identity;             // (1) Identity for Identity Packets, rsvd1 for others
    UINT8  rsvd2;                // (1) Not used - should be 0
    UINT8  rsvd3;                // (1) Not used - should be 0
    UINT32 SessionId;            // (4) Unique id for this attempt.  This is zero when hashed
    UINT16 SystemMfgOffset;      // (2) Offset to Mfg string in this structure. From SmbiosSystemManufacturer
    UINT16 SystemProductOffset;  // (2) Offset to Product string in this structure. From SmbiosSystemProductName
    UINT16 SystemSerialOffset;   // (2) Offset to Serial Number string in this structure.  From SmbiosSystemSerialNumber
    UINT16 PayloadSize;          // (2) Xml Payload size  - This is TrustedCertSize for Identity packets
    UINT16 PayloadOffset;        // (2) offset to Payload - This is TrustedCert for Identity packets
} DFCI_PACKET_HEADER;            //(22)

#pragma pack (pop)

#endif // __DFCI_PACKET_HEADER_H__
