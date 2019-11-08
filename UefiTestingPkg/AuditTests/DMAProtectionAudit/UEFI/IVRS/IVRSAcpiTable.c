/** @file IVRSAcpiTable.c

Modified from Tianocore
https://github.com/tianocore/edk2-platforms/tree/master/Silicon/Intel/IntelSiliconPkg/Feature/VTd/IntelVTdDxe
for purpose of easy IVRS/BME parsing

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "../Acpi.h"
#include "IVRS.h"
#include "DmaProtection.h"

EFI_ACPI_IVRS_HEADER             *mAcpiIVRSTable = NULL;
IVHD_Header                      *mIvhdUnitInformation;
UINTN                            mIvhdUnitNumber;

/**
  Process IVRS IVHD entry.

  @param[in]  IommuIndex  The index of IOMMU engine.
  @param[in]  IvrsIvhd    The IVHD entry.

  @retval EFI_SUCCESS The IVHD entry is processed.
**/
EFI_STATUS
ProcessIvhd (
  IN UINTN                      IommuIndex,
  IN IVHD_Header                *IvrsIvhd
  )
{
  IVHD_DEVICE_ENTRY_COMMON      *IvrsDeviceTableEntry;
  IVHD_DEVICE_ENTRY_COMMON      *IvrsDeviceTableNextEntry;

  CopyMem (&mIvhdUnitInformation[IommuIndex], IvrsIvhd, sizeof(IVHD_Header));
  DEBUG ((DEBUG_INFO,"  IVHD (%d) Type - 0x%02X, BaseAddress -  0x%016lx:\n", IommuIndex, IvrsIvhd->Type, IvrsIvhd->IOMMUBaseAddress));

  mIvhdUnitInformation[IommuIndex].PCISegmentGroup = IvrsIvhd->PCISegmentGroup;

  if (IvrsIvhd->Type == IVHD_TYPE_10H) {
    IvrsDeviceTableEntry = (IVHD_DEVICE_ENTRY_COMMON *)((UINTN)(IvrsIvhd + 1));
  }
  else if ((IvrsIvhd->Type == IVHD_TYPE_11H) || (IvrsIvhd->Type == IVHD_TYPE_40H)) {
    IvrsDeviceTableEntry = (IVHD_DEVICE_ENTRY_COMMON *)((UINTN)IvrsIvhd + sizeof(IVHD_Header) + 16);
  }
  else {
    return EFI_UNSUPPORTED;
  }

  while ((UINTN)IvrsDeviceTableEntry < (UINTN)IvrsIvhd + IvrsIvhd->Length) {

    DEBUG ((DEBUG_INFO,"  ProcessDhrd: "));
    switch (IvrsDeviceTableEntry->DeviceType) {
    case IVRS_DTE_TYPE_00H:
      IvrsDeviceTableNextEntry = (IVHD_DEVICE_ENTRY_COMMON *)((UINTN)IvrsDeviceTableEntry + sizeof(IVHD_DEVICE_ENTRY_COMMON));
      break;
    case IVRS_DTE_TYPE_01H:
      IvrsDeviceTableNextEntry = (IVHD_DEVICE_ENTRY_COMMON *)((UINTN)IvrsDeviceTableEntry + sizeof(IVHD_DEVICE_ENTRY_COMMON));
      DEBUG ((DEBUG_INFO,"All devices"));
      break;
    case IVRS_DTE_TYPE_02H:
      IvrsDeviceTableNextEntry = (IVHD_DEVICE_ENTRY_COMMON *)((UINTN)IvrsDeviceTableEntry + sizeof(IVHD_DEVICE_ENTRY_COMMON));
      DEBUG ((DEBUG_INFO,"Select device"));
      break;
    case IVRS_DTE_TYPE_03H:
      IvrsDeviceTableNextEntry = (IVHD_DEVICE_ENTRY_COMMON *)((UINTN)IvrsDeviceTableEntry + sizeof(IVHD_DEVICE_ENTRY_COMMON));
      DEBUG ((DEBUG_INFO,"Start of range"));
      break;
    case IVRS_DTE_TYPE_04H:
      IvrsDeviceTableNextEntry = (IVHD_DEVICE_ENTRY_COMMON *)((UINTN)IvrsDeviceTableEntry + sizeof(IVHD_DEVICE_ENTRY_COMMON));
      DEBUG ((DEBUG_INFO,"End of range"));
      break;
    case IVRS_DTE_TYPE_42H:
      IvrsDeviceTableNextEntry = (IVHD_DEVICE_ENTRY_COMMON *)((UINTN)IvrsDeviceTableEntry + sizeof(IVHD_DEVICE_ENTRY_COMMON) + sizeof(IVHD_DEVICE_ENTRY_EX));
      DEBUG ((DEBUG_INFO,"Alias select"));
      break;
    case IVRS_DTE_TYPE_43H:
      IvrsDeviceTableNextEntry = (IVHD_DEVICE_ENTRY_COMMON *)((UINTN)IvrsDeviceTableEntry + sizeof(IVHD_DEVICE_ENTRY_COMMON) + sizeof(IVHD_DEVICE_ENTRY_EX));
      DEBUG ((DEBUG_INFO,"Alias start of range"));
      break;
    case IVRS_DTE_TYPE_46H:
      IvrsDeviceTableNextEntry = (IVHD_DEVICE_ENTRY_COMMON *)((UINTN)IvrsDeviceTableEntry + sizeof(IVHD_DEVICE_ENTRY_COMMON) + sizeof(IVHD_DEVICE_ENTRY_EX));
      DEBUG ((DEBUG_INFO,"Extended select"));
      break;
    case IVRS_DTE_TYPE_47H:
      IvrsDeviceTableNextEntry = (IVHD_DEVICE_ENTRY_COMMON *)((UINTN)IvrsDeviceTableEntry + sizeof(IVHD_DEVICE_ENTRY_COMMON) + sizeof(IVHD_DEVICE_ENTRY_EX));
      DEBUG ((DEBUG_INFO,"Extended start of range"));
      break;
    case IVRS_DTE_TYPE_48H:
      IvrsDeviceTableNextEntry = (IVHD_DEVICE_ENTRY_COMMON *)((UINTN)IvrsDeviceTableEntry + sizeof(IVHD_DEVICE_ENTRY_COMMON) + sizeof(IVHD_DEVICE_ENTRY_EX));
      DEBUG ((DEBUG_INFO,"Special device"));
      break;
    case IVRS_DTE_TYPE_F0H:
      IvrsDeviceTableNextEntry = (IVHD_DEVICE_ENTRY_COMMON*)((UINTN)IvrsDeviceTableEntry + sizeof(IVHD_DEVICE_ENTRY_F0H) + ((IVHD_DEVICE_ENTRY_F0H *)IvrsDeviceTableEntry)->UniqueIdLength);
      DEBUG ((DEBUG_INFO,"ACPI Hardware ID Device Entries"));
      break;
    default:
      ASSERT(FALSE);
      break;
    }
    DEBUG ((DEBUG_INFO," S%04x B%02x D%02x F%02x\n", IvrsIvhd->PCISegmentGroup, IvrsDeviceTableEntry->DeviceID.Bits.Bus, IvrsDeviceTableEntry->DeviceID.Bits.Device, IvrsDeviceTableEntry->DeviceID.Bits.Function));

    IvrsDeviceTableEntry = IvrsDeviceTableNextEntry;
  }

  return EFI_SUCCESS;
}

/**
  Parse IVRS table.

  @return Linked List to all IVMD headers. Caller is required to check for NULL if no RMRRs found.
**/
IVMDListNode*
GetIvrsAcpiTableIvmd (
  VOID
  )
{
  IVMD_Header                    *IvrsHeader;

  IVMDListNode*                  Head = NULL;
  IVMDListNode*                  Tail = Head;

  IvrsHeader = (IVMD_Header *)((UINTN)(mAcpiIVRSTable + 1));
  while ((UINTN)IvrsHeader < (UINTN)mAcpiIVRSTable + mAcpiIVRSTable->Header.Length) {
    switch (IvrsHeader->Type) {
    case IVMD_TYPE_20H:
    case IVMD_TYPE_21H:
    case IVMD_TYPE_22H:
      //If IVMD found add to end of linked list
      IVMDListNode* newNode = (IVMDListNode*)AllocateZeroPool(sizeof(IVMDListNode));
      newNode->IVMD = (IVMD_Header *)IvrsHeader;
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
    IvrsHeader = (IVMD_Header *)((UINTN)IvrsHeader + IvrsHeader->Length);
  }
  return Head;
}

/**
  Get Ivhd Entry number.
**/
UINTN
GetIvhdEntryNumber (
  VOID
  )
{
  IVHD_Header                    *IvhdHeader;
  UINTN                           IvhdIndex;

  IvhdIndex = 0;
  IvhdHeader = (IVHD_Header *)((UINTN)(mAcpiIVRSTable + 1));
  while ((UINTN)IvhdHeader < (UINTN)mAcpiIVRSTable + mAcpiIVRSTable->Header.Length) {
    switch (IvhdHeader->Type) {
    case IVHD_TYPE_10H:
    case IVHD_TYPE_11H:
    case IVHD_TYPE_40H:
      IvhdIndex++;
      break;
    default:
      break;
    }
    IvhdHeader = (IVHD_Header *)((UINTN)IvhdHeader + IvhdHeader->Length);
  }
  return IvhdIndex;
}

/**
  Parse IVRS IVHD table.

  @return EFI_SUCCESS  The IVRS IVHD table is parsed.
**/
EFI_STATUS
ParseIvrsAcpiTableIvhd (
  VOID
  )
{
  IVHD_Header                                       *IvrsHeader;
  EFI_STATUS                                        Status;
  UINTN                                             IommuIndex;

  mIvhdUnitNumber = GetIvhdEntryNumber ();
  DEBUG ((DEBUG_INFO,"  IvhdUnitNumber - %d\n", mIvhdUnitNumber));
  ASSERT (mIvhdUnitNumber > 0);
  if (mIvhdUnitNumber == 0) {
    return EFI_DEVICE_ERROR;
  }

  mIvhdUnitInformation = AllocateZeroPool (sizeof(*mIvhdUnitInformation) * mIvhdUnitNumber);
  ASSERT (mIvhdUnitInformation != NULL);
  if (mIvhdUnitInformation == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  IommuIndex = 0;
  IvrsHeader = (IVHD_Header *)((UINTN)(mAcpiIVRSTable + 1));
  while ((UINTN)IvrsHeader < (UINTN)mAcpiIVRSTable + mAcpiIVRSTable->Header.Length) {
    switch (IvrsHeader->Type) {
    case IVHD_TYPE_10H:
    case IVHD_TYPE_11H:
    case IVHD_TYPE_40H:
      ASSERT (IommuIndex < mIvhdUnitNumber);
      Status = ProcessIvhd (IommuIndex, (IVHD_Header *)IvrsHeader);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      IommuIndex++;

      break;

    default:
      break;
    }
    IvrsHeader = (IVHD_Header *)((UINTN)IvrsHeader + IvrsHeader->Length);
  }
  ASSERT (IommuIndex == mIvhdUnitNumber);

  return EFI_SUCCESS ;
}

/**
  Get the IVRS ACPI table.

  @retval EFI_SUCCESS           The IVRS ACPI table is got.
  @retval EFI_ALREADY_STARTED   The IVRS ACPI table has been got previously.
  @retval EFI_NOT_FOUND         The IVRS ACPI table is not found.
**/
EFI_STATUS
GetIvrsAcpiTable (
  VOID
  )
{
  return GetAcpiTable (IVRS_HEADER_SIGNATURE, &mAcpiIVRSTable);
}
