/** @file
DfciDeviceIdSupportLib.h

Library supports getting the device Id elements.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_XML_IDENTITY_CURRENT_SCHEMA_SUPPORT_LIB__
#define __DFCI_XML_IDENTITY_CURRENT_SCHEMA_SUPPORT_LIB__

#define IDENTITY_CURRENT_PACKET_ELEMENT_NAME   "UEFIIdentityCurrentPacket"
#define IDENTITY_CURRENT_VERSION_ELEMENT_NAME  "Version"
#define IDENTITY_CURRENT_LIST_ELEMENT_NAME     "Certificates"
#define IDENTITY_CURRENT_ELEMENT_NAME          "Certificate"
#define IDENTITY_CURRENT_ID_ELEMENT_NAME       "Id"
#define IDENTITY_CURRENT_VALUE_ELEMENT_NAME    "Value"
#define IDENTITY_CURRENT_ZTD_CERT_NAME         "ZeroTouch"
#define IDENTITY_CURRENT_OWNER_CERT_NAME       "Owner"
#define IDENTITY_CURRENT_USER_CERT_NAME        "User"
#define IDENTITY_CURRENT_USER1_CERT_NAME       "User1"
#define IDENTITY_CURRENT_USER2_CERT_NAME       "User2"
#define IDENTITY_CURRENT_NO_CERTIFICATE_VALUE  "Cert not installed"
#define IDENTITY_CURRENT_THUMBPRINT_NAME       "Thumbprint"

XmlNode *
EFIAPI
GetIdentityCurrentPacketNode (
  IN CONST XmlNode  *RootNode
  );

XmlNode *
EFIAPI
GetIdentityCurrentListNodeFromPacketNode (
  IN CONST XmlNode  *PacketNode
  );

// ***************************** EXAMPLE IDENTITY CURRENT PACKET (OUTPUT FROM UEFI) *******************************//

/*
<?xml version="1.0" encoding="utf-8"?>
<UEFIIdentityCurrentPacket>
  <Version>2</Version>
  <Certificates>
    <Certificate>
      <Id>Owner</Id>
      <Thumbprint>45 d6 42 7a 83 9c ef 48 fa 36 c5 bc 0a 4a 27 c1 6f c5 72 f7</Thumbprint>
    </Certificate>
    <Certificate>
      <Id>User</Id>
      <Thumbprint>Cert not installed</Thumbprint>
    </Certificate>
    <Certificate>
      <Id>User1</Id>
      <Thumbprint>Cert not installed</Thumbprint>
    </Certificate>
    <Certificate>
      <Id>User2</Id>
      <Thumbprint>Cert not installed</Thumbprint>
    </Certificate>
  </Certificates>
</UEFIIdentityCurrentPacket>
**/

/**
Create a new Device Id Packet Node List
**/
XmlNode *
EFIAPI
New_IdentityCurrentPacketNodeList (
  VOID
  );

/**
 * Add the current DFCI Version
 *
 * @param IdPacketNode
 * @param DfciVersion
 */
EFI_STATUS
EFIAPI
AddVersionNode (
  IN CONST XmlNode  *IdPacketNode,
  IN CONST CHAR8    *Version
  );

/**
 * Set Identity Certificate Element
 *
 *
 * @param ParentCertificateListNode
 * @param Id
 * @param Thumbprint
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
SetIdentityCurrentCertificate (
  IN CONST XmlNode  *ParentCertificateListNode,
  IN CONST CHAR8    *Id,
  IN CONST CHAR8    *Thumbprint
  );

#endif //__DFCI_XML_DEVICE_ID_SCHEMA_SUPPORT_LIB__
