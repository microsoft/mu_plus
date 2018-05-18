/** @file

This library supports the SettingsManager XML schema


Copyright (C) 2015 Microsoft Corporation. All Rights Reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

**/

#ifndef __DFCI_XML_SETTING_SCHEMA_SUPPORT_LIB__
#define __DFCI_XML_SETTING_SCHEMA_SUPPORT_LIB__


/**
<Settings>
<Setting Type="">
<Id>%Id%</Id>
<Value>%Value%</Value>
</Setting>
...
</Settings>
**/
#define SETTINGS_PACKET_ELEMENT_NAME  "SettingsPacket"
#define SETTINGS_VERSION_ELEMENT_NAME "Version"
#define SETTINGS_LSV_ELEMENT_NAME     "LowestSupportedVersion"
#define SETTINGS_LIST_ELEMENT_NAME    "Settings"
#define SETTING_ELEMENT_NAME          "Setting"
#define SETTING_ID_ELEMENT_NAME       "Id"
#define SETTING_VALUE_ELEMENT_NAME    "Value"

/**
<Settings>
<SettingResult>
<Id>%Id%</Id>
<Flags>%HEX_FLAGS_VALUE%</Flags>
<Result>%HEX_STATUS_VALUE%</Result>
</SettingResult>
...
</Settings>
**/
#define RESULTS_PACKET_ELEMENT_NAME           "ResultsPacket"
#define RESULTS_APPLIED_ON_ELEMENT_NAME       "AppliedOn"
#define RESULTS_SETTINGS_LIST_ELEMENT_NAME    SETTINGS_LIST_ELEMENT_NAME
#define RESULTS_SETTING_ELEMENT_NAME          "SettingResult"
#define RESULTS_SETTING_ID_ELEMENT_NAME       "Id"
#define RESULTS_SETTING_FLAG_ELEMENT_NAME    "Flags"
#define RESULTS_SETTING_STATUS_ELEMENT_NAME   "Result"



#define CURRENT_PACKET_ELEMENT_NAME           "CurrentSettingsPacket"
#define CURRENT_DATE_ELEMENT_NAME             "Date"
#define CURRENT_LSV_ELEMENT_NAME              "LSV"
#define CURRENT_SETTINGS_LIST_ELEMENT_NAME    SETTINGS_LIST_ELEMENT_NAME
#define CURRENT_SETTING_ELEMENT_NAME          "SettingCurrent"
#define CURRENT_SETTING_ID_ELEMENT_NAME       "Id"
#define CURRENT_SETTING_VALUE_ELEMENT_NAME    "Value"



/**
Creates a new XmlNode list following the ResultPacket
format.

Return NULL if error occurs. Otherwise return a pointer to xml doc root element 
of a ResultPacketNodeList. 

List must be freed using FreeNodeList

**/
XmlNode *
EFIAPI
New_ResultPacketNodeList(EFI_TIME *Date);

XmlNode*
EFIAPI
GetSettingsPacketNode(CONST XmlNode* RootNode);

XmlNode*
EFIAPI
GetResultsPacketNode(CONST XmlNode* RootNode);

XmlNode*
EFIAPI
GetSettingsListNodeFromPacketNode(CONST XmlNode* PacketNode);

EFI_STATUS
EFIAPI
GetInputSettings(
  IN CONST XmlNode* ParentSettingNode,
  OUT CHAR8** Id, 
  OUT CHAR8** Value);

EFI_STATUS
EFIAPI
SetOutputSettingsStatus(
  IN CONST XmlNode* ParentSettingsListNode,
  IN CONST CHAR8* Id,
  IN CONST CHAR8* Result, 
  IN CONST CHAR8* Flags  OPTIONAL
  );

/**
Create a new Current Settings Packet Node List
**/
XmlNode *
EFIAPI
New_CurrentSettingsPacketNodeList(EFI_TIME *Date);

EFI_STATUS
EFIAPI
SetCurrentSettings(
  IN CONST XmlNode *ParentSettingsListNode,
  IN CONST CHAR8* Id,
  IN CONST CHAR8* Value);

XmlNode*
EFIAPI
GetCurrentSettingsPacketNode(
  IN CONST XmlNode* RootNode);

EFI_STATUS
EFIAPI
AddSettingsLsvNode(
  IN CONST XmlNode* CurrentSettingsPacketNode,
  IN CONST CHAR8* Lsv);



//***************************** EXAMPLE SETTINGS PACKET (INTPUT TO UEFI) *******************************//
/*
<?xml version="1.0" encoding="us-ascii"?>
<SettingsPacket xmlns="urn:UefiSettings-Schema">
  <CreatedBy>%UserName%</CreatedBy>
  <CreatedOn>%Date%</CreatedOn>
  <Version>%VersionNumber%</Version>
  <LowestSupportedVersion>%LowestSupportedVersionNumber%</LowestSupportedVersion>
  <Settings>
    <Setting Type="AssetTag">
      <!-- Asset Tag -->
      <Id>100</Id>
      <Value>7897897890</Value>
    </Setting>
    <Setting Type="SecureBootKey">
      <!-- Secure Boot Key Enum -->
      <Id>200</Id>
      <Value>MsOnly</Value>
    </Setting>
    <Setting Type="Enable">
      <!-- TPM Enable -->
      <Id>300</Id>
      <Value>Enabled</Value>
    </Setting>
    <Setting Type="Enable">
      <!-- Docking Station USB -->
      <Id>301</Id>
      <Value>Enabled</Value>
    </Setting>
  </Settings>
</SettingsPacket>
*/

//***************************** EXAMPLE RESULTS PACKET (OUTPUT FROM UEFI) *****************************//
/*
<?xml version="1.0" encoding="us-ascii"?>
<ResultsPacket xmlns="urn:UefiSettings-Schema">
  <AppliedOn>%Date%</AppliedOn>
  <Settings>
    <SettingResult>
      <!-- Asset Tag -->
      <Id>100</Id>
      <Flags>0x0000000000000001</Flags>
      <Result>0x8000000000000001</Result>
    </SettingResult>
    <SettingResult>
      <!-- TPM Enable -->
      <Id>300</Id>
      <Flags>0x0000000000000001</Flags>
      <Result>0x0</Result>
    </SettingResult>
  </Settings>
</ResultsPacket>
*/


#endif //__DFCI_XML_SETTING_SCHEMA_SUPPORT_LIB__
