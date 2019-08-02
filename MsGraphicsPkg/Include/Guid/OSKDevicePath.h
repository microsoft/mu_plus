/**

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __OSK_DEVICE_PATH_GUID_H__
#define __OSK_DEVICE_PATH_GUID_H__

///
/// Vendor Device Path definition.
///
typedef struct {
    VENDOR_DEVICE_PATH             VendorDevicePath;
    EFI_DEVICE_PATH_PROTOCOL       End;
} OSK_DEVICE_PATH;


#define OSK_DEVICE_PATH_GUID  {0xad603516, 0xd94a, 0x425e, {0x9b, 0x42, 0x0b, 0x55, 0xfd, 0xdb, 0xd8, 0xa1}}

//This file describes the Guid for the OSK Vendor DevicePath that is installed by the OnScreenKeyboard Driver
extern EFI_GUID gOSKDevicePathGuid;


#endif
