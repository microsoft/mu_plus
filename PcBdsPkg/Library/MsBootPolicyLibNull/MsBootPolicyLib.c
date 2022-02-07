/** @file
Null Power Services library class to support Platforms that dont have battery

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/MsBootPolicyLib.h>
#include <Library/DeviceBootManagerLib.h>
#include <Library/MsPlatformDevicesLib.h>

/**
 *Ask if the platform is requesting Settings Change

 *@retval TRUE     System is requesting Settings Change
 *@retval FALSE    System is not requesting Changes.
**/
BOOLEAN
EFIAPI
MsBootPolicyLibIsSettingsBoot (
  VOID
  )
{
  return FALSE;
}

/**
 *Ask if the platform is requesting an Alternate Boot

 *@retval TRUE     System is requesting Alternate Boot
 *@retval FALSE    System is not requesting AltBoot.
**/
BOOLEAN
EFIAPI
MsBootPolicyLibIsAltBoot (
  VOID
  )
{
  return FALSE;
}

/**

 *Ask if the specified device is bootable

 *@retval TRUE     System is requesting Alternate Boot
 *@retval FALSE    System is not requesting AltBoot.
**/
BOOLEAN
EFIAPI
MsBootPolicyLibIsDeviceBootable (
  EFI_HANDLE  ControllerHandle
  )
{
  return FALSE;
}

/**

 *Ask if the specified device path is bootable

 *@retval TRUE     System is requesting Alternate Boot
 *@retval FALSE    System is not requesting AltBoot.
**/
BOOLEAN
EFIAPI
MsBootPolicyLibIsDevicePathBootable (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  return FALSE;
}

/**
  Asks the platform if the DevicePath provided is a valid bootable 'USB' device.
  USB here indicates the port connection type not the device protocol.
  With TBT or USB4 support PCIe storage devices are valid 'USB' boot options.

  Default implementation:
    The platform alone determines if DevicePath is valid for USB boot support.

  @param DevicePath Pointer to DevicePath to check

  @retval TRUE     Device is a valid USB boot option
  @retval FALSE    Device is not a valid USB boot option
 **/
BOOLEAN
EFIAPI
MsBootPolicyLibIsDevicePathUsb (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  return PlatformIsDevicePathUsb (DevicePath);
}

/**
 *Ask if the platform is requesting an Alternate Boot

 *@retval EFI_SUCCESS  BootSequence pointer returned
 *@retval Other        Error getting boot sequence

 BootSequence is assumed to be a pointer to constant data, and
 is not freed by the caller.

**/
EFI_STATUS
EFIAPI
MsBootPolicyLibGetBootSequence (
  BOOT_SEQUENCE  **BootSequence,
  BOOLEAN        AltBootRequest
  )
{
  return EFI_UNSUPPORTED;
}

/**
 *Clears the boot requests for settings or Alt boot

 *@retval EFI_SUCCESS  Boot requests cleared
 *@retval Other        Error clearing requests

**/
EFI_STATUS
EFIAPI
MsBootPolicyLibClearBootRequests (
  VOID
  )
{
  return EFI_SUCCESS;
}
