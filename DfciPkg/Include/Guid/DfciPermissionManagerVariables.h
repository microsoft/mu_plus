/** @file
Contains definitions for PermissionManager variables. This 
allows a tool to set/remove Policy Permissions on the device.  These permisions
are how acccess is controlled to different settings.    


Copyright (c) 2015, Microsoft Corporation. All rights reserved.

**/

#ifndef __DFCI_PERMISSION_MANAGER_VARIABLES_H__
#define __DFCI_PERMISSION_MANAGER_VARIABLES_H__

//
// Variable namespace
//
extern EFI_GUID gDfciPermissionManagerVarNamespace;

#define DFCI_PERMISSION_POLICY_APPLY_VAR_NAME   L"UEFIPermissionApply"
#define DFCI_PERMISSION_POLICY_RESULT_VAR_NAME  L"UEFIPermissionResult"
#define DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES    (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

#define DFCI_PERMISSION_POLICY_APPLY_VAR_SIGNATURE    SIGNATURE_32('M','P','P','A')
#define DFCI_PERMISSION_POLICY_RESULT_VAR_SIGNATURE   SIGNATURE_32('M','P','P','R')

#define DFCI_PERMISSION_POLICY_VAR_VERSION_V1 (1)
#define DFCI_PERMISSION_POLICY_VAR_VERSION_V2 (2)

#define DFCI_PERMISSION_POLICY_RESULT_VERSION (1)

#define MAX_ALLOWABLE_DFCI_PERMISSION_POLICY_VAR_INPUT_SIZE (1024 * 16)  //16kb

#pragma warning(push)
#pragma warning(disable: 4200) // zero-sized array
#pragma pack (push, 1)

typedef struct {
  UINT32 HeaderSignature;   // 'M', 'P', 'P', 'A'
  UINT8  HeaderVersion;     // 1
} DFCI_PERMISSION_POLICY_APPLY_VAR_HEADER;

typedef struct {
  UINT32 HeaderSignature;   // 'M', 'P', 'P', 'A'
  UINT8  HeaderVersion;     // 1
  UINT8  rsvd[3];           // Not used
  UINT64 SerialNumber;      // Target a single device.  If 0 then it works on any device.  If non zero then it will only apply to a device with that serial number
  UINT32 SessionId;         // Unique session id tool generated
  UINT16 PayloadSize;       // Xml Payload size
  UINT8  Payload[];         // Xml Payload   <-- ConfigPacket
  //WIN_CERTIFICATE_UEFI_GUID Signature;  //Signature Auth Data PKCS7 - Hash covers all header with SessionId=0  (This is a dynamically sized structure with data appended to end of payload)
} DFCI_PERMISSION_POLICY_APPLY_VAR_V1;

typedef struct {
  UINT32 HeaderSignature;     // 'M', 'P', 'P', 'A'
  UINT8  HeaderVersion;       // 2
  UINT8  rsvd[3];             // Not used
  EFI_GUID SystemUuid;        // From SmbiosUuid
  UINT32 SessionId;           // Unique id for this attempt.  This is zero when hashed
  UINT16 SystemMfgOffset;     // Offset to Mfg string in this structure. From SmbiosSystemManufacturer
  UINT16 SystemProductOffset; // Offset to Product string in this structure. From SmbiosSystemProductName
  UINT16 SystemSerialOffset;  // Offset to Serial Number string in this structure.  From SmbiosSystemSerialNumber
  UINT16 PayloadSize;         // Xml Payload size
  UINT16 PayloadOffset;       // offset to Payload
  UINT8  SmBiosStrings[];     // Where SmBios strings start
  //
  // The strings, payload, and certificate MUST be in this order in order to use the Offsets to determine the max
  // string size.  eg. StrSize(MfgName) == SystemProductOffset-SystemMfgOffset.
  //
  // CHAR8 MfgName            // NULL terminated MfgName
  // CHAR8 ProductName        // NULL terminated ProductName
  // CHAR8 SerialNumber       // NULL terminated SerialNumber
  // UINT8 Payload            // Xml Payload <--- ConfigPacket
  // WIN_CERTIFICATE_UEFI_GUID Signature;  //Signature Auth Data PKCS7 - Hash covers all header with SessionId=0
} DFCI_PERMISSION_POLICY_APPLY_VAR_V2;

typedef union {
   DFCI_PERMISSION_POLICY_APPLY_VAR_HEADER vh;
   DFCI_PERMISSION_POLICY_APPLY_VAR_V1     v1;
   DFCI_PERMISSION_POLICY_APPLY_VAR_V2     v2;
} DFCI_PERMISSION_POLICY_APPLY_VAR;

typedef struct {
  UINT32 HeaderSignature;   // 'M', 'P', 'P', 'R'
  UINT8  HeaderVersion;
  UINT8  rsvd[3];
  UINT64 Status;            // Global Status of the request.  SUCCESS here means XML was parsed and Permissions applied.  ERROR means XML was not parsed.  
  UINT32 SessionId;         // Unique session id tool generated  -- matches the incomming apply var
} DFCI_PERMISSION_POLICY_RESULT_VAR;

#pragma pack (pop)
#pragma warning(pop)


#endif // __DFCI_PERMISSION_MANAGER_VARIABLES_H__
