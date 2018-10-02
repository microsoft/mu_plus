/** @file
DfciApplyPacket.h

This protocol is used to apply general packets that have been processed by
DFciManager into the proper Identity, Permission, or Setting manager.

Copyright (c) 2018, Microsoft Corporation

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

#ifndef __DFCI_APPLY_PACKET_H__
#define __DFCI_APPLY_PACKET_H__

/**
Define the DFCI_APPLY_PACKET_PROTOCOL related structures
**/

typedef struct _DFCI_APPLY_PACKET_PROTOCOL DFCI_APPLY_PACKET_PROTOCOL;

#define DFCI_APPLY_PACKET_SIGNATURE     SIGNATURE_32('Y','P','P','A')
#define DFCI_APPLY_PACKET_VERSION      (1)

// Last Known Good operations
#define DFCI_LKG_RESTORE  (1)
#define DFCI_LKG_COMMIT   (2)

//
// DFCI Internal Packet
//
typedef enum {
    DFCI_PACKET_STATE_UNINITIALIZED = 0x00,
    DFCI_PACKET_STATE_DATA_PRESENT = 0x01,
    DFCI_PACKET_STATE_DATA_AUTHENTICATED = 0x02,
    DFCI_PACKET_STATE_DATA_USER_APPROVED = 0x03,
    DFCI_PACKET_STATE_DATA_APPLIED = 0x04,
    DFCI_PACKET_STATE_DATA_COMPLETE = 0x0F,   //Complete
    DFCI_PACKET_STATE_VERSION_ERROR = 0xF0,   //LSV blocked processing settings
    DFCI_PACKET_STATE_ABORTED = 0xF1,         //Aborted due to atomic fail
    DFCI_PACKET_STATE_ACCESS_DENIED = 0xF7,   //identity that signed doesn't have permission to update
    DFCI_PACKET_STATE_BAD_XML = 0xF8,         //Bad XML data.  Didn't follow rules
    DFCI_PACKET_STATE_DATA_NO_OWNER = 0xF9,   //Can't provision User Auth before having valid Owner Auth
    DFCI_PACKET_STATE_DATA_NOT_CORRECT_TARGET = 0xFA,  //SN target doesn't match device
    DFCI_PACKET_STATE_DATA_DELAYED_PROCESSING = 0xFB,  //needs delayed processing for ui or other reasons
    DFCI_PACKET_STATE_DATA_USER_REJECTED = 0xFC,
    DFCI_PACKET_STATE_DATA_INVALID = 0xFD,   //Need to delete var because of error condition
    DFCI_PACKET_STATE_DATA_AUTH_FAILED = 0xFE,
    DFCI_PACKET_STATE_DATA_SYSTEM_ERROR = 0xFF
} DFCI_PACKET_STATE;

typedef struct {
    // Parameter passed into decoder

    // The Packet is opaqe to DFCI.  Only the Decoder Library understands
    // the actual packet data, and transforms the packet into an internally usable
    // structure. While opaque, it is validated with signatures.
    DFCI_PACKET_SIGNATURE *Packet;            // (NeedToFree) Apply Packet
    UINTN                  PacketSize;        // Total size of the packet
    CONST CHAR16          *MailboxName;       // (Not in Packet) Name of mailbox
    CONST CHAR16          *ResultName;        // (Not in Packet) Name of result mailbox
    CONST EFI_GUID        *NameSpace;         // (Not in Packet) Namespace of Mailbox and Result
    DFCI_PACKET_SIGNATURE  Expected;          // Expected Signature

    // The following fields are populated by the PacketDecoder
    WIN_CERTIFICATE       *Signature;         // (Pkt) Full packet signature
    UINTN                  SignedDataLength;  // PacketSize - Signature Length (Derived)
    DFCI_PACKET_STATE      State;
    UINT32                 SessionId;
    BOOLEAN                DfciWildcard;      // Only allow OwnerSigned packets
    UINT8                 *VarIdentity;       // (Pkt) Identity
    UINT32                *Version;           // (Pkt) New Version
    UINT32                *LSV;               // (Pkt) New LSV
    BOOLEAN                V1Mode;            // Set Defaults as if V1 support

    // The following are return values
    EFI_STATUS             StatusCode;        // Status of Apply
    BOOLEAN                ResetRequired;
    BOOLEAN                LKGDirty;
    BOOLEAN                Unenroll;          // Process packet after Perms and Settings

    // The Payload.  TrustedCert for Idenity Packets, XML for Permissions and Settings Packets
    UINTN                  PayloadSize;
    CONST CHAR8           *Payload;           // (Pkt) Payload is in the Packet

    // Global State for the packet during processing, not an input or an output
    DFCI_AUTH_TOKEN        AuthToken;         // Temp Auth Token
    DFCI_IDENTITY_ID       DfciIdentity;      // Work DficIdentity
    BOOLEAN                UserConfirmationRequired;
    CHAR8                 *ResultXml;         // Settings Result Work Area
    UINTN                  ResultXmlSize;     //

    // The following targeting fields are only used by the DfciManager
    CHAR8                 *Manufacturer;      // (Pkt) String is in the Packet
    UINTN                  ManufacturerSize;  // Derived Manufacturer Size
    CHAR8                 *ProductName;       // (Pkt) String is in the Packet
    UINTN                  ProductNameSize;   // Derived Product Name Size
    CHAR8                 *SerialNumber;      // (Pkt) String is in the Packet
    UINTN                  SerialNumberSize;  // Derived Serial Number Size
}  DFCI_INTERNAL_PACKET;

/**
 *  Apply an Identity, a Permissions or a Settings packet  This same protocol will be
 *  published under three different guids, one from Identity Manager, one from
 *  PermissionsLib of the Settings Manager, and one from Settings Manager itself.
 *
 * @param[in]  This:           Apply Packet Protocol
 * @param[in]  ApplyPacket:    Pointer to buffer containing packet
 *
 * @retval EFI_SUCCESS -       Packet processed normally
 * @retval Error -             Severe error processing packet
 */
typedef
EFI_STATUS
(EFIAPI *DFCI_APPLY_PACKET_PROTOCOL_APPLY_PACKET) (
    IN  CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
    IN        DFCI_INTERNAL_PACKET        *Packet
);

/**
 *  Set the results for an Apply an Identity, a Permissions or a Settings packet
 *
 * @param[in]  This:           Apply Packet Protocol
 * @param[in]  ApplyPacket:    Pointer to buffer containing packet
 *
 * @retval EFI_SUCCESS -       Packet processed normally
 * @retval Error -             Severe error processing packet
 */
typedef
EFI_STATUS
(EFIAPI *DFCI_APPLY_PACKET_PROTOCOL_APPLY_RESULT) (
    IN  CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
    IN        DFCI_INTERNAL_PACKET        *Packet
);

/**
 *  Last Known Good operations
 *
 * @param[in] This:            Apply Packet Protocol
 * @param[in] Operation
 *                        SNAPSHOT  save the current settings to "LKG"
 *                        RESTORE   retores the current values from "LKG"
 *                        COMMIT    discards the current "LKG"
 *
 * @return EFI_STATUS EFIAPI
 * @return EFI_UNSUPPORTED  (For Apply Settings)
 */
typedef
EFI_STATUS
(EFIAPI *DFCI_APPLY_PACKET_PROTOCOL_LAST_KNOWN_GOOD) (
    IN  CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
    IN        DFCI_INTERNAL_PACKET        *Packet,
    IN        UINT8                        Operation
);

//
// Protocol definition
//
#pragma pack (push, 1)
struct _DFCI_APPLY_PACKET_PROTOCOL {
    UINT32     Signature;                       // 'Y', 'P', 'P', 'A'
    UINT8      Version;                         // 1
    UINT8      Rsvd[3];
    DFCI_APPLY_PACKET_PROTOCOL_APPLY_PACKET     ApplyPacket;
    DFCI_APPLY_PACKET_PROTOCOL_APPLY_RESULT     ApplyResult;
    DFCI_APPLY_PACKET_PROTOCOL_LAST_KNOWN_GOOD  Lkg;
};

#pragma pack (pop)

extern EFI_GUID     gDfciApplyPermissionsProtocolGuid;
extern EFI_GUID     gDfciApplySettingsProtocolGuid;
extern EFI_GUID     gDfciApplyIdentityProtocolGuid;

#endif // __DFCI_APPLY_PACKET_H__
