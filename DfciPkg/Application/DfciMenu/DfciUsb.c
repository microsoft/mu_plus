/** @file
DfciUsb.c

This module will request new DFCI configuration data from a USB drive.

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
//   .xid  -- Identity packet
//   .xps  -- Permission packet
//   .xss  -- Settings packet
//
//  After assembling the filename, each character is inspected for invalid characters.  The
//  following are invalid characters:
//
//  Any binary value of 0x01-0x1f, and any of    " * / : < > ? \ |
//
//  All invalid characters are changed to @
//

#include <Uefi.h>

#include <Guid/DfciPacketHeader.h>
#include <Guid/DfciIdentityAndAuthManagerVariables.h>
#include <Guid/DfciPermissionManagerVariables.h>
#include <Guid/DfciSettingsManagerVariables.h>

#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DfciDeviceIdSupportLib.h>
#include <Library/FileHandleLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "DfciMenu.h"
#include "DfciUsb.h"

// MAX_USB_FILE_NAME_LENGTH Includes the terminating NULL
#define MAX_USB_FILE_NAME_LENGTH 256

typedef enum {
    IdentityPkt     = 0,
    PermissionsPkt  = 1,
    SettingsPkt     = 2,
    Identity2Pkt    = 3,
    Permissions2Pkt = 4,
    Settings2Pkt    = 5,
    MAX_PACKET_TYPE = 6
} DFCI_USB_PACKET_TYPE;

STATIC CHAR16    *mPktName[MAX_PACKET_TYPE];
STATIC UINT64     mPktResponse[MAX_PACKET_TYPE];
STATIC EFI_STATUS mPktStatus[MAX_PACKET_TYPE];

/**
*
*  Scan USB Drives looking for the file name passed in.
*
*  @param[in]     PktFileName     Name of update file to read.
*  @param[out]    Buffer          Where to store a buffer pointer.
*  @param[out]    BufferSize      Where to store the buffer size.
*
*
*  @retval   EFI_SUCCESS     The FS volume was opened successfully.
*  @retval   Others          The operation failed.
*
**/
STATIC
EFI_STATUS
FindUsbDriveWithDfciUpdate (
    IN  CHAR16   *PktFileName,
    OUT CHAR8   **Buffer,
    OUT UINTN    *BufferSize
  ) {

    EFI_FILE_PROTOCOL               *FileHandle;
    EFI_FILE_PROTOCOL               *VolHandle;
    EFI_HANDLE                      *HandleBuffer;
    UINTN                            Index;
    UINTN                            NumHandles;
    EFI_STATUS                       Status;
    EFI_STATUS                       Status2;
    EFI_DEVICE_PATH_PROTOCOL        *BlkIoDevicePath;
    EFI_DEVICE_PATH_PROTOCOL        *UsbDevicePath;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SfProtocol;
    EFI_HANDLE                       Handle;
    EFI_FILE_INFO                   *FileInfo;
    UINTN                            ReadSize;


    if ((NULL == PktFileName) ||
        (NULL == Buffer) ||
        (NULL == BufferSize)) {
        return EFI_INVALID_PARAMETER;
    }

    NumHandles = 0;
    HandleBuffer = NULL;
    SfProtocol = NULL;

    //
    // Locate all handles that are using the SFS protocol.
    //
    Status = gBS->LocateHandleBuffer(ByProtocol,
                                     &gEfiSimpleFileSystemProtocolGuid,
                                     NULL,
                                     &NumHandles,
                                     &HandleBuffer);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: failed to locate any handles using the Simple FS protocol (%r)\n", __FUNCTION__, Status));
        goto CleanUp;
    }

    //
    // Search the handles to find one that has has a USB node in the device path.
    //
    for (Index = 0; (Index < NumHandles); Index += 1) {
        //
        // Insure this device is on a USB controller
        //
        UsbDevicePath = DevicePathFromHandle(HandleBuffer[Index]);
        if (UsbDevicePath == NULL) {
            continue;
        }
        Status = gBS->LocateDevicePath (&gEfiUsbIoProtocolGuid,
                                        &UsbDevicePath,
                                        &Handle);
        if (EFI_ERROR(Status)) {
            // Device is not USB;
            continue;
        }

        //
        // Check if this is a block IO device path.
        //
        BlkIoDevicePath = DevicePathFromHandle(HandleBuffer[Index]);
        if (BlkIoDevicePath == NULL) {
            continue;
        }
        Status = gBS->LocateDevicePath(&gEfiBlockIoProtocolGuid,
                                       &BlkIoDevicePath,
                                       &Handle);
        if (EFI_ERROR(Status)) {
            // Device is not BlockIo;
            continue;
        }

        Status = gBS->HandleProtocol(HandleBuffer[Index],
                                     &gEfiSimpleFileSystemProtocolGuid,
                                     (VOID**)&SfProtocol);

        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: Failed to locate Simple FS protocol. %r\n", __FUNCTION__, Status));
            continue;
        }

        //
        // Open the volume/partition.
        //
        Status = SfProtocol->OpenVolume(SfProtocol, &VolHandle);
        if (EFI_ERROR(Status) != FALSE) {
            DEBUG((DEBUG_ERROR,"%a: Unable to open SimpleFileSystem. Code = %r\n", __FUNCTION__, Status));
            continue;
        }

        //
        // Insure the PktName file is present
        //
        Status = VolHandle->Open (VolHandle, &FileHandle, PktFileName, EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_INFO,"%a: Unable to locate %s. Code = %r\n", __FUNCTION__, PktFileName, Status));
            Status2 = FileHandleClose (VolHandle);
            if (EFI_ERROR(Status2)) {
                DEBUG((DEBUG_ERROR,"%a: Error closing Vol Handle. Code = %r\n", __FUNCTION__, Status2));
            }
            continue;
        }

        FileInfo = FileHandleGetInfo (FileHandle);

        if (FileInfo == NULL) {
            DEBUG((DEBUG_ERROR,"%a: Error getting file info.\n", __FUNCTION__));
            FileHandleClose (FileHandle);
            FileHandleClose (VolHandle);
            continue;
        }

        if ((FileInfo->FileSize == 0) ||
            (FileInfo->FileSize > MAX_ALLOWABLE_DFCI_APPLY_VAR_SIZE)) {
            DEBUG((DEBUG_ERROR,"%a: Invalid file size %d.\n", __FUNCTION__, FileInfo->FileSize));
            Status = EFI_BAD_BUFFER_SIZE;
            FileHandleClose (FileHandle);
            FileHandleClose (VolHandle);
            continue;
        }

        *Buffer = AllocatePool (FileInfo->FileSize);
        if (*Buffer == NULL) {
            FileHandleClose (FileHandle);
            FileHandleClose (VolHandle);
            DEBUG((DEBUG_ERROR,"%a: Unable to allocate buffer.\n", __FUNCTION__));
            Status = EFI_OUT_OF_RESOURCES;
            goto CleanUp; // Fatal error, don't try anymore
        }
        *BufferSize = FileInfo->FileSize;

        DEBUG((DEBUG_INFO,"Reading file into buffer @ %p, size = %d\n",*Buffer, *BufferSize));

        ReadSize = FileInfo->FileSize;
        Status = FileHandleRead (FileHandle, &ReadSize, *Buffer);
        if (EFI_ERROR(Status) || (ReadSize != FileInfo->FileSize)) {
            FileHandleClose (FileHandle);
            FileHandleClose (VolHandle);
            DEBUG((DEBUG_ERROR,"%a: Unable to read file. ReadSize=%d, Size=%d. Code=%r\n",
                               __FUNCTION__,
                               ReadSize,
                               FileInfo->FileSize,
                               Status));

            if (Status == EFI_SUCCESS) {
                Status = EFI_BAD_BUFFER_SIZE;
            }
            FreePool (*Buffer);
            *Buffer = NULL;
            continue;
        }
        DEBUG((DEBUG_INFO,"Finished Reading File\n"));
        FileHandleClose (FileHandle);
        FileHandleClose (VolHandle);
        Status = EFI_SUCCESS;
        break;
    }

CleanUp:
    if (HandleBuffer != NULL) {
        FreePool(HandleBuffer);
    }

    DEBUG((DEBUG_INFO,"Exit reading file\n"));

    return Status;
}

/**
*  Process the type of packet selected
*
*  @param[in]  Pkt                   Which type of packet to load
*
*  @retval EFI_SUCCESS           Pkt processed.
*  @retval EFI_NOT_FOUND         No update found
*
**/
STATIC
EFI_STATUS
ProcessPacket (
    IN  DFCI_USB_PACKET_TYPE  Pkt
  ) {

    CHAR8      *Buffer;
    UINTN       BufferSize;
    CHAR16     *Ext;
    UINTN       i;
    CHAR8      *Manufacturer;
    UINTN       ManufacturerSize;
    CHAR16     *PktFileName;
    UINTN       PktNameLen;
    CHAR8      *ProductName;
    UINTN       ProductNameSize;
    CHAR8      *SerialNumber;
    UINTN       SerialNumberSize;
    EFI_STATUS  Status;
    CHAR16     *VariableName;
    EFI_GUID   *VariableGuid;
    UINT32      Attributes;


    Manufacturer = NULL;
    ProductName = NULL;
    SerialNumber = NULL;
    PktFileName = NULL;

    switch (Pkt) {
        case IdentityPkt:
           VariableName = DFCI_IDENTITY_APPLY_VAR_NAME;
           VariableGuid = &gDfciAuthProvisionVarNamespace;
           Attributes = DFCI_IDENTITY_VAR_ATTRIBUTES;
           Ext = L".xid";
           break;

        case Identity2Pkt:
           VariableName = DFCI_IDENTITY2_APPLY_VAR_NAME;
           VariableGuid = &gDfciAuthProvisionVarNamespace;
           Attributes = DFCI_IDENTITY_VAR_ATTRIBUTES;
           Ext = L".xi2";
           break;

        case PermissionsPkt:
           VariableName = DFCI_PERMISSION_POLICY_APPLY_VAR_NAME;
           VariableGuid = &gDfciPermissionManagerVarNamespace;
           Attributes = DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES;
           Ext = L".xps";
           break;

        case Permissions2Pkt:
           VariableName = DFCI_PERMISSION2_POLICY_APPLY_VAR_NAME;
           VariableGuid = &gDfciPermissionManagerVarNamespace;
           Attributes = DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES;
           Ext = L".xp2";
           break;

        case SettingsPkt:
           VariableName = DFCI_SETTINGS_APPLY_INPUT_VAR_NAME;
           VariableGuid = &gDfciSettingsManagerVarNamespace;
           Attributes = DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES;
           Ext = L".xss";
           break;

        case Settings2Pkt:
           VariableName = DFCI_SETTINGS2_APPLY_INPUT_VAR_NAME;
           VariableGuid = &gDfciSettingsManagerVarNamespace;
           Attributes = DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES;
           Ext = L".xs2";
           break;

        default:
           return EFI_INVALID_PARAMETER;
    }

    Status = DfciIdSupportGetSerialNumber (&SerialNumber, &SerialNumberSize);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to get ProductName. Code=%r\n", Status));
        goto Error;
    }

    Status = DfciIdSupportGetManufacturer (&Manufacturer, &ManufacturerSize);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to get ProductName. Code=%r\n", Status));
        goto Error;
    }

    Status = DfciIdSupportGetProductName (&ProductName, &ProductNameSize);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to get ProductName. Code=%r\n", Status));
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
                                           SerialNumber,
                                           ProductName,
                                           Manufacturer);

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

    Status = StrCatS (PktFileName, MAX_USB_FILE_NAME_LENGTH * sizeof(CHAR16), Ext);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to append the file name ext. Code=%r\n", Status));
        goto Error;
    }

    Status = FindUsbDriveWithDfciUpdate (PktFileName, &Buffer, &BufferSize);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to read update. Code=%r\n", Status));
    } else {
        DEBUG((DEBUG_INFO,"gRT=%p, Writing variable from buffer @ %p size=%d\n", gRT, Buffer, BufferSize));
        Status = gRT->SetVariable(VariableName, VariableGuid, Attributes, BufferSize, Buffer);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Unable to set mailbox %s. Code = %r\n", Status));
        }  else {
            DEBUG((DEBUG_INFO, "Mailbox %s setup\n", VariableName));
        }

        FreePool (Buffer);
    }

Error:
    if (NULL != Manufacturer) {
        FreePool (Manufacturer);
    }

    if (NULL != ProductName) {
        FreePool (ProductName);
    }

    if (NULL != Manufacturer) {
        FreePool (SerialNumber);
    }

    mPktName[Pkt] = PktFileName;
    if (EFI_ERROR(Status)) {
        mPktResponse[Pkt] = USER_STATUS_NO_FILE;
    } else {
        mPktResponse[Pkt] = USER_STATUS_SUCCESS;
    }
    mPktStatus[Pkt] = Status;

    return Status;
}

/**
*
*  Request all of the packets from the USB drive.
*
*  @param[in]     HiiHandle       TO get the message strings.
*  @param [out]   StatusText      Names of the files and their status
*
*  @retval   EFI_SUCCESS     Always returns success.
*
**/
EFI_STATUS
EFIAPI
DfciUsbRequestProcess (
    IN  EFI_HII_HANDLE   HiiHandle,
    OUT CHAR16         **StatusText
  ) {

    UINTN       i;
    CHAR16     *Msg;
    UINTN       MsgSize;
    EFI_STRING  StatusSuccess;
    EFI_STRING  StatusNotFound;
    EFI_STRING  StatusFailed;


    ProcessPacket (IdentityPkt);
    ProcessPacket (Identity2Pkt);
    ProcessPacket (PermissionsPkt);
    ProcessPacket (Permissions2Pkt);
    ProcessPacket (SettingsPkt);
    ProcessPacket (Settings2Pkt);

    StatusSuccess  = HiiGetString(HiiHandle, STRING_TOKEN(STR_DFCI_MB_SUCCESS), NULL);
    StatusNotFound = HiiGetString(HiiHandle, STRING_TOKEN(STR_DFCI_MB_NOT_FOUND), NULL);
    StatusFailed   = HiiGetString(HiiHandle, STRING_TOKEN(STR_DFCI_MB_FAILED), NULL);

    //
    // Send the previous results to the Settings Manager
    //
    MsgSize = 0;
    for (i=0; i < MAX_PACKET_TYPE; i++) {
        if (mPktName[i] != NULL) {
            MsgSize += StrSize (mPktName[i]);
            switch (mPktStatus[i]) {
                case EFI_NOT_FOUND:
                    if (StatusNotFound != NULL) {
                        MsgSize += StrSize (StatusNotFound);
                    }
                    break;
                case EFI_SUCCESS:
                    if (StatusSuccess != NULL) {
                        MsgSize += StrSize (StatusSuccess);
                    }
                    break;
                default:
                    if (StatusFailed != NULL) {
                        MsgSize += StrSize (StatusFailed);
                    }
                    break;
            }
            MsgSize += StrSize (L"\n");
        }
    }

    //
    // Now, ask the Settings Manager for new settings
    //
    Msg = AllocatePool(MsgSize);
    if (Msg != NULL) {
        Msg[0] = L'\0';
        for (i=0; i < MAX_PACKET_TYPE; i++) {
            if (mPktName[i] != NULL) {
                StrCatS(Msg, MsgSize, mPktName[i]);

                switch (mPktStatus[i]) {
                    case EFI_NOT_FOUND:
                        if (StatusNotFound != NULL) {
                            StrCatS(Msg, MsgSize, StatusNotFound);
                        }
                        break;
                    case EFI_SUCCESS:
                        if (StatusSuccess != NULL) {
                            StrCatS(Msg, MsgSize, StatusSuccess);
                        }
                        break;
                    default:
                        if (StatusFailed != NULL) {
                            StrCatS(Msg, MsgSize, StatusFailed);
                        }
                        break;
                }

                StrCatS(Msg, MsgSize, L"\n");
                FreePool (mPktName[i]);
                mPktName[i] = NULL;
            }
        }
    }

    if (StatusSuccess != NULL) {
        FreePool (StatusSuccess);
    }

    if (StatusSuccess != NULL) {
        FreePool (StatusNotFound);
    }

    if (StatusSuccess != NULL) {
        FreePool (StatusFailed);
    }

    *StatusText = Msg;;

    return EFI_SUCCESS;
}