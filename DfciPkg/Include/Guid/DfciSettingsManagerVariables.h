/** @file
DfciSettingsManagerVariables.h

Contains definitions for SettingsManager variables.

These variables are used to provision or change the Device Settings

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/** @file
Contains definitions for SettingsManager variables.

Copyright (c) 2015, Microsoft Corporation. All rights reserved.

**/

#ifndef __DFCI_SETTINGS_MANAGER_VARIABLES_H__
#define __DFCI_SETTINGS_MANAGER_VARIABLES_H__

#include <Guid/DfciPacketHeader.h>

//
// Variable name-space
//
extern EFI_GUID  gDfciSettingsManagerVarNamespace;

#define DFCI_SETTINGS_APPLY_INPUT_VAR_NAME     L"DfciSettingsRequest"
#define DFCI_SETTINGS_APPLY_OUTPUT_VAR_NAME    L"DfciSettingsResult"
#define DFCI_SETTINGS2_APPLY_INPUT_VAR_NAME    L"DfciSettings2Request"
#define DFCI_SETTINGS2_APPLY_OUTPUT_VAR_NAME   L"DfciSettings2Result"
#define DFCI_SETTINGS_CURRENT_OUTPUT_VAR_NAME  L"DfciSettingsCurrent"

#define DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES  (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

#define DFCI_SECURED_SETTINGS_APPLY_VAR_SIGNATURE   SIGNATURE_32('M','S','S','A')
#define DFCI_SECURED_SETTINGS_RESULT_VAR_SIGNATURE  SIGNATURE_32('M','S','S','R')

#define DFCI_SECURED_SETTINGS_VAR_VERSION  (2)

#define DFCI_SECURED_SETTINGS_RESULTS_VERSION  (1)

#pragma pack (push, 1)

typedef struct {
  DFCI_PACKET_HEADER    Header;          // Signature = 'M', 'S', 'S', 'A'
                                         // Version = 2
  UINT8                 SmBiosStrings[]; // Where SmBios strings start
  //
  // The strings and certificate MUST be in this order in order to use the Offsets to determine the max
  // string size.  eg. StrSize(MfgName) == SystemProductOffset-SystemMfgOffset.
  //
  // CHAR8 MfgName             // NULL terminated MfgName
  // CHAR8 ProductName         // NULL terminated ProductName
  // CHAR8 SerialNumber        // NULL terminated SerialNumber
  // UINT8 Payload             // Xml Payload <--- ConfigPacket
  // WIN_CERTIFICATE_UEFI_GUID Signature;  //Signature Auth Data PKCS7 - Hash covers all header with SessionId=0  (This is a dynamically sized structure with data appended to end of payload)
} DFCI_SECURED_SETTINGS_APPLY_VAR;

typedef struct {
  DFCI_PACKET_SIGNATURE    Header;      // Signature = 'M', 'S', 'S', 'R'
                                        // Version = 1
  UINT8                    rsvd[3];     // Not used
  UINT64                   Status;      // Global Status of the request.  SUCCESS here means XML was parsed and payload contains detailed result.  ERROR means XML was not parsed.
  UINT32                   SessionId;   // Unique session id tool generated  -- matches the incoming apply var
  UINT16                   PayloadSize; // Size of Xml Payload
  UINT8                    Payload[];   // Xml Payload <-- ResultConfigPacket
} DFCI_SECURED_SETTINGS_RESULT_VAR;

#pragma pack (pop)

#endif // __DFCI_SETTINGS_MANAGER_VARIABLES_H__
