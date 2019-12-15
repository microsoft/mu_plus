/** @file
  Device Boot Manager library definition. A device can implement
  instances to support device specific behavior.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DEVICE_BOOT_MANAGER_LIB_H_
#define __DEVICE_BOOT_MANAGER_LIB_H_

#include <Library/UefiBootManagerLib.h>

//
/// Console Type Information
///
#define CONSOLE_OUT 0x00000001
#define STD_ERROR   0x00000002
#define CONSOLE_IN  0x00000004
#define CONSOLE_ALL (CONSOLE_OUT | CONSOLE_IN | STD_ERROR)

#define OEM_REBOOT_TO_SETUP_KEY          (MAX_2_BITS | 0x4000 )
#define OEM_REBOOT_TO_SETUP_OS           (MAX_2_BITS | 0x4001 )
#define OEM_PREVIOUS_SECURITY_VIOLATION  (MAX_2_BITS | 0x4002 )

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     ConnectType;
} BDS_CONSOLE_CONNECT_ENTRY;

/**
  OnDemandConInConnect
 */
EFI_DEVICE_PATH_PROTOCOL **
EFIAPI
DeviceBootManagerOnDemandConInConnect (
  VOID
  );

/**
  Do the device specific action at start of BDS
**/
VOID
EFIAPI
DeviceBootManagerBdsEntry (
  VOID
  );

/**
  Do the device specific action before the console is connected.

  Such as:
      Initialize the platform boot order
      Supply Console information
**/
EFI_HANDLE
EFIAPI
DeviceBootManagerBeforeConsole (
  EFI_DEVICE_PATH_PROTOCOL    **DevicePath,
  BDS_CONSOLE_CONNECT_ENTRY   **PlatformConsoles
  );

/**
  Do the device specific action after the console is connected.

  Such as:
**/
EFI_DEVICE_PATH_PROTOCOL **
EFIAPI
DeviceBootManagerAfterConsole (
  VOID
  );

/**
ProcessBootCompletion
*/
VOID
EFIAPI
DeviceBootManagerProcessBootCompletion (
  IN EFI_BOOT_MANAGER_LOAD_OPTION *BootOption
  );

/**
 * Check for HardKeys during boot.  If the hard keys are pressed, builds
 * a boot option for the specific hard key setting.
 *
 *
 * @param BootOption   - Boot Option filled in based on which hard key is pressed
 *
 * @return EFI_STATUS  - EFI_NOT_FOUND - no hard key pressed, no BootOption
 *                       EFI_SUCCESS   - BootOption is valid
 *                       other error   - Unable to build BootOption
 */
EFI_STATUS
EFIAPI
DeviceBootManagerPriorityBoot (
  EFI_BOOT_MANAGER_LOAD_OPTION   *BootOption
  );

/**
 This is called from BDS right before going into front page
 when no bootable devices/options found
*/
VOID
EFIAPI
DeviceBootManagerUnableToBoot (
  VOID
  );

#endif
