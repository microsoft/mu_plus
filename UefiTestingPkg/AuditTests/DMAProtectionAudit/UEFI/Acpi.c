/** @file

Modified from Tianocore
https://github.com/tianocore/edk2-platforms/tree/master/Silicon/Intel/IntelSiliconPkg/Feature/VTd/IntelVTdDxe
for purpose of easy ACPI table parsing

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciSegmentLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>

#include <Guid/Acpi.h>

#include <Protocol/PciIo.h>
#include <Protocol/PciEnumerationComplete.h>
#include <Protocol/IoMmu.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/DmaRemappingReportingTable.h>

#include "Acpi.h"

/**
  This function scan ACPI table in RSDT.

  @param[in]  Rsdt      ACPI RSDT
  @param[in]  Signature ACPI table signature

  @return ACPI table
**/
VOID *
ScanTableInRSDT (
  IN RSDT_TABLE  *Rsdt,
  IN UINT32      Signature
  )
{
  UINTN                        Index;
  UINT32                       EntryCount;
  UINT32                       *EntryPtr;
  EFI_ACPI_DESCRIPTION_HEADER  *Table;

  EntryCount = (Rsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT32);

  EntryPtr = &Rsdt->Entry;
  for (Index = 0; Index < EntryCount; Index++, EntryPtr++) {
    Table = (EFI_ACPI_DESCRIPTION_HEADER *)((UINTN)(*EntryPtr));
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
  IN XSDT_TABLE  *Xsdt,
  IN UINT32      Signature
  )
{
  UINTN                        Index;
  UINT32                       EntryCount;
  UINT64                       EntryPtr;
  UINTN                        BasePtr;
  EFI_ACPI_DESCRIPTION_HEADER  *Table;

  EntryCount = (Xsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT64);

  BasePtr = (UINTN)(&(Xsdt->Entry));
  for (Index = 0; Index < EntryCount; Index++) {
    CopyMem (&EntryPtr, (VOID *)(BasePtr + Index * sizeof (UINT64)), sizeof (UINT64));
    Table = (EFI_ACPI_DESCRIPTION_HEADER *)((UINTN)(EntryPtr));
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
  IN EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp,
  IN UINT32                                        Signature
  )
{
  EFI_ACPI_DESCRIPTION_HEADER  *AcpiTable;
  RSDT_TABLE                   *Rsdt;
  XSDT_TABLE                   *Xsdt;

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
  Get the ACPI table.

  @retval EFI_SUCCESS           The ACPI table is got.
  @retval EFI_NOT_FOUND         The ACPI table is not found.
**/
EFI_STATUS
GetAcpiTable (
  IN  UINT32  AcpiSignature,
  OUT VOID    **AcpiTable
  )
{
  EFI_STATUS  Status;
  VOID        *AcpiConfigurationTable = NULL;

  if (AcpiTable == NULL) {
    return EFI_INVALID_PARAMETER;
  } else if (*AcpiTable != NULL) {
    return EFI_ALREADY_STARTED;
  }

  *AcpiTable = NULL;
  Status     = EfiGetSystemConfigurationTable (
                 &gEfiAcpi20TableGuid,
                 &AcpiConfigurationTable
                 );
  if (EFI_ERROR (Status)) {
    Status = EfiGetSystemConfigurationTable (
               &gEfiAcpi10TableGuid,
               &AcpiConfigurationTable
               );
  }

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  ASSERT (AcpiConfigurationTable != NULL);

  *AcpiTable = FindAcpiPtr (
                 (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)AcpiConfigurationTable,
                 AcpiSignature
                 );
  if (*AcpiTable == NULL) {
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "ACPI Table - 0x%08x\n", *AcpiTable));

  return EFI_SUCCESS;
}
