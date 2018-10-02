/** @file
DfciDeviceidSupportLib.c

This library provides access to platform data the becomes the DFCI DeviceId.

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

#include <PiDxe.h>
#include <Library/DfciDeviceIdSupportLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
Gets the serial number for this device.

@para[out] SerialNumber - UINTN value of serial number

@return EFI_SUCCESS - SerialNumber has been updated to equal the serial number of the device
@return EFI_ERROR   - Error getting number

**/
EFI_STATUS
EFIAPI
DfciIdSupportV1GetSerialNumber(
  OUT UINTN*  SerialNumber
  ) {

   return EFI_UNSUPPORTED;
}

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
    UINTN    *ManufacturerSize   OPTIONAL
  ) {

    return EFI_UNSUPPORTED;
}

/**
 * Get the ProductName
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
  ) {

    return EFI_UNSUPPORTED;
}

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
  ) {

    return EFI_UNSUPPORTED;
}

/**
 * Get the Uuid
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
  ) {

    return EFI_UNSUPPORTED;
}
