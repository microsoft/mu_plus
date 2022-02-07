/** @file Acpi.h

Modified from Tianocore
https://github.com/tianocore/edk2-platforms/tree/master/Silicon/Intel/IntelSiliconPkg/Feature/VTd/IntelVTdDxe
for purpose of easy ACPI table parsing

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ACPI_UNIT_TEST_H__
#define __ACPI_UNIT_TEST_H__

#pragma pack(1)

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT32                         Entry;
} RSDT_TABLE;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT64                         Entry;
} XSDT_TABLE;

#pragma pack()

/**
  Get the ACPI table.

  @retval EFI_SUCCESS           The ACPI table is got.
  @retval EFI_NOT_FOUND         The ACPI table is not found.
**/
EFI_STATUS
GetAcpiTable (
  IN  UINT32  AcpiSignature,
  OUT VOID    **AcpiTable
  );

#endif // __ACPI_UNIT_TEST_H__
