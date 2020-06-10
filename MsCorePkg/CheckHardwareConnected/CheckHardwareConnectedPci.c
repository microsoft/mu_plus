/** @file
  Contains PCI functionality used by this driver.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/Pci.h>
#include <Library/BaseLib.h>
#include <Library/DeviceSpecificBusInfoLib.h>
#include <Protocol/PciIo.h>

#include "CheckHardwareConnected.h"

typedef enum {
  PciDevice,
  PciP2pBridge,
  PciCardBusBridge,
  PciUndefined
} PCI_HEADER_TYPE;

typedef union {
  PCI_DEVICE_HEADER_TYPE_REGION Device;
  PCI_BRIDGE_CONTROL_REGISTER   Bridge;
  PCI_CARDBUS_CONTROL_REGISTER  CardBus;
} NON_COMMON_UNION;

typedef struct {
  PCI_DEVICE_INDEPENDENT_REGION Common;
  NON_COMMON_UNION              NonCommon;
  UINT32                        Data[48];
} PCI_CONFIG_SPACE;

/**
  Locate capability register block per capability ID.

  @param[in] ConfigSpace       Data in PCI configuration space.
  @param[in] CapabilityId      The capability ID.

  @return   The offset of the register block per capability ID,
            or 0 if the register block cannot be found.
**/
UINT8
LocatePciCapability (
  IN PCI_CONFIG_SPACE   *ConfigSpace,
  IN UINT8              CapabilityId
  )
{
  UINT8                   CapabilityPtr;
  EFI_PCI_CAPABILITY_HDR  *CapabilityEntry;

  //
  // To check the capability of this device supports
  //
  if ((ConfigSpace->Common.Status & EFI_PCI_STATUS_CAPABILITY) == 0) {
    return 0;
  }

  switch ((PCI_HEADER_TYPE) (ConfigSpace->Common.HeaderType & HEADER_LAYOUT_CODE)) {
    case PciDevice:
      CapabilityPtr = ConfigSpace->NonCommon.Device.CapabilityPtr;
      break;
    case PciP2pBridge:
      CapabilityPtr = ConfigSpace->NonCommon.Bridge.CapabilityPtr;
      break;
    case PciCardBusBridge:
      CapabilityPtr = ConfigSpace->NonCommon.CardBus.Cap_Ptr;
      break;
    default:
      return 0;
  }

  while ((CapabilityPtr >= 0x40) && ((CapabilityPtr & 0x03) == 0x00)) {
    CapabilityEntry = (EFI_PCI_CAPABILITY_HDR *) ((UINT8 *) ConfigSpace + CapabilityPtr);
    if (CapabilityEntry->CapabilityID == CapabilityId) {
      return CapabilityPtr;
    }

    //
    // Certain PCI devices may incorrectly have the capability pointer pointing to itself,
    // break to avoid dead loop.
    //
    if (CapabilityPtr == CapabilityEntry->NextItemPtr) {
      break;
    }

    CapabilityPtr = CapabilityEntry->NextItemPtr;
  }

  return 0;
}

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
  )
{
  EFI_STATUS            Status;
  PCI_CONFIG_SPACE      ConfigSpace;
  UINTN                 Seg;
  UINTN                 Bus;
  UINTN                 Dev;
  UINTN                 Fun;
  UINT8                 PcieCapabilityPtr;
  PCI_CAPABILITY_PCIEXP *DevicePciExpressCapability;

  if (DevicePciIoProtocol == NULL || DeviceLinkSpeed == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = DevicePciIoProtocol->GetLocation (DevicePciIoProtocol, &Seg, &Bus, &Dev, &Fun);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DevicePciIoProtocol->Pci.Read (
                                      DevicePciIoProtocol,
                                      EfiPciIoWidthUint8,
                                      0,
                                      sizeof (ConfigSpace),
                                      &ConfigSpace
                                      );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PcieCapabilityPtr = LocatePciCapability (&ConfigSpace, EFI_PCI_CAPABILITY_ID_PCIEXP);
  if (PcieCapabilityPtr == 0) {
    return EFI_NOT_FOUND;
  }

  DevicePciExpressCapability = (PCI_CAPABILITY_PCIEXP *) ((UINT8 *) &ConfigSpace + PcieCapabilityPtr);
  switch (DevicePciExpressCapability->LinkStatus.Bits.CurrentLinkSpeed) {
    case 1:
      *DeviceLinkSpeed = Gen1;
      break;
    case 2:
      *DeviceLinkSpeed = Gen2;
      break;
    case 3:
      *DeviceLinkSpeed = Gen3;
      break;
    case 4:
      *DeviceLinkSpeed = Gen4;
      break;
    case 5:
      *DeviceLinkSpeed = Gen5;
      break;
    case 6:
      *DeviceLinkSpeed = Gen6;
      break;
    default:
      *DeviceLinkSpeed = Unknown;
      break;
  }

  return EFI_SUCCESS;
}
