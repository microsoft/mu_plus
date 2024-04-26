/** @file

  Library to optionally assist populating the MFCI device targeting variables

  Refer to MfciPkg/Include/MfciVariables.h, "Targeting Variable Names" for additional details

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MFCI_DEVICE_ID_SUPPORT_LIB_H_
#define MFCI_DEVICE_ID_SUPPORT_LIB_H_

#include <MfciVariables.h>

/**
  Function pointer definition to get a device-unique targeting value.
  It is the callers responsibility to free the String buffer returned using FreePool().

  @param[out] String      Device targeting value, a UTF-16 little endian string.
                          Includes a wide NULL terminator.
                          Refer to MfciPkg/Include/MfciVariables.h for more details.
  @param[out] StringSize  (OPTIONAL) String size in bytes including the wide NULL terminator.
                          NULL may be supplied if the size is not requested (it is NULL terminated after all)

  @retval EFI_UNSUPPORTED       Likely using the NULL library instance.
  @retval EFI_SUCCESS           Successfully retrieved the string and length.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory to allocate the string.
  @retval EFI_INVALID_PARAMETER The String is NULL.
**/
typedef
EFI_STATUS
(EFIAPI *MFCI_DEVICE_ID_FN)(
  OUT CHAR16 **String,
  OUT UINTN *StringSize OPTIONAL
  );

/**
  Function that returns Manufacturer Name of the device and string size upon on return.
  It is the callers responsibility to free the buffer returned using FreePool().

  @param[out] Manufacturer      The Manufacturer string to be returned.
  @param[out] ManufacturerSize  The size of the Manufacturer string.

  @retval EFI_SUCCESS           The Manufacturer string was successfully returned.
  @retval EFI_UNSUPPORTED       The function is not supported.
  @retval EFI_INVALID_PARAMETER The Manufacturer is NULL.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory to allocate the Manufacturer string.
**/
EFI_STATUS
EFIAPI
MfciIdSupportGetManufacturer (
  OUT CHAR16  **Manufacturer,
  OUT UINTN   *ManufacturerSize  OPTIONAL
  );

/**
  Function that returns the Product Name string and size upon on return.
  It is the callers responsibility to free the buffer returned using FreePool().

  @param[out] ProductName     The ProductName string to be returned.
  @param[out] ProductNameSize The size of the ProductName string.

  @retval EFI_SUCCESS           The ProductName string was successfully returned.
  @retval EFI_UNSUPPORTED       The function is not supported.
  @retval EFI_INVALID_PARAMETER The ProductName is NULL.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory to allocate the ProductName string.
**/
EFI_STATUS
EFIAPI
MfciIdSupportGetProductName (
  OUT CHAR16  **ProductName,
  OUT UINTN   *ProductNameSize  OPTIONAL
  );

/**
  Function that returns the SerialNumber string and size upon on return.
  It is the callers responsibility to free the buffer returned using FreePool().

  @param[out] SerialNumber
  @param[out] SerialNumberSize

  @retval EFI_SUCCESS           The ProductName string was successfully returned.
  @retval EFI_UNSUPPORTED       The function is not supported.
  @retval EFI_INVALID_PARAMETER The ProductName is NULL.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory to allocate the ProductName string.
**/
EFI_STATUS
EFIAPI
MfciIdSupportGetSerialNumber (
  OUT CHAR16  **SerialNumber,
  OUT UINTN   *SerialNumberSize  OPTIONAL
  );

/**
  Function that returns the Oem1 string and size upon on return.
  It is the callers responsibility to free the buffer returned using FreePool().

  @param[out] Oem1      The OEM1 string to be returned.
  @param[out] Oem1Size  The size of the OEM1 string.

  @retval EFI_SUCCESS           The OEM1 string was successfully returned.
  @retval EFI_UNSUPPORTED       The function is not supported.
  @retval EFI_INVALID_PARAMETER The Oem1 is NULL.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory to allocate the OEM1 string.
**/
EFI_STATUS
EFIAPI
MfciIdSupportGetOem1 (
  OUT CHAR16  **Oem1,
  OUT UINTN   *Oem1Size  OPTIONAL
  );

/**
  Function that returns the Oem2 string and size upon on return.
  It is the callers responsibility to free the buffer returned using FreePool().

  @param[out] Oem2      The OEM2 string to be returned.
  @param[out] Oem2Size  The size of the OEM2 string.

  @retval EFI_SUCCESS           The OEM2 string was successfully returned.
  @retval EFI_UNSUPPORTED       The function is not supported.
  @retval EFI_INVALID_PARAMETER The Oem2 is NULL.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory to allocate the OEM2 string.
**/
EFI_STATUS
EFIAPI
MfciIdSupportGetOem2 (
  OUT CHAR16  **Oem2,
  OUT UINTN   *Oem2Size  OPTIONAL
  );

//
// The following helps iterate over the functions and set the corresponding target variable names.
//

// define a structure that pairs up the function pointer with the UEFI variable name
typedef struct {
  MFCI_DEVICE_ID_FN    DeviceIdFn;
  CHAR16               *DeviceIdVarName;
} MFCI_DEVICE_ID_FN_TO_VAR_NAME_MAP;

extern CONST MFCI_DEVICE_ID_FN_TO_VAR_NAME_MAP  gDeviceIdFnToTargetVarNameMap[];

#endif //MFCI_DEVICE_ID_SUPPORT_LIB_H_
