/** @file AcpiRgrt.h
 *
 * This driver provides an entry in the ACPI table that provides a PNG graphic
  for regulatory purposes.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
///
/// "RGRT" Regulatory Graphic Resource Table
///
#define MSFT_ACPI_REGULATORY_GRAPHIC_RESOURCE_TABLE_SIGNATURE  SIGNATURE_32('R', 'G', 'R', 'T')

typedef struct _MSFT_RGRT_ACPI_TABLE {
  EFI_ACPI_DESCRIPTION_HEADER       Header;
  UINT16                            Version;
  UINT8                             ImageType;
  UINT8                             Reserved;
  UINT8                             Image[];
} MSFT_RGRT_ACPI_TABLE;

// Image Type (enumerated field)
// 0 = reserved, 1 = PNG, 2+ = reserved
#define MSFT_ACPI_REGULATORY_GRAPHIC_RESOURCE_TABLE_IMAGE_TYPE_PNG 0x01;
// Version (16 bits)- value must be 1

#define MSFT_ACPI_REGULATORY_GRAPHIC_RESOURCE_TABLE_IMAGE_VERSION 0x01;
// Revision (16 bits)- value must be 1
#define MSFT_ACPI_REGULATORY_GRAPHIC_RESOURCE_TABLE_IMAGE_REVISION 0x01;