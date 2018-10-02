/** @file
DfciDeviceIdSupportLib.h

Library supports getting the device Id elements.

Copyright (c) 2018, Microsoft Corporation

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

#ifndef __DFCI_DEVICE_ID_SUPPORT_LIB_H__
#define __DFCI_DEVICE_ID_SUPPORT_LIB_H__

// *-------------------------------------------------------------------*
//  TO DO - Remove this fucntion when V2 packet support is complete    *
// *-------------------------------------------------------------------*
/**
Gets the serial number for this device.

@para[out] SerialNumber - UINTN value of serial number

@return EFI_SUCCESS - SerialNumber has been updated to equal the serial number of the device
@return EFI_ERROR   - Error getting number

**/
EFI_STATUS
EFIAPI
DfciIdSupportV1GetSerialNumber (
  OUT UINTN*  SerialNumber
  );

/**
 * Get the Manufacturer Name
 *
 * @param Manufacturer
 * @param ManufacturerSize
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DfciIdSupportGetManufacturer (
    CHAR8   **Manufacturer,
    UINTN    *ManufacturerSize  OPTIONAL
  );

/**
 *
 * Get the Product Name
 *
 * @param ProductName
 * @param ProductNameSize
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DfciIdSupportGetProductName (
    CHAR8   **ProductName,
    UINTN    *ProductNameSize  OPTIONAL
  );

/**
 * Get the SerialNumber
 *
 * @param SerialNumber
 * @param SerialNumberSize
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DfciIdSupportGetSerialNumber (
    CHAR8   **SerialNumber,
    UINTN    *SerialNumberSize  OPTIONAL
  );

/**
 * Get the Uuid
 *
 * The format of the output Unicode string String consists of 36 characters, as follows:
 *
 *                aabbccdd-eeff-gghh-iijj-kkllmmnnoopp
 *
 * The pairs aa - pp are two characters in the range [0-9], [a-f] and
 * [A-F], with each pair representing a single byte hexadecimal value.
 *
 * The mapping between String and the EFI_GUID structure is as follows:
 *                aa          Data1[24:31]
 *                bb          Data1[16:23]
 *                cc          Data1[8:15]
 *                dd          Data1[0:7]
 *                ee          Data2[8:15]
 *                ff          Data2[0:7]
 *                gg          Data3[8:15]
 *                hh          Data3[0:7]
 *                ii          Data4[0:7]
 *                jj          Data4[8:15]
 *                kk          Data4[16:23]
 *                ll          Data4[24:31]
 *                mm          Data4[32:39]
 *                nn          Data4[40:47]
 *                oo          Data4[48:55]
 *                pp          Data4[56:63]
 *
 * @param Uuid
 * @param UuidSize
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DfciIdSupportGetUuid (
    CHAR8   **Uuid,
    UINTN    *UuidSize  OPTIONAL
  );

#endif //__DFCI_DEVICE_ID_SUPPORT_LIB_H__
