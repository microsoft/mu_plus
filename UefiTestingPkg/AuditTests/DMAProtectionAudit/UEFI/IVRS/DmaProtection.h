/** @file DmaProtection.h

Modified from Tianocore
https://github.com/tianocore/edk2-platforms/tree/master/Silicon/Intel/IntelSiliconPkg/Feature/VTd/IntelVTdDxe
for purpose of easy IVRS/BME parsing

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DMA_PROTECTION_H_
#define _DMA_PROTECTION_H_

#include <uefi.h>

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

//Linked List node used for IVMD Test
typedef struct IVMDListNode_t_def
{
  IVMD_Header*                  IVMD;
  struct IVMDListNode_t_def*    Next;
} IVMDListNode;

extern EFI_ACPI_IVRS_HEADER             *mAcpiIVRSTable;
extern UINTN                            mIvhdUnitNumber;
extern IVHD_Header                      *mIvhdUnitInformation;

/**
  Get the IVRS ACPI table.

  @retval EFI_SUCCESS           The IVRS ACPI table is got.
  @retval EFI_ALREADY_STARTED   The IVRS ACPI table has been got previously.
  @retval EFI_NOT_FOUND         The IVRS ACPI table is not found.
**/
EFI_STATUS
GetIvrsAcpiTable (
  VOID
  );

/**
  Parse IVRS IVHD table.

  @return EFI_SUCCESS  The IVRS IVHD table is parsed.
**/
EFI_STATUS
ParseIvrsAcpiTableIvhd (
  VOID
  );

/**
  Parse IVRS table.

  @return Linked List to all IVMD headers. Caller is required to check for NULL if no IVMDs found.
**/
IVMDListNode*
GetIvrsAcpiTableIvmd (
  VOID
  );

/**
  Get IVHD Entry number.
**/
UINTN
GetIvhdEntryNumber (
  VOID
  );

#endif // _DMA_PROTECTION_H_
