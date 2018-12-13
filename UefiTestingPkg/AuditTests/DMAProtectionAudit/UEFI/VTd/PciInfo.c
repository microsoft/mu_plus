/** @file

Copied from Tianocore IntelSiliconPkg\Feature\VTd\IntelVTdDxe for purpose of easy DMAR/BME parsing

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DmaProtection.h"

/**
  Return the index of PCI data.

  @param[in]  VtdIndex          The index used to identify a VTd engine.
  @param[in]  Segment           The Segment used to identify a VTd engine.
  @param[in]  SourceId          The SourceId used to identify a VTd engine and table entry.

  @return The index of the PCI data.
  @retval (UINTN)-1  The PCI data is not found.
**/
UINTN
GetPciDataIndex (
  IN UINTN          VtdIndex,
  IN UINT16         Segment,
  IN VTD_SOURCE_ID  SourceId
  )
{
  UINTN          Index;
  VTD_SOURCE_ID  *PciSourceId;

  if (Segment != mVtdUnitInformation[VtdIndex].Segment) {
    return (UINTN)-1;
  }

  for (Index = 0; Index < mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDeviceDataNumber; Index++) {
    PciSourceId = &mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDeviceData[Index].PciSourceId;
    if ((PciSourceId->Bits.Bus == SourceId.Bits.Bus) &&
        (PciSourceId->Bits.Device == SourceId.Bits.Device) &&
        (PciSourceId->Bits.Function == SourceId.Bits.Function) ) {
      return Index;
    }
  }

  return (UINTN)-1;
}

/**
  Register PCI device to VTd engine.

  @param[in]  VtdIndex              The index of VTd engine.
  @param[in]  Segment               The segment of the source.
  @param[in]  SourceId              The SourceId of the source.
  @param[in]  DeviceType            The DMAR device scope type.
  @param[in]  CheckExist            TRUE: ERROR will be returned if the PCI device is already registered.
                                    FALSE: SUCCESS will be returned if the PCI device is registered.

  @retval EFI_SUCCESS           The PCI device is registered.
  @retval EFI_OUT_OF_RESOURCES  No enough resource to register a new PCI device.
  @retval EFI_ALREADY_STARTED   The device is already registered.
**/
EFI_STATUS
RegisterPciDevice (
  IN UINTN          VtdIndex,
  IN UINT16         Segment,
  IN VTD_SOURCE_ID  SourceId,
  IN UINT8          DeviceType,
  IN BOOLEAN        CheckExist
  )
{
  PCI_DEVICE_INFORMATION           *PciDeviceInfo;
  VTD_SOURCE_ID                    *PciSourceId;
  UINTN                            PciDataIndex;
  UINTN                            Index;
  PCI_DEVICE_DATA                  *NewPciDeviceData;
  EDKII_PLATFORM_VTD_PCI_DEVICE_ID *PciDeviceId;

  PciDeviceInfo = &mVtdUnitInformation[VtdIndex].PciDeviceInfo;

  if (PciDeviceInfo->IncludeAllFlag) {
    //
    // Do not register device in other VTD Unit
    //
    for (Index = 0; Index < VtdIndex; Index++) {
      PciDataIndex = GetPciDataIndex (Index, Segment, SourceId);
      if (PciDataIndex != (UINTN)-1) {
        DEBUG ((DEBUG_INFO, "  RegisterPciDevice: PCI S%04x B%02x D%02x F%02x already registered by Other Vtd(%d)\n", Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function, Index));
        return EFI_SUCCESS;
      }
    }
  }

  PciDataIndex = GetPciDataIndex (VtdIndex, Segment, SourceId);
  if (PciDataIndex == (UINTN)-1) {
    //
    // Register new
    //

    if (PciDeviceInfo->PciDeviceDataNumber >= PciDeviceInfo->PciDeviceDataMaxNumber) {
      //
      // Reallocate
      //
      NewPciDeviceData = AllocateZeroPool (sizeof(*NewPciDeviceData) * (PciDeviceInfo->PciDeviceDataMaxNumber + MAX_VTD_PCI_DATA_NUMBER));
      if (NewPciDeviceData == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      PciDeviceInfo->PciDeviceDataMaxNumber += MAX_VTD_PCI_DATA_NUMBER;
      if (PciDeviceInfo->PciDeviceData != NULL) {
        CopyMem (NewPciDeviceData, PciDeviceInfo->PciDeviceData, sizeof(*NewPciDeviceData) * PciDeviceInfo->PciDeviceDataNumber);
        FreePool (PciDeviceInfo->PciDeviceData);
      }
      PciDeviceInfo->PciDeviceData = NewPciDeviceData;
    }

    ASSERT (PciDeviceInfo->PciDeviceDataNumber < PciDeviceInfo->PciDeviceDataMaxNumber);

    PciSourceId = &PciDeviceInfo->PciDeviceData[PciDeviceInfo->PciDeviceDataNumber].PciSourceId;
    PciSourceId->Bits.Bus = SourceId.Bits.Bus;
    PciSourceId->Bits.Device = SourceId.Bits.Device;
    PciSourceId->Bits.Function = SourceId.Bits.Function;

    DEBUG ((DEBUG_INFO, "  RegisterPciDevice: PCI S%04x B%02x D%02x F%02x", Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function));

    PciDeviceId = &PciDeviceInfo->PciDeviceData[PciDeviceInfo->PciDeviceDataNumber].PciDeviceId;
    if ((DeviceType == EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT) ||
        (DeviceType == EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE)) {
      PciDeviceId->VendorId   = PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS(Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function, PCI_VENDOR_ID_OFFSET));
      PciDeviceId->DeviceId   = PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS(Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function, PCI_DEVICE_ID_OFFSET));
      PciDeviceId->RevisionId = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function, PCI_REVISION_ID_OFFSET));

      DEBUG ((DEBUG_INFO, " (%04x:%04x:%02x", PciDeviceId->VendorId, PciDeviceId->DeviceId, PciDeviceId->RevisionId));

      if (DeviceType == EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT) {
        PciDeviceId->SubsystemVendorId = PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS(Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function, PCI_SUBSYSTEM_VENDOR_ID_OFFSET));
        PciDeviceId->SubsystemDeviceId = PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS(Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function, PCI_SUBSYSTEM_ID_OFFSET));
        DEBUG ((DEBUG_INFO, ":%04x:%04x", PciDeviceId->SubsystemVendorId, PciDeviceId->SubsystemDeviceId));
      }
      DEBUG ((DEBUG_INFO, ")"));
    }

    PciDeviceInfo->PciDeviceData[PciDeviceInfo->PciDeviceDataNumber].DeviceType = DeviceType;

    if ((DeviceType != EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT) &&
        (DeviceType != EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE)) {
      DEBUG ((DEBUG_INFO, " (*)"));
    }
    DEBUG ((DEBUG_INFO, "\n"));

    PciDeviceInfo->PciDeviceDataNumber++;
  } else {
    if (CheckExist) {
      DEBUG ((DEBUG_INFO, "  RegisterPciDevice: PCI S%04x B%02x D%02x F%02x already registered\n", Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function));
      return EFI_ALREADY_STARTED;
    }
  }

  return EFI_SUCCESS;
}

/**
  The scan bus callback function to register PCI device.

  @param[in]  Context               The context of the callback.
  @param[in]  Segment               The segment of the source.
  @param[in]  Bus                   The bus of the source.
  @param[in]  Device                The device of the source.
  @param[in]  Function              The function of the source.

  @retval EFI_SUCCESS           The PCI device is registered.
**/
EFI_STATUS
EFIAPI
ScanBusCallbackRegisterPciDevice (
  IN VOID           *Context,
  IN UINT16         Segment,
  IN UINT8          Bus,
  IN UINT8          Device,
  IN UINT8          Function
  )
{
  VTD_SOURCE_ID           SourceId;
  UINTN                   VtdIndex;
  UINT8                   BaseClass;
  UINT8                   SubClass;
  UINT8                   DeviceType;
  EFI_STATUS              Status;

  VtdIndex = (UINTN)Context;
  SourceId.Bits.Bus = Bus;
  SourceId.Bits.Device = Device;
  SourceId.Bits.Function = Function;

  DeviceType = EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT;
  BaseClass = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_CLASSCODE_OFFSET + 2));
  if (BaseClass == PCI_CLASS_BRIDGE) {
    SubClass = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_CLASSCODE_OFFSET + 1));
    if (SubClass == PCI_CLASS_BRIDGE_P2P) {
      DeviceType = EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE;
    }
  }

  Status = RegisterPciDevice (VtdIndex, Segment, SourceId, DeviceType, FALSE);
  return Status;
}

/**
  Scan PCI bus and invoke callback function for each PCI devices under the bus.

  @param[in]  Context               The context of the callback function.
  @param[in]  Segment               The segment of the source.
  @param[in]  Bus                   The bus of the source.
  @param[in]  Callback              The callback function in PCI scan.

  @retval EFI_SUCCESS           The PCI devices under the bus are scaned.
**/
EFI_STATUS
ScanPciBus (
  IN VOID                         *Context,
  IN UINT16                       Segment,
  IN UINT8                        Bus,
  IN SCAN_BUS_FUNC_CALLBACK_FUNC  Callback
  )
{
  UINT8                   Device;
  UINT8                   Function;
  UINT8                   SecondaryBusNumber;
  UINT8                   HeaderType;
  UINT8                   BaseClass;
  UINT8                   SubClass;
  UINT32                  MaxFunction;
  UINT16                  VendorID;
  UINT16                  DeviceID;
  EFI_STATUS              Status;

  // Scan the PCI bus for devices
  for (Device = 0; Device < PCI_MAX_DEVICE + 1; Device++) {
    HeaderType = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, 0, PCI_HEADER_TYPE_OFFSET));
    MaxFunction = PCI_MAX_FUNC + 1;
    if ((HeaderType & HEADER_TYPE_MULTI_FUNCTION) == 0x00) {
      MaxFunction = 1;
    }
    for (Function = 0; Function < MaxFunction; Function++) {
      VendorID  = PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_VENDOR_ID_OFFSET));
      DeviceID  = PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_DEVICE_ID_OFFSET));
      if (VendorID == 0xFFFF && DeviceID == 0xFFFF) {
        continue;
      }

      Status = Callback (Context, Segment, Bus, Device, Function);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      BaseClass = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_CLASSCODE_OFFSET + 2));
      if (BaseClass == PCI_CLASS_BRIDGE) {
        SubClass = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_CLASSCODE_OFFSET + 1));
        if (SubClass == PCI_CLASS_BRIDGE_P2P) {
          SecondaryBusNumber = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, Bus, Device, Function, PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET));
          DEBUG ((DEBUG_INFO,"  ScanPciBus: PCI bridge S%04x B%02x D%02x F%02x (SecondBus:%02x)\n", Segment, Bus, Device, Function, SecondaryBusNumber));
          if (SecondaryBusNumber != 0) {
            Status = ScanPciBus (Context, Segment, SecondaryBusNumber, Callback);
            if (EFI_ERROR (Status)) {
              return Status;
            }
          }
        }
      }
    }
  }

  return EFI_SUCCESS;
}
