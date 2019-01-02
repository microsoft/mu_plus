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

#include "DfciUsb.h"
#include "DfciRequest.h"

//
// The standard Dfci Json string is of the format:
//    { "ProvisioningPacket",  "b64-encoded-dfci-identity-packet",
//      "ProvisioningPacket2", "b64-encoded-dfci-identity-packet",
//      "Permissions",         "b64-encoded-dfci-permissions-packet",
//      "Permissions2",        "b64-encoded-dfci-permissions-packet",
//      "SettingsPacket",      "b64-encoded-dfci-settings-packet",
//      "SettingsPacket2",     "b64-encoded-dfci-settings-packet" }
//

#define JSON_REQUEST_MFG            "OemManufacturer"
#define JSON_REQUEST_MODEL          "ModelName"
#define JSON_REQUEST_SERIAL         "SerialNumber"
#define JSON_REQUEST_THUMBPRINT     "Thumbprint"

#define JSON_REQUEST_COUNT 4

#define JSON_RESPONSE_IDENTITY      "ProvisioningPacket"
#define JSON_RESPONSE_IDENTITY2     "ProvisioningPacket2"
#define JSON_RESPONSE_PERMISSIONS   "PermissionsPacket"
#define JSON_RESPONSE_PERMISSIONS2  "PermissionsPacket2"
#define JSON_RESPONSE_SETTINGS      "SettingsPacket"
#define JSON_RESPONSE_SETTINGS2     "SettingsPacket2"

typedef struct {
    CHAR8    *FieldName;
    CHAR16   *MailboxName;
    EFI_GUID *MailboxNamespace;
    UINT32    MailboxAttributes;
    UINT32    Signature;
} JSON_TO_MAILBOX_ENTRY;

STATIC JSON_TO_MAILBOX_ENTRY mJsonToMailbox[] = {
    {  JSON_RESPONSE_IDENTITY,
       DFCI_IDENTITY_APPLY_VAR_NAME,
      &gDfciAuthProvisionVarNamespace,
       DFCI_IDENTITY_VAR_ATTRIBUTES,
       DFCI_IDENTITY_APPLY_VAR_SIGNATURE },
    {  JSON_RESPONSE_IDENTITY2,
       DFCI_IDENTITY2_APPLY_VAR_NAME,
      &gDfciAuthProvisionVarNamespace,
       DFCI_IDENTITY_VAR_ATTRIBUTES,
       DFCI_IDENTITY_APPLY_VAR_SIGNATURE },
    {  JSON_RESPONSE_PERMISSIONS,
       DFCI_PERMISSION_POLICY_APPLY_VAR_NAME,
      &gDfciPermissionManagerVarNamespace,
       DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES,
       DFCI_PERMISSION_POLICY_APPLY_VAR_SIGNATURE },
    {  JSON_RESPONSE_PERMISSIONS2,
       DFCI_PERMISSION2_POLICY_APPLY_VAR_NAME,
      &gDfciPermissionManagerVarNamespace,
       DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES,
       DFCI_PERMISSION_POLICY_APPLY_VAR_SIGNATURE },
    {  JSON_RESPONSE_SETTINGS,
       DFCI_SETTINGS_APPLY_INPUT_VAR_NAME,
      &gDfciSettingsManagerVarNamespace,
       DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES,
       DFCI_SECURED_SETTINGS_APPLY_VAR_SIGNATURE },
    {  JSON_RESPONSE_SETTINGS2,
       DFCI_SETTINGS2_APPLY_INPUT_VAR_NAME,
      &gDfciSettingsManagerVarNamespace,
       DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES,
       DFCI_SECURED_SETTINGS_APPLY_VAR_SIGNATURE }
};

#define JSON_RESPONSE_COUNT (sizeof(mJsonToMailbox) / sizeof(mJsonToMailbox[0]))

typedef struct {
    CHAR8      *SerialNumber;
    UINTN       SerialNumberSize;
    CHAR8      *Manufacturer;
    UINTN       ManufacturerSize;
    CHAR8      *ProductName;
    UINTN       ProductNameSize;
} DFCI_SYSTEM_INFORMATION;

/**
 * FreeDfciInfo
 *
 * Free the system identifier elements
 **/
STATIC
VOID
FreeDfciSystemInfo (
    IN DFCI_SYSTEM_INFORMATION *DfciInfo
  ) {

    if (NULL != DfciInfo->SerialNumber) {
        FreePool (DfciInfo->SerialNumber);
    }
    if (NULL != DfciInfo->Manufacturer) {
        FreePool (DfciInfo->Manufacturer);
    }
    if (NULL != DfciInfo->ProductName) {
        FreePool (DfciInfo->ProductName);
    }
}

/**
 * GetDfciSystemInfo
 *
 * Get the system identifier elements
 *
 * @param[in] DfciInfo
 *
 **/
STATIC
EFI_STATUS
GetDfciSystemInfo (
    IN DFCI_SYSTEM_INFORMATION *DfciInfo
  ) {

    EFI_STATUS      Status;

    ZeroMem (DfciInfo, sizeof(DFCI_SYSTEM_INFORMATION));

    Status = DfciIdSupportGetSerialNumber (&DfciInfo->SerialNumber, &DfciInfo->SerialNumberSize);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to get ProductName. Code=%r\n", Status));
        goto Error;
    }

    Status = DfciIdSupportGetManufacturer (&DfciInfo->Manufacturer, &DfciInfo->ManufacturerSize);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to get ProductName. Code=%r\n", Status));
        goto Error;
    }

    Status = DfciIdSupportGetProductName (&DfciInfo->ProductName, &DfciInfo->ProductNameSize);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to get ProductName. Code=%r\n", Status));
        goto Error;
    }

    return EFI_SUCCESS;

Error:
    FreeDfciSystemInfo (DfciInfo);
    return Status;
}

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
    CHAR16                  *PktFileName;
    UINTN                    PktNameLen;
    EFI_STATUS               Status;

    Status = GetDfciSystemInfo (&DfciInfo);
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
    FreeDfciSystemInfo (&DfciInfo);
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

    FreeDfciSystemInfo (&DfciInfo);

    return Status;
}

/**
  EncodeData - Convert blob data to a base64 value

**/
EFI_STATUS
EncodeData (
    CHAR8                *Value,
    UINTN                 ValueSize,
    JSON_REQUEST_ELEMENT *Rqst
  ) {

    EFI_STATUS      Status;


    Status = Base64Encode (Value,
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

    Status = Base64Encode (Value,
                           ValueSize - sizeof(CHAR8),
                           Rqst->Value,
                          &Rqst->ValueSize);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Convert to b64 failed. Code = %r\n", Status));
    }

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
    OUT CHAR8      **JsonString,
    OUT UINTN       *JsonStringSize
  ) {

    DFCI_SYSTEM_INFORMATION DfciInfo;
    CHAR8                  *JsonRequestString;
    UINTN                   JsonRequestStringSize;
    JSON_REQUEST_ELEMENT    JsonRequest[JSON_REQUEST_COUNT];
    EFI_STATUS              Status;
    CHAR8                  *Thumbprint;
    UINTN                   ThumbprintSize;

    //
    //  TODO - Add code to get the Owner thumbprint
    //
#define THUMBPRINT "00 01 02 03 04 05 06 07 08 09 10"
    Thumbprint = THUMBPRINT;
    ThumbprintSize = sizeof(THUMBPRINT);

    ZeroMem (JsonRequest, sizeof(JsonRequest));

    Status = GetDfciSystemInfo (&DfciInfo);
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

    return Status;
}

/**
 *  Function to process a Json Element
 *
 * @param[in]  Json Request Element   Element Being Processed
 * @param[in]  Context                Context for Apply Function
 *
 * @retval EFI_SUCCESS -       Packet processed normally
 * @retval Error -             Severe error processing packet
 */
STATIC
EFI_STATUS
ApplyFunction (
    IN  JSON_REQUEST_ELEMENT *Rqst
  ) {

    UINTN               j;
    DFCI_PACKET_HEADER *Pkt;
    EFI_STATUS          Status;
    BOOLEAN             Valid;
    UINTN               ValueSize;

    //
    // The FieldName and Value must be specified
    //
    if ((NULL == Rqst) || (NULL == Rqst->FieldName) || (NULL == Rqst->Value)) {
        DEBUG((DEBUG_ERROR,"Invalid or missing Request entry\n"));
        return EFI_INVALID_PARAMETER;
    }

    Valid = FALSE;
    for (j = 0; j < JSON_RESPONSE_COUNT; j++) {
        if (0 == AsciiStrnCmp (mJsonToMailbox[j].FieldName, Rqst->FieldName, Rqst->FieldSize)) {

            ValueSize = 0;
            Status = Base64Decode(Rqst->Value, Rqst->ValueSize - sizeof(CHAR8), NULL, &ValueSize);
            if (Status != EFI_BUFFER_TOO_SMALL) {
              DEBUG((DEBUG_ERROR, "Cannot query binary blob size. Code = %r\n",Status));
              return EFI_INVALID_PARAMETER;
            }

            Pkt = (DFCI_PACKET_HEADER *) AllocatePool (ValueSize);
            if (NULL == Rqst->Value) {
              DEBUG((DEBUG_ERROR, "Cannot allocate Rqst->Value\n"));
              return EFI_OUT_OF_RESOURCES;
            }

            Status = Base64Decode (Rqst->Value, Rqst->ValueSize - sizeof(CHAR8), &Pkt->Sig.Pkt[0], &ValueSize);
            if (EFI_ERROR(Status)) {
                FreePool (Pkt);
                DEBUG((DEBUG_ERROR, "Cannot decode Value data. Code=%r\n",Status));
                return Status;
            }

            if (mJsonToMailbox[j].Signature != Pkt->Sig.Signature) {
                DEBUG((DEBUG_ERROR,"Invalid binary signature %4.4x for Rqst %a\n",
                    Pkt->Sig.Signature,
                    Rqst->FieldName));
                return EFI_INVALID_PARAMETER;
            }

            Status = gRT->SetVariable(mJsonToMailbox[j].MailboxName,
                                      mJsonToMailbox[j].MailboxNamespace,
                                      mJsonToMailbox[j].MailboxAttributes,
                                      ValueSize,
                                      (VOID *) Pkt);
            FreePool (Pkt);
            if (EFI_ERROR(Status)) {
                DEBUG((DEBUG_ERROR, "Unable to set mailbox %s. Code = %r\n", mJsonToMailbox[j].MailboxName, Status));
                return Status;
            }  else {
                DEBUG((DEBUG_INFO, "Mailbox %s setup\n", mJsonToMailbox[j].MailboxName));
            }

            Valid = TRUE;
            break;
        }
    }

    return (Valid == TRUE) ? EFI_SUCCESS : EFI_INVALID_PARAMETER;
}

/**
 * UpdateDfciFromJson
 *
 * Parse the Json String and update DFCI with each element parsed.
 *
 * @param [in]  JsonString      - Dfci Update Json string
 * @param [in]  JsonStringSize  - Size of Json String
 *
 **/
EFI_STATUS
EFIAPI
UpdateDfciFromJson (
    IN  CHAR8   *JsonString,
    IN  UINTN    JsonStringSize
    ) {

    EFI_STATUS      Status;

    Status = JsonLibParse (JsonString, JsonStringSize, ApplyFunction);
    return Status;
}