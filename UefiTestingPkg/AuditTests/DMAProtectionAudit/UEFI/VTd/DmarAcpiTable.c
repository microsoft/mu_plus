/** @file

Copied from Tianocore IntelSiliconPkg\Feature\VTd\IntelVTdDxe for purpose of easy DMAR/BME parsing

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DmaProtection.h"

#pragma pack(1)

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER  Header;
  UINT32                       Entry;
} RSDT_TABLE;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER  Header;
  UINT64                       Entry;
} XSDT_TABLE;

#pragma pack()

EFI_ACPI_DMAR_HEADER             *mAcpiDmarTable = NULL;
VTD_UNIT_INFORMATION             *mVtdUnitInformation;
UINTN                            mVtdUnitNumber;

/**
  Get PCI device information from DMAR DevScopeEntry.

  @param[in]  Segment               The segment number.
  @param[in]  DmarDevScopeEntry     DMAR DevScopeEntry
  @param[out] Bus                   The bus number.
  @param[out] Device                The device number.
  @param[out] Function              The function number.

  @retval EFI_SUCCESS  The PCI device information is returned.
**/
EFI_STATUS
GetPciBusDeviceFunction (
  IN  UINT16                                      Segment,
  IN  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *DmarDevScopeEntry,
  OUT UINT8                                       *Bus,
  OUT UINT8                                       *Device,
  OUT UINT8                                       *Function
  )
{
  EFI_ACPI_DMAR_PCI_PATH                     *DmarPciPath;
  UINT8                                      MyBus;
  UINT8                                      MyDevice;
  UINT8                                      MyFunction;

  DmarPciPath = (EFI_ACPI_DMAR_PCI_PATH *)((UINTN)(DmarDevScopeEntry + 1));
  MyBus = DmarDevScopeEntry->StartBusNumber;
  MyDevice = DmarPciPath->Device;
  MyFunction = DmarPciPath->Function;

  switch (DmarDevScopeEntry->Type) {
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT:
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE:
    while ((UINTN)DmarPciPath + sizeof(EFI_ACPI_DMAR_PCI_PATH) < (UINTN)DmarDevScopeEntry + DmarDevScopeEntry->Length) {
      MyBus = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, MyBus, MyDevice, MyFunction, PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET));
      DmarPciPath ++;
      MyDevice = DmarPciPath->Device;
      MyFunction = DmarPciPath->Function;
    }
    break;
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_IOAPIC:
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_MSI_CAPABLE_HPET:
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_ACPI_NAMESPACE_DEVICE:
    break;
  }

  *Bus = MyBus;
  *Device = MyDevice;
  *Function = MyFunction;

  return EFI_SUCCESS;
}

/**
  Process DMAR DHRD table.

  @param[in]  VtdIndex  The index of VTd engine.
  @param[in]  DmarDrhd  The DRHD table.

  @retval EFI_SUCCESS The DRHD table is processed.
**/
EFI_STATUS
ProcessDhrd (
  IN UINTN                      VtdIndex,
  IN EFI_ACPI_DMAR_DRHD_HEADER  *DmarDrhd
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER       *DmarDevScopeEntry;
  UINT8                                             Bus;
  UINT8                                             Device;
  UINT8                                             Function;
  UINT8                                             SecondaryBusNumber;
  EFI_STATUS                                        Status;
  VTD_SOURCE_ID                                     SourceId;

  mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress = (UINTN)DmarDrhd->RegisterBaseAddress;
  DEBUG ((DEBUG_INFO,"  VTD (%d) BaseAddress -  0x%016lx\n", VtdIndex, DmarDrhd->RegisterBaseAddress));

  mVtdUnitInformation[VtdIndex].Segment = DmarDrhd->SegmentNumber;

  if ((DmarDrhd->Flags & EFI_ACPI_DMAR_DRHD_FLAGS_INCLUDE_PCI_ALL) != 0) {
    mVtdUnitInformation[VtdIndex].PciDeviceInfo.IncludeAllFlag = TRUE;
    DEBUG ((DEBUG_INFO,"  ProcessDhrd: with INCLUDE ALL\n"));

    Status = ScanPciBus((VOID *)VtdIndex, DmarDrhd->SegmentNumber, 0, ScanBusCallbackRegisterPciDevice);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    mVtdUnitInformation[VtdIndex].PciDeviceInfo.IncludeAllFlag = FALSE;
    DEBUG ((DEBUG_INFO,"  ProcessDhrd: without INCLUDE ALL\n"));
  }

  DmarDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)(DmarDrhd + 1));
  while ((UINTN)DmarDevScopeEntry < (UINTN)DmarDrhd + DmarDrhd->Header.Length) {

    Status = GetPciBusDeviceFunction (DmarDrhd->SegmentNumber, DmarDevScopeEntry, &Bus, &Device, &Function);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DEBUG ((DEBUG_INFO,"  ProcessDhrd: "));
    switch (DmarDevScopeEntry->Type) {
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT:
      DEBUG ((DEBUG_INFO,"PCI Endpoint"));
      break;
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE:
      DEBUG ((DEBUG_INFO,"PCI-PCI bridge"));
      break;
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_IOAPIC:
      DEBUG ((DEBUG_INFO,"IOAPIC"));
      break;
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_MSI_CAPABLE_HPET:
      DEBUG ((DEBUG_INFO,"MSI Capable HPET"));
      break;
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_ACPI_NAMESPACE_DEVICE:
      DEBUG ((DEBUG_INFO,"ACPI Namespace Device"));
      break;
    }
    DEBUG ((DEBUG_INFO," S%04x B%02x D%02x F%02x\n", DmarDrhd->SegmentNumber, Bus, Device, Function));

    SourceId.Bits.Bus = Bus;
    SourceId.Bits.Device = Device;
    SourceId.Bits.Function = Function;

    Status = RegisterPciDevice (VtdIndex, DmarDrhd->SegmentNumber, SourceId, DmarDevScopeEntry->Type, TRUE);
    if (EFI_ERROR (Status)) {
      //
      // There might be duplication for special device other than standard PCI device.
      //
      switch (DmarDevScopeEntry->Type) {
      case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT:
      case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE:
        return Status;
      }
    }

    switch (DmarDevScopeEntry->Type) {
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE:
      SecondaryBusNumber = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(DmarDrhd->SegmentNumber, Bus, Device, Function, PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET));
      Status = ScanPciBus ((VOID *)VtdIndex, DmarDrhd->SegmentNumber, SecondaryBusNumber, ScanBusCallbackRegisterPciDevice);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      break;
    default:
      break;
    }

    DmarDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDevScopeEntry + DmarDevScopeEntry->Length);
  }

  return EFI_SUCCESS;
}

/**
  Parse DMAR table.

  @return Linked List to all RMRR headers. Caller is required to check for NULL if no RMRRs found.
**/
struct RMRRListNode*
GetDmarAcpiTableRmrr (
  VOID
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;

  struct RMRRListNode*                              Head = NULL;
  struct RMRRListNode*                              Tail = Head;

  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(mAcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)mAcpiDmarTable + mAcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_RMRR:
      //If RMRR found add to end of linked list
      struct RMRRListNode* newNode = (struct RMRRListNode*)AllocateZeroPool(sizeof(struct RMRRListNode));
      newNode->RMRR = (EFI_ACPI_DMAR_RMRR_HEADER *)DmarHeader;
      newNode->Next = NULL;
      
      if (Head == NULL) {
        Head = newNode;
        Tail = newNode;
      }
      else {
        Tail->Next = newNode;
        Tail = newNode;
      }
      break;
    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
  return Head;
}

/**
  Get VTd engine number.
**/
UINTN
GetVtdEngineNumber (
  VOID
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;
  UINTN                                             VtdIndex;

  VtdIndex = 0;
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(mAcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)mAcpiDmarTable + mAcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_DRHD:
      VtdIndex++;
      break;
    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
  return VtdIndex ;
}

/**
  Parse DMAR DRHD table.

  @return EFI_SUCCESS  The DMAR DRHD table is parsed.
**/
EFI_STATUS
ParseDmarAcpiTableDrhd (
  VOID
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;
  EFI_STATUS                                        Status;
  UINTN                                             VtdIndex;

  mVtdUnitNumber = GetVtdEngineNumber ();
  DEBUG ((DEBUG_INFO,"  VtdUnitNumber - %d\n", mVtdUnitNumber));
  ASSERT (mVtdUnitNumber > 0);
  if (mVtdUnitNumber == 0) {
    return EFI_DEVICE_ERROR;
  }

  mVtdUnitInformation = AllocateZeroPool (sizeof(*mVtdUnitInformation) * mVtdUnitNumber);
  ASSERT (mVtdUnitInformation != NULL);
  if (mVtdUnitInformation == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  VtdIndex = 0;
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(mAcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)mAcpiDmarTable + mAcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_DRHD:
      ASSERT (VtdIndex < mVtdUnitNumber);
      Status = ProcessDhrd (VtdIndex, (EFI_ACPI_DMAR_DRHD_HEADER *)DmarHeader);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      VtdIndex++;

      break;

    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
  ASSERT (VtdIndex == mVtdUnitNumber);

  /*
  for (VtdIndex = 0; VtdIndex < mVtdUnitNumber; VtdIndex++) {
    DumpPciDeviceInfo (VtdIndex);
  }
  */
  return EFI_SUCCESS ;
}

/**
  This function scan ACPI table in RSDT.

  @param[in]  Rsdt      ACPI RSDT
  @param[in]  Signature ACPI table signature

  @return ACPI table
**/
VOID *
ScanTableInRSDT (
  IN RSDT_TABLE                   *Rsdt,
  IN UINT32                       Signature
  )
{
  UINTN                         Index;
  UINT32                        EntryCount;
  UINT32                        *EntryPtr;
  EFI_ACPI_DESCRIPTION_HEADER   *Table;

  EntryCount = (Rsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT32);

  EntryPtr = &Rsdt->Entry;
  for (Index = 0; Index < EntryCount; Index ++, EntryPtr ++) {
    Table = (EFI_ACPI_DESCRIPTION_HEADER*)((UINTN)(*EntryPtr));
    if ((Table != NULL) && (Table->Signature == Signature)) {
      return Table;
    }
  }

  return NULL;
}

/**
  This function scan ACPI table in XSDT.

  @param[in]  Xsdt      ACPI XSDT
  @param[in]  Signature ACPI table signature

  @return ACPI table
**/
VOID *
ScanTableInXSDT (
  IN XSDT_TABLE                   *Xsdt,
  IN UINT32                       Signature
  )
{
  UINTN                        Index;
  UINT32                       EntryCount;
  UINT64                       EntryPtr;
  UINTN                        BasePtr;
  EFI_ACPI_DESCRIPTION_HEADER  *Table;

  EntryCount = (Xsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT64);

  BasePtr = (UINTN)(&(Xsdt->Entry));
  for (Index = 0; Index < EntryCount; Index ++) {
    CopyMem (&EntryPtr, (VOID *)(BasePtr + Index * sizeof(UINT64)), sizeof(UINT64));
    Table = (EFI_ACPI_DESCRIPTION_HEADER*)((UINTN)(EntryPtr));
    if ((Table != NULL) && (Table->Signature == Signature)) {
      return Table;
    }
  }

  return NULL;
}

/**
  This function scan ACPI table in RSDP.

  @param[in]  Rsdp      ACPI RSDP
  @param[in]  Signature ACPI table signature

  @return ACPI table
**/
VOID *
FindAcpiPtr (
  IN EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp,
  IN UINT32                                       Signature
  )
{
  EFI_ACPI_DESCRIPTION_HEADER                    *AcpiTable;
  RSDT_TABLE                                     *Rsdt;
  XSDT_TABLE                                     *Xsdt;

  AcpiTable = NULL;

  //
  // Check ACPI2.0 table
  //
  Rsdt = (RSDT_TABLE *)(UINTN)Rsdp->RsdtAddress;
  Xsdt = NULL;
  if ((Rsdp->Revision >= 2) && (Rsdp->XsdtAddress < (UINT64)(UINTN)-1)) {
    Xsdt = (XSDT_TABLE *)(UINTN)Rsdp->XsdtAddress;
  }
  //
  // Check Xsdt
  //
  if (Xsdt != NULL) {
    AcpiTable = ScanTableInXSDT (Xsdt, Signature);
  }
  //
  // Check Rsdt
  //
  if ((AcpiTable == NULL) && (Rsdt != NULL)) {
    AcpiTable = ScanTableInRSDT (Rsdt, Signature);
  }

  return AcpiTable;
}

/**
  Get the DMAR ACPI table.

  @retval EFI_SUCCESS           The DMAR ACPI table is got.
  @retval EFI_ALREADY_STARTED   The DMAR ACPI table has been got previously.
  @retval EFI_NOT_FOUND         The DMAR ACPI table is not found.
**/
EFI_STATUS
GetDmarAcpiTable (
  VOID
  )
{
  VOID                              *AcpiTable;
  EFI_STATUS                        Status;

  if (mAcpiDmarTable != NULL) {
    return EFI_ALREADY_STARTED;
  }

  AcpiTable = NULL;
  Status = EfiGetSystemConfigurationTable (
             &gEfiAcpi20TableGuid,
             &AcpiTable
             );
  if (EFI_ERROR (Status)) {
    Status = EfiGetSystemConfigurationTable (
               &gEfiAcpi10TableGuid,
               &AcpiTable
               );
  }
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  ASSERT (AcpiTable != NULL);

  mAcpiDmarTable = FindAcpiPtr (
                      (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)AcpiTable,
                      EFI_ACPI_4_0_DMA_REMAPPING_TABLE_SIGNATURE
                      );
  if (mAcpiDmarTable == NULL) {
    return EFI_NOT_FOUND;
  }
  DEBUG ((DEBUG_INFO,"DMAR Table - 0x%08x\n", mAcpiDmarTable));

  return EFI_SUCCESS;
}
