/** @file

This library supports the SettingPermission Lib and the   
Permission XML schema  

Copyright (C) 2015 Microsoft Corporation. All Rights Reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

**/

#ifndef __DFCI_XML_PERMISSION_SCHEMA_SUPPORT_LIB__
#define __DFCI_XML_PERMISSION_SCHEMA_SUPPORT_LIB__


#define PERMISSIONS_PACKET_ELEMENT_NAME  "PermissionsPacket"
#define PERMISSIONS_VERSION_ELEMENT_NAME "Version"
#define PERMISSIONS_LSV_ELEMENT_NAME     "LowestSupportedVersion"
#define PERMISSIONS_LIST_ELEMENT_NAME    "Permissions"
#define PERMISSIONS_LIST_DEFAULT_ATTRIBUTE_NAME "Default"
#define PERMISSIONS_LIST_APPEND_ATTRIBUTE_NAME  "Append"
#define PERMISSION_ELEMENT_NAME          "Permission"
#define PERMISSION_ID_ELEMENT_NAME       "Id"
#define PERMISSION_MASK_VALUE_ELEMENT_NAME    "PMask"




XmlNode*
EFIAPI
GetPermissionPacketNode(
  IN CONST XmlNode* RootNode);


XmlNode*
EFIAPI
GetPermissionsListNodeFromPacketNode(
  IN CONST XmlNode* PacketNode);

EFI_STATUS
EFIAPI
GetPermissionsListDefaultPMask(
  IN CONST XmlNode      *PermissionListNode,
  OUT DFCI_PERMISSION_MASK  *PMask);

  /**
  Returns true if Permission Entries should be
  appended to existing Permission List
  **/
  EFI_STATUS
  EFIAPI
  PermissionListEntriesAppend(
    IN CONST XmlNode *PermissionListNode,
    OUT BOOLEAN        *Result);

EFI_STATUS
EFIAPI
GetInputPermission(
  IN CONST XmlNode* ParentPermissionNode,
  OUT DFCI_SETTING_ID_ENUM *Id,
  OUT DFCI_PERMISSION_MASK *PMask);



//***************************** EXAMPLE PERMISSION PACKET (INTPUT TO UEFI) *******************************//
/*
<?xml version="1.0" encoding="utf-8"?>
<PermissionsPacket xmlns="urn:UefiSettings-Schema">
  <CreatedBy>%UserName%</CreatedBy>
  <CreatedOn>%Date%</CreatedOn>
  <Version>%VersionNumber%</Version>
  <LowestSupportedVersion>%LowestSupportedVersionNumber%</LowestSupportedVersion>
  <Permissions Default="%PMASK%" Append="%True|False%">
    <Permission>
      <!-- Asset Tag -->
      <Id>100</Id>
      <PMask>0x00</PMask>
    </Permission>
    <Permission>
      <Id>300</Id>
      <PMask>0x81</PMask>
    </Permission>
  </Permissions>
</PermissionPacket>
*/

#endif //__DFCI_XML_PERMISSION_SCHEMA_SUPPORT_LIB__
