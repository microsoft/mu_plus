/** @file
DfciPermissionManagerVariables.h

Contains definitions for PermissionManager variables. This allows a tool to
set/remove Policy Permissions on the device.  These permissions are how access
is controlled to different settings.

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

#ifndef __DFCI_PERMISSION_MANAGER_VARIABLES_H__
#define __DFCI_PERMISSION_MANAGER_VARIABLES_H__

#include <Guid/DfciPacketHeader.h>

//
// Variable name-space
//
extern EFI_GUID gDfciPermissionManagerVarNamespace;

#define DFCI_PERMISSION_POLICY_CURRENT_VAR_NAME  L"DfciPermissionCurrent"
#define DFCI_PERMISSION_POLICY_APPLY_VAR_NAME    L"DfciPermissionApply"
#define DFCI_PERMISSION_POLICY_RESULT_VAR_NAME   L"DfciPermissionResult"
#define DFCI_PERMISSION2_POLICY_APPLY_VAR_NAME   L"DfciPermission2Apply"
#define DFCI_PERMISSION2_POLICY_RESULT_VAR_NAME  L"DfciPermission2Result"
#define DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES    (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

#define DFCI_PERMISSION_POLICY_APPLY_VAR_SIGNATURE    SIGNATURE_32('M','P','P','A')
#define DFCI_PERMISSION_POLICY_RESULT_VAR_SIGNATURE   SIGNATURE_32('M','P','P','R')

#define DFCI_PERMISSION_POLICY_VAR_VERSION    (2)

#define DFCI_PERMISSION_POLICY_RESULT_VERSION_V1 (1)
#define DFCI_PERMISSION_POLICY_RESULT_VERSION    (2)

#pragma pack (push, 1)

typedef struct {
  DFCI_PACKET_HEADER Header;  // Signature =  'M', 'P', 'P', 'A'
                              // Version = 2
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
} DFCI_PERMISSION_POLICY_APPLY_VAR;

typedef struct {
  DFCI_PACKET_SIGNATURE Header;  // Signature =  // 'M', 'P', 'P', 'R'
                              // Version = 1
  UINT8  rsvd[3];             // Not used
  UINT64 Status;              // Global Status of the request.  SUCCESS here means XML was parsed and Permissions applied.  ERROR means XML was not parsed.
  UINT32 SessionId;           // Unique session id tool generated  -- matches the incoming apply var
} DFCI_PERMISSION_POLICY_RESULT_VAR_V1;

typedef struct {
  DFCI_PACKET_SIGNATURE Header;  // Signature =  // 'M', 'P', 'P', 'R'
                              // Version = 2
  UINT8  rsvd[3];             // Not used
  UINT64 Status;              // Global Status of the request.  SUCCESS here means XML was parsed and Permissions applied.  ERROR means XML was not parsed.
  UINT32 SessionId;           // Unique session id tool generated  -- matches the incoming apply var
  UINT16 PayloadSize;         // Size of Xml Payload
  UINT8  Payload[];           // Xml Payload <-- ResultConfigPacket
} DFCI_PERMISSION_POLICY_RESULT_VAR;

#pragma pack (pop)


#endif // __DFCI_PERMISSION_MANAGER_VARIABLES_H__
