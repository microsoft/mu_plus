/** @file
Contains definitions for SettingsManager variables.   

Copyright (c) 2015, Microsoft Corporation. All rights reserved.

**/

#ifndef __DFCI_SETTINGS_MANAGER_VARIABLES_H__
#define __DFCI_SETTINGS_MANAGER_VARIABLES_H__

//
// Variable namespace
//
extern EFI_GUID gDfciSettingsManagerVarNamespace;

#define XML_SETTINGS_APPLY_INPUT_VAR_NAME    L"UEFISettingsRequest"
#define XML_SETTINGS_APPLY_OUTPUT_VAR_NAME   L"UEFISettingsResult"
#define XML_SETTINGS_CURRENT_OUTPUT_VAR_NAME L"UEFISettingsCurrent"

#define DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES    (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS) 

#define DFCI_SECURED_SETTINGS_APPLY_VAR_SIGNATURE     SIGNATURE_32('M','S','S','A')
#define DFCI_SECURED_SETTINGS_RESULT_VAR_SIGNATURE    SIGNATURE_32('M','S','S','R')

#define DFCI_SECURED_SETTINGS_VAR_VERSION (1)


#define MAX_ALLOWABLE_VAR_INPUT_SIZE (1024 * 8)  //8kb
#define MAX_ALLOWABLE_OUTPUT_PAYLOAD_SIZE (1024 * 4) // 4kb

#pragma warning(push)
#pragma warning(disable: 4200) // zero-sized array
#pragma pack (push, 1)

typedef struct {
  UINT32 HeaderSignature;   // 'M', 'S', 'S', 'A'
  UINT8  HeaderVersion;     // 1
  UINT8  rsvd[3];           // Not used
  UINT64 SerialNumber;      // Target a single device.  If 0 then it works on any device.  If non zero then it will only apply to a device with that serial number
  UINT32 SessionId;         // Unique session id tool generated
  UINT16 PayloadSize;       // Xml Payload size
  UINT8  Payload[];         // Xml Payload   <-- ConfigPacket
  //WIN_CERTIFICATE_UEFI_GUID Signature;  //Signature Auth Data PKCS7 - Hash covers all header with SessionId=0  (This is a dynamically sized structure with data appended to end of payload)
} DFCI_SECURED_SETTINGS_APPLY_VAR;

typedef struct {
  UINT32 HeaderSignature;   // 'M', 'S', 'S', 'R'
  UINT8  HeaderVersion;
  UINT8  rsvd[3];
  UINT64 Status;            // Global Status of the request.  SUCCESS here means XML was parsed and payload contains detailed result.  ERROR means XML was not parsed.  
  UINT32 SessionId;         // Unique session id tool generated  -- matches the incomming apply var
  UINT16 PayloadSize;
  UINT8  Payload[];         // Xml Payload <-- ResultConfigPacket
} DFCI_SECURED_SETTINGS_RESULT_VAR;

#pragma pack (pop)
#pragma warning(pop)


#endif // __DFCI_SETTINGS_MANAGER_VARIABLES_H__
