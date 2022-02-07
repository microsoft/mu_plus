/** @file
DfciSettings.h

Definition of the Group Settings

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_SETTINGS_H__
#define __DFCI_SETTINGS_H__

// Settings name convention:

// Definer.SettingName.Type
//
// Some settings types are asymmetric in that the types are different for getting and
// updating a setting. The HttpsCert, for example, is set with a binary blob of a
// Certificate. On Get, a string of the thumbprint is returned.

//
// Group Settings
//

// AllCameras Group Setting.
// AllCameras Enable allows the cameras to be enabled.  AllCameras disabled
// ensures all cameras are disabled.  When received as a setting, all cameras
// will be disabled, and individual settings will be disabled, and grayed out.
#define DFCI_STD_SETTING_ID__ALL_CAMERAS  "Dfci.OnboardCameras.Enable"

// Cpu and I/O Virtualization Settings
//
#define DFCI_STD_SETTING_ID__ALL_CPU_IO_VIRT  "Dfci.CpuAndIoVirtualization.Enable"

//
// Onboard Audio devices settings
//
#define DFCI_STD_SETTING_ID__ALL_AUDIO  "Dfci.OnboardAudio.Enable"

//
//  Onboard radios
//
#define DFCI_STD_SETTING_ID__ALL_RADIOS  "Dfci.OnboardRadios.Enable"

//
// Boot from External Media
//
#define DFCI_STD_SETTING_ID__EXTERNAL_MEDIA  "Dfci.BootExternalMedia.Enable"

//
// Boot from On Board Network
//
#define DFCI_STD_SETTING_ID__ENABLE_NETWORK  "Dfci.BootOnboardNetwork.Enable"

//
// Disable simultaneous multi threading support.
//
#define DFCI_STD_SETTING_ID_V3_ENABLE_SMT  "Dfci3.ProcessorSMT.Enable"

//
// Disable components known to inject ACPI OS Executable code.
//
#define DFCI_STD_SETTING_ID_V3_ENABLE_WPBT  "Dfci3.OnboardWpbt.Enable"

//
// Set the device ASSET TAG.
//
#define DFCI_STD_SETTING_ID_V3_ASSET_TAG  "Dfci3.AssetTag.String"

//
// Enable or Disable Bluetooth devices.
//
#define DFCI_STD_SETTING_ID_V4_ENABLE_BLUETOOTH  "Dfci4.Bluetooth.Enable"

//
// Enable or Disable Cellular data devices.
//
#define DFCI_STD_SETTING_ID_V4_ENABLE_WWAN  "Dfci4.WWANEnable"

//
// Enable or Disable Near Field Communication devices.
//
#define DFCI_STD_SETTING_ID_V4_ENABLE_NFC  "Dfci4.Nfc.Enable"

//
// Enable or Disable WiFi devices
//
#define DFCI_STD_SETTING_ID_V4_ENABLE_WIFI  "Dfci4.WiFi.Enable"

//
// Enable or Disable all USB Type A ports
//
#define DFCI_STD_SETTING_ID_V4_ENABLE_USBA  "Dfci4.UsbTypeAPort.Enable"

//
// Enable or Disable all USB Type C ports
//
#define DFCI_STD_SETTING_ID_V4_ENABLE_USBC  "Dfci4.UsbTypeCPort.Enable"

//
// Enable or Disable Wake On Lan.
//
#define DFCI_STD_SETTING_ID_V4_ENABLE_WAKE_ON_LAN  "Dfci4.WakeOnLan.Enable"

//
// Enable or Disable Wake On Power.
//
#define DFCI_STD_SETTING_ID_V4_ENABLE_WAKE_ON_POWER  "Dfci4.WakeOnPower.Enable"

//
// Enable or Disable the Front Camera(s).
//
#define DFCI_STD_SETTING_ID_V4_ENABLE_FRONT_CAMERA  "Dfci4.FrontCamera.Enable"

//
// Enable or Disable the Rear Camera(s).
//
#define DFCI_STD_SETTING_ID_V4_ENABLE_REAR_CAMERA  "Dfci4.RearCamera.Enable"

//
// Enable or Disable the IR Camera(s).
//
#define DFCI_STD_SETTING_ID_V4_ENABLE_IR_CAMERA  "Dfci4.IRCamera.Enable"

//
// Enable or Disable the microphone.
//
#define DFCI_STD_SETTING_ID_V4_ENABLE_MICROPHONE  "Dfci4.Microphone.Enable"

//
// Enable or Disable booting from the SD Card.
//
#define DFCI_STD_SETTING_ID_V4_ENABLE_SDCARD  "Dfci4.Sdcard.Enable"

#endif //  __DFCI_SETTINGS_H__
