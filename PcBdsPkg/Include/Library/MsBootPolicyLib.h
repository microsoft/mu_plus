/** @file
 *Header file for Ms Boot Policy Library

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MS_BOOT_POLICY_LIB_H_
#define _MS_BOOT_POLICY_LIB_H_

/**
 * BOOT_SEQUENCE types are used to describe classes of devices.  The identifiers are used in
 * boot policy applications to create boot sequences for the EFI_LOAD_OPTIONS. An example
 * application is at PcsBdsPkg/MsBootPolicy.
 *
 * Boot Policy Applications can be created to meet a platform's requirements. The PcBdsPkg
 * example application impelements USB, PXE and HDD boot sequences, in addition to a default
 * sequences of HDD, USB, PXE.
**/
typedef enum {
  MsBootDone,       /// Boot Sequence terminator, used to exit boot application
  MsBootPXE4,       /// Boot devices that support IPV4 PXE
  MsBootPXE6,       /// Boot devices supporting IPV6 PXE
  MsBootHDD,        /// Hard Drive type boot devices
  MsBootUSB,        /// Boot Devices containing a Usb Device
  MsBootNVME,       /// Nvme boot devices
  MsBootODD,        /// Optical Disk drive devices
  MsBootSD,         /// Sd/Emmc type devices
  MsBootRAMDISK     /// Ram Disk devices
} BOOT_SEQUENCE;

/**
 *Ask if the platform is requesting Settings Change

 *@retval TRUE     System is requesting Settings Change
 *@retval FALSE    System is not requesting Changes.
**/
BOOLEAN
EFIAPI
MsBootPolicyLibIsSettingsBoot (
  VOID
  );

/**
 *Ask if the platform is requesting an Alternate Boot

 *@retval TRUE     System is requesting Alternate Boot
 *@retval FALSE    System is not requesting AltBoot.
**/
BOOLEAN
EFIAPI
MsBootPolicyLibIsAltBoot (
  VOID
  );

/**

 *Ask if the specified device is bootable

 *@retval TRUE     System is requesting Alternate Boot
 *@retval FALSE    System is not requesting AltBoot.
**/
BOOLEAN
EFIAPI
MsBootPolicyLibIsDeviceBootable (
  EFI_HANDLE  ControllerHandle
  );

/**

 *Ask if the specified device path is bootable

 *@retval TRUE     System is requesting Alternate Boot
 *@retval FALSE    System is not requesting AltBoot.
**/
BOOLEAN
EFIAPI
MsBootPolicyLibIsDevicePathBootable (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Asks the platform if the DevicePath provided is a valid bootable 'USB' device.
  USB here indicates the port connection type not the device protocol.
  With TBT or USB4 support PCIe storage devices are valid 'USB' boot options.

  @param DevicePath Pointer to DevicePath to check

  @retval TRUE     Device is a valid USB boot option
  @retval FALSE    Device is not a valid USB boot option
 **/
BOOLEAN
EFIAPI
MsBootPolicyLibIsDevicePathUsb (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

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
  );

/**
 *Clears the boot requests for settings or Alt boot

 *@retval EFI_SUCCESS  Boot requests cleared
 *@retval Other        Error clearing requests

**/
EFI_STATUS
EFIAPI
MsBootPolicyLibClearBootRequests (
  VOID
  );

#endif
