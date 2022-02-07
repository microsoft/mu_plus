/** @file
DfciOemSample.h

Definition of OEM settings provided by DfciPkg sample providers

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_OEM_SAMPLE_H__
#define __DFCI_OEM_SAMPLE_H__

//
// OEM Settings
//

// Device setting for use with DfciPkg\Library\DfciVirtualizationSettings
#define DFCI_OEM_SETTING_ID__ENABLE_VIRT_SETTINGS  "Device.CpuAndIoVirtualization.Enable"

// Device setting for use with DfciPkg\Library\DfciPasswordProvider
#define DFCI_OEM_SETTING_ID__PASSWORD  "Device.Password.Password"

#endif //  __DFCI_OEM_SAMPLE_H__
