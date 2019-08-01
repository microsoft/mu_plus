/** @file
DfciDeviceIdVariable.h

Contains definitions for Device Id variables. This provides information
on the version of DFCI that is supported

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_DEVICE_ID_VARIABLES_H__
#define __DFCI_DEVICE_ID_VARIABLES_H__

//
// Variable namespace
//
extern EFI_GUID gDfciDeviceIdVarNamespace;

#define DFCI_DEVICE_ID_VAR_NAME   L"DfciDeviceIdentifier"
#define DFCI_DEVICE_ID_VAR_ATTRIBUTES    (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)


#define DFCI_DEVICE_ID_VERSION (1)
#define DFCI_VERSION           (2)

#define MAX_ALLOWABLE_DFCI_DEVICE_ID_VARIABLE_SIZE (1024 * 2)  //2kb


#endif // __DFCI_DEVICE_ID_VARIABLES_H__
