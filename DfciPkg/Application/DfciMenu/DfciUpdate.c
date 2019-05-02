/** @file
DfciUpdate.c

This module will request and apply a new DFCI configuration.

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

// A file on a USB key is limted to 255 characters.  This code will generate a filename
// based on the Serial Number, Model, and Manaufacturer strings concatenated with "_",
// and truncated to 250 characters.  The file name extensions will be
//
//   .Dfi  -- Dfci Update Package
//
//  After assembling the filename, each character is inspected for invalid characters.  The
//  following are invalid characters:
//
//  Any binary value of 0x01-0x1f, and any of    " * / : < > ? \ |
//
//  All invalid characters are changed to @
//
// The packet from a network contains the exact same contents as the USB update.

#include <Uefi.h>

#include <Guid/DfciPacketHeader.h>
#include <Guid/DfciIdentityAndAuthManagerVariables.h>
#include <Guid/DfciPermissionManagerVariables.h>
#include <Guid/DfciSettingsManagerVariables.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DfciDeviceIdSupportLib.h>
#include <Library/JsonLiteParser.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "DfciPrivate.h"
#include "DfciUtility.h"
#include "DfciUsb.h"
#include "DfciRequest.h"
#include "DfciUpdate.h"

STATIC JSON_SET_VARIABLE_TABLE_ENTRY mJsonSetVariableEntryMailbox[] = {
    { //  JSON_SET_IDENTITY
      DFCI_IDENTITY_APPLY_VAR_NAME,
     &gDfciAuthProvisionVarNamespace,
      DFCI_IDENTITY_VAR_ATTRIBUTES,
      DFCI_IDENTITY_APPLY_VAR_SIGNATURE },

    { // JSON_SET_IDENTITY2
      DFCI_IDENTITY2_APPLY_VAR_NAME,
     &gDfciAuthProvisionVarNamespace,
      DFCI_IDENTITY_VAR_ATTRIBUTES,
      DFCI_IDENTITY_APPLY_VAR_SIGNATURE },

    { // JSON_SET_PERMISSIONS
      DFCI_PERMISSION_POLICY_APPLY_VAR_NAME,
     &gDfciPermissionManagerVarNamespace,
      DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES,
      DFCI_PERMISSION_POLICY_APPLY_VAR_SIGNATURE },

    { // JSON_SET_PERMISSIONS2
      DFCI_PERMISSION2_POLICY_APPLY_VAR_NAME,
     &gDfciPermissionManagerVarNamespace,
      DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES,
      DFCI_PERMISSION_POLICY_APPLY_VAR_SIGNATURE },

    { // JSON_SET_SETTINGS
      DFCI_SETTINGS_APPLY_INPUT_VAR_NAME,
     &gDfciSettingsManagerVarNamespace,
      DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES,
      DFCI_SECURED_SETTINGS_APPLY_VAR_SIGNATURE },

    { // JSON_SET_SETTINGS2
      DFCI_SETTINGS2_APPLY_INPUT_VAR_NAME,
     &gDfciSettingsManagerVarNamespace,
      DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES,
      DFCI_SECURED_SETTINGS_APPLY_VAR_SIGNATURE }
};

#define KEYWORD_MFG               "OemManufacturer"
#define KEYWORD_MODEL             "ModelName"
#define KEYWORD_SERIAL            "SerialNumber"
#define KEYWORD_THUMBPRINT        "Thumbprint"

#define KEYWORD_PROVISIONING      "ProvisioningPacket"
#define KEYWORD_PROVISIONING2     "ProvisioningPacket2"
#define KEYWORD_PERMISSIONS       "PermissionsPacket"
#define KEYWORD_PERMISSIONS2      "PermissionsPacket2"
#define KEYWORD_SETTINGS          "SettingsPacket"
#define KEYWORD_SETTINGS2         "SettingsPacket2"
#define KEYWORD_TRANSITIONING1    "TransitionPacket1"
#define KEYWORD_TRANSITIONING2    "TransitionPacket2"
#define KEYWORD_RESULT_MESSAGE    "ResultMessage"
#define KEYWORD_RESULT_CODE       "ResultCode"
#define KEYWORD_OWNER_THUMBPRINT  "DdsWildcardCertificateThumbprint"
#define KEYWORD_HTTPS_THUMBPRINT  "DdsEncryptionCertificateThumbprint"

//
// DFCI RecoveryBootstrap response
//
JSON_RESPONSE_TO_ACTION_ENTRY mRecoveryBootstrapResponse[] = {
    {
        KEYWORD_TRANSITIONING1,
        NULL,
        JSON_ACTION_SET_VARIABLE,
        JSON_SET_IDENTITY
    },
    {
        KEYWORD_TRANSITIONING2,
        NULL,
        JSON_ACTION_SET_VARIABLE,
        JSON_SET_IDENTITY2
    },
    {
        KEYWORD_SETTINGS,
        NULL,
        JSON_ACTION_SET_VARIABLE,
        JSON_SET_SETTINGS
    },
    {
        KEYWORD_RESULT_MESSAGE,
       &mDfciNetworkRequest.HttpStatus.HttpMessage,
        JSON_ACTION_SET_HTTP_MESSAGE,
        0
    },
    {
        KEYWORD_RESULT_CODE,
       &mDfciNetworkRequest.HttpStatus.HttpReturnCode,
        JSON_ACTION_SET_RETURN_CODE,
        0
    },
    {
        NULL,
        NULL,
        0,
        0
    }
};

//
// DFCI Recovery response
//
JSON_RESPONSE_TO_ACTION_ENTRY mRecoveryResponse[] = {
    { KEYWORD_PROVISIONING,
      NULL,
      JSON_ACTION_SET_VARIABLE,
      JSON_SET_IDENTITY
    },
    { KEYWORD_PERMISSIONS,
      NULL,
      JSON_ACTION_SET_VARIABLE,
      JSON_SET_PERMISSIONS
    },
    { KEYWORD_SETTINGS,
      NULL,
      JSON_ACTION_SET_VARIABLE,
      JSON_SET_SETTINGS
    },
    { KEYWORD_RESULT_MESSAGE,
     &mDfciNetworkRequest.HttpStatus.HttpMessage,
      JSON_ACTION_SET_HTTP_MESSAGE,
      0
    },
    { KEYWORD_RESULT_CODE,
      &mDfciNetworkRequest.HttpStatus.HttpReturnCode,
     JSON_ACTION_SET_RETURN_CODE,
      0
    },
    { NULL,
      0,
      0
    }
};

//
// DFCI USB Recovery response
//
//
// The standard Dfci Json string is of the format:
//    { "ProvisioningPacket",  "b64-encoded-dfci-identity-packet",
//      "ProvisioningPacket2", "b64-encoded-dfci-identity-packet",
//      "Permissions",         "b64-encoded-dfci-permissions-packet",
//      "Permissions2",        "b64-encoded-dfci-permissions-packet",
//      "SettingsPacket",      "b64-encoded-dfci-settings-packet",
//      "SettingsPacket2",     "b64-encoded-dfci-settings-packet" }
//
JSON_RESPONSE_TO_ACTION_ENTRY mUsbRecovery[] = {
    { KEYWORD_PROVISIONING,
      JSON_ACTION_SET_VARIABLE,
      JSON_SET_IDENTITY
    },
    { KEYWORD_PROVISIONING2,
      JSON_ACTION_SET_VARIABLE,
      JSON_SET_IDENTITY2
    },
    { KEYWORD_PERMISSIONS,
      JSON_ACTION_SET_VARIABLE,
      JSON_SET_PERMISSIONS
    },
    { KEYWORD_PERMISSIONS2,
      JSON_ACTION_SET_VARIABLE,
      JSON_SET_PERMISSIONS2
    },
    { KEYWORD_SETTINGS,
      JSON_ACTION_SET_VARIABLE,
      JSON_SET_SETTINGS
    },
    { KEYWORD_SETTINGS2,
      JSON_ACTION_SET_VARIABLE,
      JSON_SET_SETTINGS2
    },
    { NULL,
      0,
      0
    }
};


/**
 * BuildUsbRequest
 *
 * @param[in]   FileNameExtension - Extension for file name
 * @param[out]  filename          - Name of the file on USB to retrieve
 *
 **/
EFI_STATUS
EFIAPI
BuildUsbRequest (
    IN  CHAR16       *FileExtension,
    OUT CHAR16      **FileName
  ) {

    DFCI_SYSTEM_INFORMATION  DfciInfo;
    UINTN                    i;
    CHAR16                  *PktFileName = NULL;
    UINTN                    PktNameLen;
    EFI_STATUS               Status;

    PktFileName = NULL;

    Status = DfciGetSystemInfo (&DfciInfo);
    if (EFI_ERROR(Status)) {
         goto Error;
    }

    PktFileName = (CHAR16 *) AllocatePool (MAX_USB_FILE_NAME_LENGTH * sizeof(CHAR16));
    if (PktFileName == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Error;
    }

    // The maximum file name length is 255 characters and a NULL.  Leave room for
    // the four character file name extension.  Create the base PktFileName out of
    // the first 251 characters of SerialNumber_ProductName_Manufacturer then add the
    // file name extension.

    PktNameLen = UnicodeSPrintAsciiFormat (PktFileName,
                                          (MAX_USB_FILE_NAME_LENGTH - 4) * sizeof(CHAR16),
                                          "%a_%a_%a",
                                           DfciInfo.SerialNumber,
                                           DfciInfo.ProductName,
                                           DfciInfo.Manufacturer);
    DfciFreeSystemInfo (&DfciInfo);
    if ((PktNameLen == 0) || (PktNameLen >= (MAX_USB_FILE_NAME_LENGTH - 4))) {
        DEBUG((DEBUG_ERROR, "Invalid file name length %d\n", PktNameLen));
        Status = EFI_BAD_BUFFER_SIZE;
        goto Error;
    }

    //
    //  Any binary value of 0x01-0x1f, and any of    " * / : < > ? \ |
    //  are not allowed in the file name.  If any of these exist, then
    //  replace the invalid character with an '@'.
    //
    for (i = 0; i < PktNameLen; i++) {
        if (((PktFileName[i] >= 0x00) &&
             (PktFileName[i] <= 0x1F)) ||
             (PktFileName[i] == L'\"') ||
             (PktFileName[i] == L'*')  ||
             (PktFileName[i] == L'/')  ||
             (PktFileName[i] == L':')  ||
             (PktFileName[i] == L'<')  ||
             (PktFileName[i] == L'>')  ||
             (PktFileName[i] == L'?')  ||
             (PktFileName[i] == L'\\') ||
             (PktFileName[i] == L'|')) {
            PktFileName[i] = L'@';
        }
    }

    Status = StrCatS (PktFileName, MAX_USB_FILE_NAME_LENGTH * sizeof(CHAR16), FileExtension);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to append the file name ext. Code=%r\n", Status));
        goto Error;
    }


    *FileName = PktFileName;

    return EFI_SUCCESS;

Error:
    if (NULL != PktFileName) {
       FreePool (PktFileName);
    }

    DfciFreeSystemInfo (&DfciInfo);

    return Status;
}

/**
  EncodeData - Convert blob data to a base64 value

**/
STATIC
EFI_STATUS
EncodeData (
    CHAR8                *Value,
    UINTN                 ValueSize,
    JSON_REQUEST_ELEMENT *Rqst
  ) {

    EFI_STATUS      Status;

    if (NULL == Value) {
        Rqst->ValueSize = 0;
        Rqst->Value = NULL;
        Status = EFI_SUCCESS;
    } else {
        Status = Base64Encode ((UINT8 *) Value,
                               ValueSize - sizeof(CHAR8),
                               NULL,
                              &Rqst->ValueSize);
        if ((EFI_BUFFER_TOO_SMALL != Status) || (0 == Rqst->ValueSize)) {
            DEBUG((DEBUG_ERROR,"Unable to convert field for index %a. Size=%d, Code = %r\n", Value, ValueSize, Status));
            if (EFI_SUCCESS == Status) {
                Status = EFI_INVALID_PARAMETER;
            }
            return Status;
        }

        Rqst->Value = AllocatePool (Rqst->ValueSize);
        if (NULL == Rqst->Value) {
            return EFI_OUT_OF_RESOURCES;
        }

        Status = Base64Encode ((UINT8 *) Value,
                               ValueSize - sizeof(CHAR8),
                               Rqst->Value,
                              &Rqst->ValueSize);
    }

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Convert to b64 failed. Code = %r\n", Status));
    }

    return Status;
}

/**
 * BuildJsonBootstrapRequest
 *
 * Build the payload for a Bootstrap request
 *
 * @Param [in]    NetworkRequest
 *
 **/
EFI_STATUS
EFIAPI
BuildJsonBootstrapRequest (
  IN DFCI_NETWORK_REQUEST *NetworkRequest
  ) {

#define JSON_RECOVERY_BOOTSTRAP_COUNT 2

    JSON_REQUEST_ELEMENT    JsonRequest[JSON_RECOVERY_BOOTSTRAP_COUNT];
    CHAR8                  *JsonRequestString;
    UINTN                   JsonRequestStringSize;
    EFI_STATUS              Status;

    JsonRequest[0].FieldName = KEYWORD_HTTPS_THUMBPRINT;
    JsonRequest[0].FieldSize = sizeof (KEYWORD_HTTPS_THUMBPRINT);
    Status = EncodeData (NetworkRequest->HttpsThumbprint, NetworkRequest->HttpsThumbprintSize, &JsonRequest[0]);
    if (EFI_ERROR(Status)) {
        goto BOOTSTRAP_CLEANUP;
    }

    JsonRequest[1].FieldName = KEYWORD_OWNER_THUMBPRINT;
    JsonRequest[1].FieldSize = sizeof (KEYWORD_OWNER_THUMBPRINT);
    Status = EncodeData (NetworkRequest->OwnerThumbprint, NetworkRequest->OwnerThumbprintSize, &JsonRequest[1]);
    if (EFI_ERROR(Status)) {
        goto BOOTSTRAP_CLEANUP;
    }

    Status = JsonLibEncode (JsonRequest, JSON_RECOVERY_BOOTSTRAP_COUNT, &JsonRequestString, &JsonRequestStringSize);
    if (!EFI_ERROR(Status)) {
        NetworkRequest->HttpRequest.Body = JsonRequestString;
        NetworkRequest->HttpRequest.BodySize= JsonRequestStringSize;
    }

BOOTSTRAP_CLEANUP:

  return Status;

}

/**
 * BuildJsonRecoveryRequest
 *
 * Build the payload for a Recovery request
 *
 * @Param [in]    NetworkRequest
 *
 **/
EFI_STATUS
BuildJsonRecoveryRequest (
  IN DFCI_NETWORK_REQUEST *NetworkRequest
  ) {

#define JSON_RECOVERY_REQUEST_COUNT 4

    JSON_REQUEST_ELEMENT    JsonRequest[JSON_RECOVERY_REQUEST_COUNT];
    CHAR8                  *JsonRequestString;
    UINTN                   JsonRequestStringSize;
    EFI_STATUS              Status;


    JsonRequest[0].FieldName = KEYWORD_MFG;
    JsonRequest[0].FieldSize = sizeof (KEYWORD_MFG);
    Status = EncodeData (NetworkRequest->DfciInfo.Manufacturer, NetworkRequest->DfciInfo.ManufacturerSize, &JsonRequest[0]);
    if (EFI_ERROR(Status)) {
        goto BOOTSTRAP_CLEANUP;
    }

    JsonRequest[1].FieldName = KEYWORD_MODEL;
    JsonRequest[1].FieldSize = sizeof (KEYWORD_MODEL);
    Status = EncodeData (NetworkRequest->DfciInfo.ProductName, NetworkRequest->DfciInfo.ProductNameSize, &JsonRequest[1]);
    if (EFI_ERROR(Status)) {
        goto BOOTSTRAP_CLEANUP;
    }

    JsonRequest[2].FieldName = KEYWORD_SERIAL;
    JsonRequest[2].FieldSize = sizeof (KEYWORD_SERIAL);
    Status = EncodeData (NetworkRequest->DfciInfo.SerialNumber, NetworkRequest->DfciInfo.SerialNumberSize, &JsonRequest[2]);
    if (EFI_ERROR(Status)) {
        goto BOOTSTRAP_CLEANUP;
    }

    JsonRequest[3].FieldName = KEYWORD_OWNER_THUMBPRINT;
    JsonRequest[3].FieldSize = sizeof (KEYWORD_OWNER_THUMBPRINT);
    Status = EncodeData (NetworkRequest->OwnerThumbprint, NetworkRequest->OwnerThumbprintSize, &JsonRequest[3]);
    if (EFI_ERROR(Status)) {
        goto BOOTSTRAP_CLEANUP;
    }

    Status = JsonLibEncode (JsonRequest, JSON_RECOVERY_REQUEST_COUNT, &JsonRequestString, &JsonRequestStringSize);
    if (!EFI_ERROR(Status)) {
        if (NetworkRequest->HttpRequest.Body == NULL) {
            FreePool (NetworkRequest->HttpRequest.Body);
        }
        NetworkRequest->HttpRequest.Body = JsonRequestString;
        NetworkRequest->HttpRequest.BodySize= JsonRequestStringSize;
    }

BOOTSTRAP_CLEANUP:

  return Status;

}

/**
 *  Build Json Request.  For a network request, the current system information
 *                       needs to be provided.
 *
 *  @param[out]  JsonString
 *  @param[out]  JsonStringSize
 *
 **/
EFI_STATUS
EFIAPI
BuildJsonRequest (
    IN  CHAR8       *OwnerThumbprint,
    IN  CHAR8       *ZtdThumbprint,
    OUT CHAR8      **JsonString,
    OUT UINTN       *JsonStringSize
  ) {

//    CHAR8                  *JsonRequestString;
//    UINTN                   JsonRequestStringSize;
//    JSON_REQUEST_ELEMENT    JsonRequest[4 /*TODO JSON_REQUEST_COUNT*/];
    EFI_STATUS              Status;
//    CHAR8                  *Thumbprint;
//    UINTN                   ThumbprintSize;

    Status = EFI_UNSUPPORTED;
/**
    if (OwnerThumbprint != NULL) {
        ThumbprintSize = Str;

        ThumbprintSize = StrnSizeS(Id, DFCI_MAX_ID_SIZE);
        if ((IdSize < 1) || (IdSize > DFCI_MAX_ID_SIZE))
        {
          DEBUG((DEBUG_ERROR, "%a - Invalid ID length %d for %a\n", __FUNCTION__, IdSize, Id));
          return EFI_INVALID_PARAMETER;
        }

        Thumbprint = AllocatePool ()
    }
    **/
    //
    //  TODO - Add code to get the Owner thumbprint
    //
#if 0

#define THUMBPRINT "00 01 02 03 04 05 06 07 08 09 10"
    Thumbprint = THUMBPRINT;
    ThumbprintSize = sizeof(THUMBPRINT);

    ZeroMem (JsonRequest, sizeof(JsonRequest));

    Status = DfciGetSystemInfo (&DfciInfo);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: Unable to get Dfci System Info. %r\n", __FUNCTION__, Status));
        goto CLEANUP;
    }

    JsonRequest[0].FieldName = JSON_REQUEST_MFG;
    JsonRequest[0].FieldSize = sizeof (JSON_REQUEST_MFG);
    Status = EncodeData (DfciInfo.Manufacturer, DfciInfo.ManufacturerSize, &JsonRequest[0]);
    if (EFI_ERROR(Status)) {
        goto CLEANUP;
    }

    JsonRequest[1].FieldName = JSON_REQUEST_MODEL;
    JsonRequest[1].FieldSize = sizeof (JSON_REQUEST_MODEL);
    Status = EncodeData (DfciInfo.ProductName, DfciInfo.ProductNameSize, &JsonRequest[1]);
    if (EFI_ERROR(Status)) {
        goto CLEANUP;
    }

    JsonRequest[2].FieldName = JSON_REQUEST_SERIAL;
    JsonRequest[2].FieldSize = sizeof (JSON_REQUEST_SERIAL);
    Status = EncodeData (DfciInfo.SerialNumber, DfciInfo.SerialNumberSize, &JsonRequest[2]);
    if (EFI_ERROR(Status)) {
        goto CLEANUP;
    }

    JsonRequest[3].FieldName = JSON_REQUEST_THUMBPRINT;
    JsonRequest[3].FieldSize = sizeof (JSON_REQUEST_THUMBPRINT);
    Status = EncodeData (Thumbprint, ThumbprintSize, &JsonRequest[3]);
    if (EFI_ERROR(Status)) {
        goto CLEANUP;
    }

    Status = JsonLibEncode (JsonRequest, JSON_REQUEST_COUNT, &JsonRequestString, &JsonRequestStringSize);
    if (EFI_ERROR(Status)) {
        *JsonString = NULL;
        *JsonStringSize = 0;
    } else {
        *JsonString = JsonRequestString;
        *JsonStringSize = JsonRequestStringSize;
    }

CLEANUP:
    FreeDfciSystemInfo (&DfciInfo);
#endif
    return Status;
}

/**
 *  Function to process a Json Element
 *
 * @param[in]  Json Request Element   Element Being Processed
 * @param[in]  Context                Context for Apply Function
 *
 * @retval EFI_SUCCESS -       Packet processed normally, no variable set
 * @retval EFI_MEDIA_CHANGED - Packet processed normally, variable set
 * @retval Error -             Severe error processing packet
 */
STATIC
EFI_STATUS
ProcessFunction (
    IN  JSON_REQUEST_ELEMENT  *Rqst,
    IN  VOID                  *Context
  ) {

    UINTN                          ActionIndex;
    UINTN                          j;
    DFCI_PACKET_HEADER            *Pkt;
    JSON_RESPONSE_TO_ACTION_ENTRY *ResponseTable;
    EFI_STATUS                     Status;
    CHAR8                         *StringValue;
    BOOLEAN                        Valid;
    BOOLEAN                        VariableChanged;
    UINTN                          ValueSize;

    //
    // The FieldName and Value must be specified
    //
    if ((NULL == Rqst) ||
        (NULL == Rqst->FieldName) ||
        (NULL == Rqst->Value) ||
        (NULL == Context)) {
        DEBUG((DEBUG_ERROR,"Invalid or missing ProcessFunction parameter\n"));
        return EFI_INVALID_PARAMETER;
    }

    ResponseTable = (JSON_RESPONSE_TO_ACTION_ENTRY *) Context;
    Valid = FALSE;
    VariableChanged = FALSE;
    for (j = 0; ResponseTable[j].FieldName != NULL; j++) {
        if ((Rqst->FieldSize == AsciiStrSize(ResponseTable[j].FieldName)) &&
            (0 == AsciiStrnCmp (ResponseTable[j].FieldName, Rqst->FieldName, Rqst->FieldSize))) {

            ValueSize = 0;
            Status = Base64Decode(Rqst->Value, Rqst->ValueSize - sizeof(CHAR8), NULL, &ValueSize);
            if (Status != EFI_BUFFER_TOO_SMALL) {
              DEBUG((DEBUG_ERROR, "Cannot query binary blob size. Code = %r\n",Status));
              return EFI_INVALID_PARAMETER;
            }

            StringValue = (CHAR8 *) AllocatePool (ValueSize + sizeof(CHAR8)); // Allow for extra NULL
            if (NULL == Rqst->Value) {
              DEBUG((DEBUG_ERROR, "Cannot allocate Rqst->Value\n"));
              return EFI_OUT_OF_RESOURCES;
            }

            Status = Base64Decode (Rqst->Value, Rqst->ValueSize - sizeof(CHAR8), (UINT8 *) StringValue, &ValueSize);
            if (EFI_ERROR(Status)) {
                FreePool (StringValue);
                DEBUG((DEBUG_ERROR, "Cannot decode Value data. Code=%r\n",Status));
                return Status;
            }

            StringValue[ValueSize] = '\0';  // Add a NULL in case it is not part of the string.
            ActionIndex = ResponseTable[j].VariableIndex;

            switch (ResponseTable[j].Action) {
            case JSON_ACTION_SET_VARIABLE:
                Pkt = (DFCI_PACKET_HEADER *) StringValue;
                if (mJsonSetVariableEntryMailbox[ActionIndex].Signature != Pkt->Sig.Signature) {
                    DEBUG((DEBUG_ERROR,"Invalid binary signature %4.4x, Indx=%d, Rqst %a. Expected %4.4x for %a\n",
                        Pkt->Sig.Signature,
                        j,
                        Rqst->FieldName,
                        mJsonSetVariableEntryMailbox[ActionIndex].Signature,
                        ResponseTable[j].FieldName));
                    FreePool (StringValue);
                    return EFI_INVALID_PARAMETER;
                }

                Status = gRT->SetVariable(mJsonSetVariableEntryMailbox[ActionIndex].MailboxName,
                                          mJsonSetVariableEntryMailbox[ActionIndex].MailboxNamespace,
                                          mJsonSetVariableEntryMailbox[ActionIndex].MailboxAttributes,
                                          ValueSize,
                                          (VOID *) Pkt);
                if (EFI_ERROR(Status)) {
                    DEBUG((DEBUG_ERROR, "Unable to set mailbox %s. Code = %r\n",
                                        mJsonSetVariableEntryMailbox[ActionIndex].MailboxName,
                                        Status));
                    FreePool (StringValue);
                    return Status;
                }  else {
                    VariableChanged = TRUE;
                    DEBUG((DEBUG_INFO, "Mailbox %s setup\n", mJsonSetVariableEntryMailbox[ActionIndex].MailboxName));
                }

                Valid = TRUE;
                break;

            case JSON_ACTION_SET_RETURN_CODE:
            case JSON_ACTION_SET_HTTP_MESSAGE:
                *ResponseTable[j].Message = StringValue;
                StringValue = NULL;
                Valid = TRUE;
                break;

            default:
                break;
            }

            if (NULL != StringValue) {
                FreePool (StringValue);
            }

            break;
        }
    }

    if (Valid) {
        if (VariableChanged) {
            Status = EFI_MEDIA_CHANGED;
            DEBUG((DEBUG_INFO, "Media Change detected in DfciUpdate\n"));
        } else {
            Status = EFI_SUCCESS;
        }
    } else {
        Status = EFI_INVALID_PARAMETER;
        DEBUG((DEBUG_ERROR, "Rqst not found int ResponseTable. Rqst=%a\n", Rqst->FieldName));
    }

    return Status;
}

/**
 * DfciUpdateFromJson
 *
 * Parse the Json String and update DFCI with each element parsed.
 *
 * @param [in]  JsonString      - Dfci Update Json string
 * @param [in]  JsonStringSize  - Size of Json String
 *
 **/
EFI_STATUS
EFIAPI
DfciUpdateFromJson (
    IN  CHAR8                         *JsonString,
    IN  UINTN                          JsonStringSize,
    IN  JSON_RESPONSE_TO_ACTION_ENTRY *ResponseTable
    ) {

    EFI_STATUS      Status;

    Status = JsonLibParse (JsonString, JsonStringSize, ProcessFunction, (VOID *) ResponseTable);
    return Status;
}