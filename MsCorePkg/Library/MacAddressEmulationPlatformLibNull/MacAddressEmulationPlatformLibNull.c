/** @file

  NULL implementation of the platform library that provides hooks for the MAC Address Emulation feature.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/MacAddressEmulationPlatformLib.h>

/**
  Reports whether MacEmulation is enabled, and returns the address to emulate.

  @param[out]  Address - Pointer to a buffer to receive the desired MAC address to use if emulation is enabled.

  @retval  EFI_UNSUPPORTED - Feature is not enabled/supported by the platrform.
  @retval  EFI_STATUS      - Status of other calls in the function.
**/
EFI_STATUS
IsMacEmulationEnabled (
  OUT EFI_MAC_ADDRESS *Address
  )
{

/*
      < OEM TODO >

  Typical checks here include examining runtime scenarios such as a board SKU ID, factory provisioning data,
  user configuration data (UEFI setup menu), etc.  If not enabled, the function should return EFI_UNSUPPORTED.

  If enabled, the *Address buffer needs to be updated with the desired MAC address to use with the platform
  prior to returning EFI_SUCCESS.

  This is also a good place to make sure any settings that need to be secured are done so properly.  For
  example, if data is stored in variable services, the access policies should be implemented to make sure
  end users can not unexpectedly enable or disable the feature.
*/

  return EFI_UNSUPPORTED;
}

/**
  Executes platform-specific logic to determine if MAC-address emulation should be supported by a specific
  SNP controller.

  @param[in]  SnpHandle - Controller handle for which to determine support.

  @retval TRUE  - Device represented by SnpHandle supports MAC emulation.
  @retval FALSE - Device represented by SnpHandle does not support MAC emulation.
**/
BOOLEAN
PlatformMacEmulationSnpCheck (
  IN  EFI_HANDLE SnpHandle
  )
{

/*
      < OEM TODO >

  Perform any logic necessary to determine if the controller represeted by the input handle should have the MAC
  address emulated.

  For example, if only a specific USB network adapter with a certain Vendor ID should be supported, the
  following pseudo code can be implemented.

    // Use the SnpHandle to get the proper USB IO protocol to that device
    SnpDevicePath = DevicePathFromHandle (SnpHandle)
    gBS->LocateDevicePath (&gEfiUsbIoProtocolGuid, &SnpDevicePath, &UsbHandle)
    gBS->HandleProtocol (UsbHandle, &gEfiUsbIoProtocolGuid, &UsbIo)
  
    // Read the Vendor ID through the USB IO protocol and return if supported
    UsbIo->UsbGetDeviceDescriptor (UsbIo, &UsbDeviceDescriptor)
    if (UsbDeviceDescriptor.IdVendor == EXPECTED_VENDOR_ID)
      return TRUE
    else
      return FALSE
*/

  return FALSE;
}

/**
  Execute any platform or network controller specific code required to implement MAC-address emulation needed in
  addition to a call to the UEFI API EFI_SIMPLE_NETWORK_PROTOCOL::StationAddress().

  @param[in]  Address - MAC Address to use for emulation

  @retval  EFI_STATUS
**/
EFI_STATUS
PlatformMacEmulationEnable (
  IN  EFI_MAC_ADDRESS *Address
  )
{

/*
      < OEM TODO >

  The MacAddressEmulationDxe driver will perform a call to the UEFI's API called EFI_SIMPLE_NETWORK_PROTOCOL::StationAddress()
  to set the requested MAC address, but if the secific network controller used by the platform requires further support, it can
  be implemented in this function.

  For example, an OS driver may require a packet published to the ACPI table for authentication, new MAC address, etc.  This
  function can be used to establish a callback when the specific ACPI table is installed and in the callback modify the table
  with the necessary information.
*/

  return EFI_SUCCESS;
}

