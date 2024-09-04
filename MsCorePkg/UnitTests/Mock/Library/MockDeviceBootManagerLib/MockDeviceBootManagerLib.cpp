/** @file MockDeviceBootManagerLib.cpp
  Google Test mocks for DeviceBootManagerLib.

  Copyright (c) Microsoft Corporation.
  Your use of this software is governed by the terms of the Microsoft agreement under which you obtained the software.
**/

#include <GoogleTest/Library/MockDeviceBootManagerLib.h>

MOCK_INTERFACE_DEFINITION (MockDeviceBootManagerLib);
MOCK_FUNCTION_DEFINITION (MockDeviceBootManagerLib, DeviceBootManagerPriorityBoot, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDeviceBootManagerLib, DeviceBootManagerUnableToBoot, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDeviceBootManagerLib, DeviceBootManagerProcessBootCompletion, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDeviceBootManagerLib, DeviceBootManagerOnDemandConInConnect, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDeviceBootManagerLib, DeviceBootManagerBdsEntry, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDeviceBootManagerLib, DeviceBootManagerBeforeConsole, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDeviceBootManagerLib, DeviceBootManagerAfterConsole, 0, EFIAPI);
