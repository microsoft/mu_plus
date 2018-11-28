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

#ifndef _DMAR_PROTECTION_H_
#define _DMAR_PROTECTION_H_

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciSegmentLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>

#include <Guid/Acpi.h>

#include <Protocol/PciIo.h>
#include <Protocol/PciEnumerationComplete.h>
#include <Protocol/PlatformVtdPolicy.h>
#include <Protocol/IoMmu.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/DmaRemappingReportingTable.h>
#include <IndustryStandard/Vtd.h>

//
// This is the initial max PCI DATA number.
// The number may be enlarged later.
//
#define MAX_VTD_PCI_DATA_NUMBER             0x100

typedef struct {
  UINT8                            DeviceType;
  VTD_SOURCE_ID                    PciSourceId;
  EDKII_PLATFORM_VTD_PCI_DEVICE_ID PciDeviceId;
  // for statistic analysis
  UINTN                            AccessCount;
} PCI_DEVICE_DATA;

typedef struct {
  BOOLEAN                          IncludeAllFlag;
  UINTN                            PciDeviceDataNumber;
  UINTN                            PciDeviceDataMaxNumber;
  PCI_DEVICE_DATA                  *PciDeviceData;
} PCI_DEVICE_INFORMATION;

typedef struct {
  UINTN                            VtdUnitBaseAddress;
  UINT16                           Segment;
  VTD_CAP_REG                      CapReg;
  VTD_ECAP_REG                     ECapReg;
  VTD_ROOT_ENTRY                   *RootEntryTable;
  VTD_EXT_ROOT_ENTRY               *ExtRootEntryTable;
  VTD_SECOND_LEVEL_PAGING_ENTRY    *FixedSecondLevelPagingEntry;
  BOOLEAN                          HasDirtyContext;
  BOOLEAN                          HasDirtyPages;
  PCI_DEVICE_INFORMATION           PciDeviceInfo;
} VTD_UNIT_INFORMATION;

//Linked List node used for RMRR Test
struct RMRRListNode
{
  EFI_ACPI_DMAR_RMRR_HEADER*    RMRR;
  struct RMRRListNode*          Next;
};

/**
  The scan bus callback function.

  It is called in PCI bus scan for each PCI device under the bus.

  @param[in]  Context               The context of the callback.
  @param[in]  Segment               The segment of the source.
  @param[in]  Bus                   The bus of the source.
  @param[in]  Device                The device of the source.
  @param[in]  Function              The function of the source.

  @retval EFI_SUCCESS           The specific PCI device is processed in the callback.
**/
typedef
EFI_STATUS
(EFIAPI *SCAN_BUS_FUNC_CALLBACK_FUNC) (
  IN VOID           *Context,
  IN UINT16         Segment,
  IN UINT8          Bus,
  IN UINT8          Device,
  IN UINT8          Function
  );

extern EFI_ACPI_DMAR_HEADER  *mAcpiDmarTable;

extern UINTN                            mVtdUnitNumber;
extern VTD_UNIT_INFORMATION             *mVtdUnitInformation;

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
  );

/**
  The scan bus callback function to always enable page attribute.

  @param[in]  Context               The context of the callback.
  @param[in]  Segment               The segment of the source.
  @param[in]  Bus                   The bus of the source.
  @param[in]  Device                The device of the source.
  @param[in]  Function              The function of the source.

  @retval EFI_SUCCESS           The VTd entry is updated to always enable all DMA access for the specific device.
**/
EFI_STATUS
EFIAPI
ScanBusCallbackRegisterPciDevice (
  IN VOID           *Context,
  IN UINT16         Segment,
  IN UINT8          Bus,
  IN UINT8          Device,
  IN UINT8          Function
  );

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
  );

/**
  Get the DMAR ACPI table.

  @retval EFI_SUCCESS           The DMAR ACPI table is got.
  @retval EFI_ALREADY_STARTED   The DMAR ACPI table has been got previously.
  @retval EFI_NOT_FOUND         The DMAR ACPI table is not found.
**/
EFI_STATUS
GetDmarAcpiTable (
  VOID
  );

/**
  Parse DMAR DRHD table.

  @return EFI_SUCCESS  The DMAR DRHD table is parsed.
**/
EFI_STATUS
ParseDmarAcpiTableDrhd (
  VOID
  );

/**
  Parse DMAR table.

  @return Linked List to all RMRR headers. Caller is required to check for NULL if no RMRRs found.
**/
struct RMRRListNode*
GetDmarAcpiTableRmrr (
  VOID
  );

/**
  Get VTd engine number.
**/
UINTN
GetVtdEngineNumber (
  VOID
  );

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
  );

#endif