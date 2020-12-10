/** @file

  Library to optionally assist populating the MFCI device targeting variables

  Refer to MfciPkg/Include/MfciVariables.h, "Targeting Variable Names" for additional details

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MFCI_DEVICE_ID_SUPPORT_LIB_H__
#define __MFCI_DEVICE_ID_SUPPORT_LIB_H__

#include <MfciVariables.h>

// define a generic function prototype shared by all library functions

/**
 * Get a device-unique targeting value
 *
 * @param[out] String   Device targeting value, a UTF-16 little endian string.
 *                      Includes a wide NULL terminator.
 *                      Refer to MfciPkg/Include/MfciVariables.h for more details.
 *
 * @param[out] StringSize  (OPTIONAL) String size in bytes including the wide NULL terminator.
 *                         NULL may be supplied if the size is not requested (it is NULL terminated after all)
 *
 * It is the callers responsibility to free the String buffer returned using FreePool()
 *
 * @return EFI_STATUS
 *
 * @retval EFI_UNSUPPORTED         Likely using the NULL library instance
 * @retval EFI_SUCCESS             Successfully retrieved the string and length
 */

typedef
EFI_STATUS
(EFIAPI *MFCI_DEVICE_ID_FN) (
  OUT CHAR16 **String,
  OUT UINTN *StringSize
);

/**
 * Get the Manufacturer Name
 *
 * @param[out] Manufacturer
 * @param[out] ManufacturerSize
 *
 * It is the callers responsibility to free the buffer returned using FreePool()
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
MfciIdSupportGetManufacturer (
    OUT CHAR16  **Manufacturer,
    OUT UINTN    *ManufacturerSize  OPTIONAL
  );

/**
 *
 * Get the Product Name
 *
 * @param[out] ProductName
 * @param[out] ProductNameSize
 *
 * It is the callers responsibility to free the buffer returned using FreePool()
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
MfciIdSupportGetProductName (
    OUT CHAR16  **ProductName,
    OUT UINTN    *ProductNameSize  OPTIONAL
  );

/**
 * Get the SerialNumber
 *
 * @param[out] SerialNumber
 * @param[out] SerialNumberSize
 *
 * It is the callers responsibility to free the buffer returned using FreePool()
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
MfciIdSupportGetSerialNumber (
    OUT CHAR16  **SerialNumber,
    OUT UINTN    *SerialNumberSize  OPTIONAL
  );

/**
 * Get OEM1
 *
 * @param[out] Oem1
 * @param[out] Oem1Size
 *
 * It is the callers responsibility to free the buffer returned using FreePool()
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
MfciIdSupportGetOem1 (
    OUT CHAR16  **Oem1,
    OUT UINTN    *Oem1Size  OPTIONAL
  );

/**
 * Get OEM2
 *
 * @param[out] Oem2
 * @param[out] Oem2Size
 *
 * It is the callers responsibility to free the buffer returned using FreePool()
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
MfciIdSupportGetOem2 (
    OUT CHAR16  **Oem2,
    OUT UINTN    *Oem2Size  OPTIONAL
  );

/**
 * the following helps iterate over the functions and set the corresponding target variable names
 */

// define a structure that pairs up the function pointer with the UEFI variable name
typedef struct {
  MFCI_DEVICE_ID_FN DeviceIdFn;
  CHAR16 *DeviceIdVarName;
} MFCI_DEVICE_ID_FN_TO_VAR_NAME_MAP;

// populate the array of structures that pair up the functions with variable names
#define MFCI_TARGET_VAR_COUNT 5
STATIC CONST MFCI_DEVICE_ID_FN_TO_VAR_NAME_MAP gDeviceIdFnToTargetVarNameMap[MFCI_TARGET_VAR_COUNT] = {
  { MfciIdSupportGetManufacturer, MFCI_MANUFACTURER_VARIABLE_NAME },
  { MfciIdSupportGetProductName, MFCI_PRODUCT_VARIABLE_NAME },
  { MfciIdSupportGetSerialNumber, MFCI_SERIALNUMBER_VARIABLE_NAME },
  { MfciIdSupportGetOem1, MFCI_OEM_01_VARIABLE_NAME },
  { MfciIdSupportGetOem2, MFCI_OEM_02_VARIABLE_NAME }
};

#endif //__MFCI_DEVICE_ID_SUPPORT_LIB_H__
