/** @file
Contains definitions for Device Id variables. This provides information*
on the version of DFCI that is supported


Copyright (c) 2018, Microsoft Corporation. All rights reserved.

**/

#ifndef __DFCI_DEVICE_ID_VARIABLES_H__
#define __DFCI_DEVICE_ID_VARIABLES_H__

//
// Variable namespace
//
extern EFI_GUID gDfciDeviceIdVarNamespace;

#define DFCI_DEVICE_ID_VAR_NAME   L"UEFIDeviceIdentifier"
#define DFCI_DEVICE_ID_VAR_ATTRIBUTES    (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)


#define DFCI_DEVICE_ID_VERSION (1)
#define DFCI_VERSION           (2)

#define DFCI_PERMISSION_POLICY_RESULT_VERSION (1)

#define MAX_ALLOWABLE_DFCI_DEVICE_ID_VARIABLE_SIZE (1024 * 2)  //2kb


#endif // __DFCI_DEVICE_ID_VARIABLES_H__
