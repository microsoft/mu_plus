/** @file

This library supports the Device IdSetting Lib and the
Device Id XML schema

Copyright (C) 2018 Microsoft Corporation. All Rights Reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

**/

#ifndef __DFCI_XML_DEVICE_ID_SCHEMA_SUPPORT_LIB__
#define __DFCI_XML_DEVICE_ID_SCHEMA_SUPPORT_LIB__


#define DEVICE_ID_PACKET_ELEMENT_NAME           "UEFIDeviceIdentifierPacket"
#define DEVICE_ID_DFCI_VERSION_ELEMENT_NAME     "DfciVersion"
#define DEVICE_ID_LIST_ELEMENT_NAME             "Identifiers"
#define DEVICE_ID_ELEMENT_NAME                  "Identifier"
#define DEVICE_ID_ID_ELEMENT_NAME               "Id"
#define DEVICE_ID_VALUE_ELEMENT_NAME            "Value"

#define DEVICE_ID_MANUFACTURER                  "Manufacturer"
#define DEVICE_ID_PRODUCT_NAME                  "Product Name"
#define DEVICE_ID_SERIAL_NUMBER                 "Serial Number"
#define DEVICE_ID_UUID                          "UUID"


XmlNode*
EFIAPI
GetDeviceIdPacketNode(
  IN CONST XmlNode* RootNode);

XmlNode*
EFIAPI
GetDeviceIdListNodeFromPacketNode(
    IN CONST XmlNode* PacketNode);

//***************************** EXAMPLE PERMISSION PACKET (INTPUT TO UEFI) *******************************//
/*
<?xml version="1.0" encoding="utf-8"?>
<UEFIDeviceIdentifierPacket>
  <DfciVersion>2</DfciVersion>
  <Identifiers>
    <Identifier>
      <Id>Manufacturer</Id>
      <Value>Microsoft</Value>
    </Identifier>
    <Identifier>
      <Id>Product Name</Id>
      <Value>Microsoft Surface Pro</Value>
    </Identifier>
    <Identifier>
      <Id>Serial Number</Id>
      <Value>40001234567</Value>
    </Identifier>
    <Identifier>
      <Id>UUID</Id>
      <Value>8a0aef87-74e2-48ad-a105-bbe07395d54d</Value>
    </Identifier>
  </Identifiers>
</UEFIDeviceIdentifierPacket>
                                                      >

/**
Create a new Device Id Packet Node List
**/
XmlNode *
EFIAPI
New_DeviceIdPacketNodeList(VOID);

/**
 * Add the current DFCI Version
 *
 * @param IdPacketNode
 * @param DfciVersion
 */
EFI_STATUS
EFIAPI
AddDfciVersionNode(
  IN CONST XmlNode *IdPacketNode,
  IN CONST CHAR8   *DfciVersion);

/**
 * Set Device Id Element
 *
 *
 * @param ParentIdentifiersListNode
 * @param Id
 * @param Value
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
SetDeviceIdIdentifier (
  IN CONST XmlNode *ParentIdentifiersListNode,
  IN CONST CHAR8 *Id,
  IN CONST CHAR8 *Value);

#endif //__DFCI_XML_DEVICE_ID_SCHEMA_SUPPORT_LIB__
