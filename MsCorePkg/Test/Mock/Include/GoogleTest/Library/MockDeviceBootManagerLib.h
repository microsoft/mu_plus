/** @file MockDeviceBootManagerLib.h
  Google Test mocks for DeviceBootManagerLib.

  Copyright (c) Microsoft Corporation.
  Your use of this software is governed by the terms of the Microsoft agreement under which you obtained the software.
**/

#ifndef MOCK_DEVICE_BOOT_MANAGER_LIB_H_
#define MOCK_DEVICE_BOOT_MANAGER_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/DeviceBootManagerLib.h>
}

struct MockDeviceBootManagerLib {
  MOCK_INTERFACE_DECLARATION (MockDeviceBootManagerLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    DeviceBootManagerPriorityBoot,
    (EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    DeviceBootManagerUnableToBoot,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    DeviceBootManagerProcessBootCompletion,
    (EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL **,
    DeviceBootManagerOnDemandConInConnect,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    DeviceBootManagerBdsEntry,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_HANDLE,
    DeviceBootManagerBeforeConsole,
    (EFI_DEVICE_PATH_PROTOCOL   **DevicePath,
     BDS_CONSOLE_CONNECT_ENTRY  **PlatformConsoles)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL **,
    DeviceBootManagerAfterConsole,
    ()
    );
};

#endif
