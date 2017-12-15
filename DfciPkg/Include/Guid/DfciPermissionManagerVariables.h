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
#define DFCI_IDENTITY_AUTH_PROVISION_SIGNER_VAR_ATTRIBUTES    (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS) 

#define DFCI_PERMISSION_POLICY_APPLY_VAR_SIGNATURE    SIGNATURE_32('M','P','P','A')
#define DFCI_PERMISSION_POLICY_RESULT_VAR_SIGNATURE   SIGNATURE_32('M','P','P','R')

#define DFCI_PERMISSION_POLICY_VAR_VERSION (1)
#define MAX_ALLOWABLE_DFCI_PERMISSION_POLICY_VAR_INPUT_SIZE (1024 * 8)  //8kb 

#pragma warning(push)
#pragma warning(disable: 4200) // zero-sized array
#pragma pack (push, 1)

typedef struct {
  UINT32 HeaderSignature;   // 'M', 'P', 'P', 'A'
  UINT8  HeaderVersion;     // 1
  UINT8  rsvd[3];           // Not used
  UINT64 SerialNumber;      // Target a single device.  If 0 then it works on any device.  If non zero then it will only apply to a device with that serial number
  UINT32 SessionId;         // Unique session id tool generated
  UINT16 PayloadSize;       // Xml Payload size
  UINT8  Payload[];         // Xml Payload   <-- ConfigPacket
  //WIN_CERTIFICATE_UEFI_GUID Signature;  //Signature Auth Data PKCS7 - Hash covers all header with SessionId=0  (This is a dynamically sized structure with data appended to end of payload)
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
