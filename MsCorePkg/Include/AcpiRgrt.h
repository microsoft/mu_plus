/** @file AcpiRgrt.h
 *
 * This driver provides an entry in the ACPI table that provides a PNG graphic
  for regulatory purposes.

  Copyright (c), Microsoft Corporation

  All rights reserved.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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