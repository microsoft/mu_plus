/** @file
 *MsPlatformDevicesLib  - Device specific library.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DeviceBootManagerLib.h>
#include <Library/MsPlatformDevicesLib.h>
/**
Library function used to proivide the platform SD Card device path
**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
GetSdCardDevicePath (
  VOID
)
{
  return NULL;
}

/**
Library function used to proivide the list of platform devices that MUST be
connected at the beginning of BDS
**/
EFI_DEVICE_PATH_PROTOCOL **
EFIAPI
GetPlatformConnectList (
  VOID
)
{
  return NULL;
}

/**
 * Library function used to provide the list of platform console devices.
 */
BDS_CONSOLE_CONNECT_ENTRY *
EFIAPI
GetPlatformConsoleList (
  VOID
)
{
  return NULL;
}

/**
Library function used to provide the list of platform devices that MUST be connected
to support ConsoleIn activity.  This call occurs on the ConIn connect event, and
allows platforms to do specific enablements for ConsoleIn support.
**/
EFI_DEVICE_PATH_PROTOCOL **
EFIAPI
GetPlatformConnectOnConInList (
  VOID
)
{
  return NULL;
}

/**
Library function used to provide the console type.  For ConType == DisplayPath,
device path is filled in to the exact controller to use.  For other ConTypes, DisplayPath
must NULL. The device path must NOT be freed.
**/
EFI_HANDLE
EFIAPI
GetPlatformPreferredConsole (
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
)
{
  return NULL;
}
