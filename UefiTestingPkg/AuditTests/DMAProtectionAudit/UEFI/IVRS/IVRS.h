/** @file IVRS.h

IVRS table constants and structures based on
AMD I/O Virtualization Technology (IOMMU) Specification 48882—Rev 3.00

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _IVRS_TABLE_H_
#define _IVRS_TABLE_H_

#define IVHD_TYPE_10H  0x10
#define IVHD_TYPE_11H  0x11
#define IVHD_TYPE_40H  0x40

#define IVMD_TYPE_20H  0x20
#define IVMD_TYPE_21H  0x21
#define IVMD_TYPE_22H  0x22

#define IVRS_DTE_TYPE_00H  0x00                 // Reserved
#define IVRS_DTE_TYPE_01H  0x01                 // All
#define IVRS_DTE_TYPE_02H  0x02                 // Select
#define IVRS_DTE_TYPE_03H  0x03                 // Start of range
#define IVRS_DTE_TYPE_04H  0x04                 // End of range
#define IVRS_DTE_TYPE_42H  0x42                 // Alias select
#define IVRS_DTE_TYPE_43H  0x43                 // Alias start of range
#define IVRS_DTE_TYPE_46H  0x46                 // Extended select
#define IVRS_DTE_TYPE_47H  0x47                 // Extended start of range
#define IVRS_DTE_TYPE_48H  0x48                 // Special device
#define IVRS_DTE_TYPE_F0H  0xF0                 // ACPI namespace

#define IOMMU_CONTROL_REG  0x18                 // MMIO Offset 0x18

#define IVRS_HEADER_SIGNATURE  SIGNATURE_32('I', 'V', 'R', 'S')

#pragma pack(1)

typedef union {
  struct {
    UINT16    Function : 3;
    UINT16    Device   : 5;
    UINT16    Bus      : 8;
  } Bits;
  UINT16    Value;
} IOMMU_DEVICE_ID;

typedef PACKED struct {
  UINT8              DeviceType;
  IOMMU_DEVICE_ID    DeviceID;
  UINT8              DataSetting;
} IVHD_DEVICE_ENTRY_COMMON;

typedef PACKED struct {
  UINT8              Handle;
  IOMMU_DEVICE_ID    DeviceID;
  UINT8              Variety;
} IVHD_DEVICE_ENTRY_EX;

typedef PACKED struct {
  IVHD_DEVICE_ENTRY_COMMON    CommonHeader;
  CHAR8                       HardwareId[8];
  CHAR8                       CompatibleId[8];
  UINT8                       UniqueIdFormat;
  UINT8                       UniqueIdLength;
  // CHAR8                  UniqueId[]
} IVHD_DEVICE_ENTRY_F0H;

typedef PACKED struct {
  UINT8              Type;
  UINT8              Flags;
  UINT16             Length;
  IOMMU_DEVICE_ID    DeviceID;
  UINT16             AuxiliaryData;
  UINT64             Reserved;
  UINT64             IVMDStartAddress;
  UINT64             IVMDMemoryBlockLength;
} IVMD_Header;

typedef PACKED union {
  UINT32    Bitfield;
  PACKED struct {
    UINT32    EFRSup    : 1;     // bit 0: Extended Feature Support.
    UINT32    Reserved1 : 4;     // bit 4-1: Must be zero.
    UINT32    GVAsize   : 3;     // bit 7-5: Guest virtual address width.
    UINT32    PAsize    : 7;     // bit 14-8: This field defines the width of the System Physical Address.
    UINT32    VAsize    : 7;     // bit 21-15: This field defines the width of the System Physical Address.
    UINT32    HtAtsResv : 1;     // bit 22: ATS response address translation range reserved.
    UINT32    Reserved2 : 9;     // bit 31-23: Must be zero.
  } Bits;
} IVRS_IVINFO;

typedef PACKED struct {
  UINT8              Type;
  UINT8              Flags;
  UINT16             Length;
  IOMMU_DEVICE_ID    DeviceID;
  UINT16             CapabilityOffset;
  UINT64             IOMMUBaseAddress;
  UINT16             PCISegmentGroup;
  UINT16             IOMMUInfo;
  UINT32             IOMMUFeatureInfo;
} IVHD_Header;

typedef PACKED struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;

  /**
    I/O virtualization information common to all IOMMU units in a system.
  **/
  IVRS_IVINFO                    IVRSIVInfo;
  UINT8                          Reserved[8];
} EFI_ACPI_IVRS_HEADER;

#pragma pack()

#endif
