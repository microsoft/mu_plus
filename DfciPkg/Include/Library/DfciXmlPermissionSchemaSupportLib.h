/** @file
DfciXmlPermissionSchemaSupport.h

This library supports the SettingPermission Lib and the Permission XML schema

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_XML_PERMISSION_SCHEMA_SUPPORT_LIB__
#define __DFCI_XML_PERMISSION_SCHEMA_SUPPORT_LIB__

#define PERMISSIONS_PACKET_ELEMENT_NAME               "PermissionsPacket"
#define PERMISSIONS_VERSION_ELEMENT_NAME              "Version"
#define PERMISSIONS_LSV_ELEMENT_NAME                  "LowestSupportedVersion"
#define PERMISSIONS_LIST_ELEMENT_NAME                 "Permissions"
#define PERMISSIONS_LIST_DELEGATED_ATTRIBUTE_NAME     "Delegated"
#define PERMISSIONS_LIST_DEFAULT_ATTRIBUTE_NAME       "Default"
#define PERMISSIONS_LIST_APPEND_ATTRIBUTE_NAME        "Append"
#define PERMISSIONS_LIST_APPEND_ATTRIBUTE_TRUE_VALUE  "True"
#define PERMISSION_ELEMENT_NAME                       "Permission"
#define PERMISSION_ID_ELEMENT_NAME                    "Id"
#define PERMISSION_MASK_VALUE_ELEMENT_NAME            "PMask"
#define PERMISSION_DELEGATED_MASK_VALUE_ELEMENT_NAME  "DMask"

/**
<Permissions>
<PermissionResult>
<Id>%Id%</Id>
<Flags>%HEX_FLAGS_VALUE%</Flags>
<Result>%HEX_STATUS_VALUE%</Result>
</PermissionResult>
...
</Permissions>
**/
#define RESULTS_PACKET_ELEMENT_NAME              "ResultsPacket"
#define RESULTS_APPLIED_ON_ELEMENT_NAME          "AppliedOn"
#define RESULTS_PERMISSIONS_LIST_ELEMENT_NAME    PERMISSIONS_LIST_ELEMENT_NAME
#define RESULTS_PERMISSIONS_ELEMENT_NAME         "PermissionResult"
#define RESULTS_PERMISSIONS_ID_ELEMENT_NAME      "Id"
#define RESULTS_PERMISSIONS_STATUS_ELEMENT_NAME  "Result"

#define CURRENT_PERMISSION_PACKET_ELEMENT_NAME  "CurrentPermissionsPacket"
#define CURRENT_PERMISSION_DATE_ELEMENT_NAME    "Date"
#define CURRENT_PERMISSION_LSV_ELEMENT_NAME     "LSV"
#define CURRENT_PERMISSION_LIST_ELEMENT_NAME    PERMISSIONS_LIST_ELEMENT_NAME
#define CURRENT_PERMISSION_ELEMENT_NAME         "PermissionCurrent"
#define CURRENT_PERMISSION_ID_ELEMENT_NAME      "Id"
#define CURRENT_PERMISSION_VALUE_ELEMENT_NAME   "PMask"

XmlNode *
EFIAPI
GetPermissionPacketNode (
  IN CONST XmlNode  *RootNode
  );

XmlNode *
EFIAPI
GetCurrentPermissionsPacketNode (
  IN CONST XmlNode  *RootNode
  );

XmlNode *
EFIAPI
GetPermissionsListNodeFromPacketNode (
  IN CONST XmlNode  *PacketNode
  );

/**
 * Get Permissin attributes DefaultPMask and DefaultMask
 *
 * Set the input values to their default before calling this function.  If the
 * values are not defined in the XML, the PMask or DMask will not be disturbed.
 *
 * @param PermissionListNode
 * @param PMask
 * @param DMask
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
GetPermissionsListDefaultPMask (
  IN CONST XmlNode          *PermissionListNode,
  OUT DFCI_PERMISSION_MASK  *PMask,
  OUT DFCI_PERMISSION_MASK  *DMask
  );

/**
  Returns true if Permission Entries should be
  appended to existing Permission List
  **/
EFI_STATUS
EFIAPI
PermissionListEntriesAppend (
  IN CONST XmlNode  *PermissionListNode,
  OUT BOOLEAN       *Result
  );

EFI_STATUS
EFIAPI
GetInputPermission (
  IN CONST XmlNode            *ParentPermissionNode,
  OUT DFCI_SETTING_ID_STRING  *Id,
  OUT DFCI_PERMISSION_MASK    *PMask,
  OUT DFCI_PERMISSION_MASK    *DMask
  );

// ***************************** EXAMPLE PERMISSION PACKET (INTPUT TO UEFI) *******************************//

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

/**
Create a new Current Permissions Packet Node List
**/
XmlNode *
EFIAPI
New_CurrentPermissionsPacketNodeList (
  EFI_TIME  *Date
  );

EFI_STATUS
EFIAPI
SetCurrentPermissions (
  IN CONST XmlNode  *ParentPermissionsListNode,
  IN CONST CHAR8    *Id,
  IN CONST UINT8    Value,
  IN CONST UINT8    DelegatedValue
  );

EFI_STATUS
EFIAPI
AddPermissionsLsvNode (
  IN CONST XmlNode  *CurrentPermissionsPacketNode,
  IN CONST CHAR8    *Lsv
  );

EFI_STATUS
EFIAPI
AddCurrentAttributes (
  IN CONST XmlNode  *CurrentPermissionsPacketNode,
  IN CONST UINT8    Value,
  IN CONST UINT8    DelegatedValue
  );

/**
Creates a new XmlNode list following the ResultPacket
format.

Return NULL if error occurs. Otherwise return a pointer to xml doc root element
of a ResultPacketNodeList.

List must be freed using FreeNodeList

**/
XmlNode *
EFIAPI
New_ResultPermissionPacketNodeList (
  EFI_TIME  *Date
  );

XmlNode *
EFIAPI
GetResultsPermissionPacketNode (
  CONST XmlNode  *RootNode
  );

EFI_STATUS
EFIAPI
SetOutputPermissionStatus (
  IN CONST XmlNode  *ParentPermissionsListNode,
  IN CONST CHAR8    *Id,
  IN CONST CHAR8    *Result
  );

#endif //__DFCI_XML_PERMISSION_SCHEMA_SUPPORT_LIB__
