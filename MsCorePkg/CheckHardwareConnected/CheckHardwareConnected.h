/** @file
  Shared definitions used by this driver.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CHECK_HARDWARE_CONNECTED_H__
#define __CHECK_HARDWARE_CONNECTED_H__

/**
  Gets the PCIe link speed for the device represented by the given PCI IO protocol instance.

  @param[in]  DevicePciIoProtocol   A pointer to a EFI_PCI_IO_PROTOCOL instance which represents the device
                                    whose link speed should be retrieved.
  @param[out] DeviceLinkSpeed       A pointer that will be set to the link speed of the device if found
                                    successfully. If there was not an error finding the link speed but the
                                    value is unknown the value will be set to Unknown.

  @retval     EFI_SUCCESS           The link speed was found and set successfully.
  @retval     EFI_INVALID_PARAMETER A required pointer parameter is NULL.
  @retval     EFI_NOT_FOUND         The PCI capabilities could not be found for the given device.

**/
EFI_STATUS
GetPciExpressDeviceLinkSpeed (
  IN    EFI_PCI_IO_PROTOCOL   *DevicePciIoProtocol,
  OUT   PCIE_LINK_SPEED       *DeviceLinkSpeed
  );

#endif
