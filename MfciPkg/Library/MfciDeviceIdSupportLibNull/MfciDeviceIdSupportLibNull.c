/** @file
  This library provides access to platform data that becomes the MFCI DeviceId.

  See MfciDeviceIdSupportLib.h for helpful documentation not duplicated here

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/MfciDeviceIdSupportLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
 * Get the Manufacturer Name
 *
 * @param[out] Manufacturer
 * @param[out] ManufacturerSize
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS
 *
 * @retval EFI_UNSUPPORTED         Likely using the NULL library instance
 * @retval EFI_SUCCESS             Successfully retrieved the string and length
 */
EFI_STATUS
EFIAPI
MfciIdSupportGetManufacturer (
  OUT CHAR16  **Manufacturer,
  OUT UINTN   *ManufacturerSize   OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

/**
 * Get the ProductName
 *
 * @param[out] ProductName
 * @param[out] ProductNameSize
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
MfciIdSupportGetProductName (
  OUT CHAR16  **ProductName,
  OUT UINTN   *ProductNameSize  OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

/**
 * Get the SerialNumber
 *
 * @param[out] SerialNumber
 * @param[out] SerialNumberSize
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
MfciIdSupportGetSerialNumber (
  OUT CHAR16  **SerialNumber,
  OUT UINTN   *SerialNumberSize  OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

/**
 * Get OEM1
 *
 * @param[out] Oem1
 * @param[out] Oem1Size
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
MfciIdSupportGetOem1 (
  OUT CHAR16  **Oem1,
  OUT UINTN   *Oem1Size  OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

/**
 * Get OEM2
 *
 * @param[out] Oem2
 * @param[out] Oem2Size
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
MfciIdSupportGetOem2 (
  OUT CHAR16  **Oem2,
  OUT UINTN   *Oem2Size  OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}
