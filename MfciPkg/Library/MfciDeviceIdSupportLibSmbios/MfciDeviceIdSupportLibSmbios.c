/** @file

  This library reads SMBIOS values to populate the MFCI Targeting UEFI Variables
  This is _a_ method of populating these variables

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/MfciDeviceIdSupportLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Uefi/UefiInternalFormRepresentation.h>
#include <Protocol/Smbios.h>

#define ID_NOT_FOUND  "Not Found"

// populate the array of structures that pair up the functions with variable names
CONST MFCI_DEVICE_ID_FN_TO_VAR_NAME_MAP  gDeviceIdFnToTargetVarNameMap[] = {
  { MfciIdSupportGetManufacturer, MFCI_MANUFACTURER_VARIABLE_NAME },
  { MfciIdSupportGetProductName,  MFCI_PRODUCT_VARIABLE_NAME      },
  { MfciIdSupportGetSerialNumber, MFCI_SERIALNUMBER_VARIABLE_NAME },
  { MfciIdSupportGetOem1,         MFCI_OEM_01_VARIABLE_NAME       },
  { MfciIdSupportGetOem2,         MFCI_OEM_02_VARIABLE_NAME       }
};

// Note: This protocol will guarantee to be met by the Depex and located at the
// constructor of this library, thus no null-pointer check in library code flow.
EFI_SMBIOS_PROTOCOL  *mSmbiosProtocol;

/**

  Acquire the string associated with the Index from smbios structure and return it.
  The caller is responsible for freeing the string buffer.

  @param    OptionalStrStart  The start position to search the string
  @param    Index             The index of the string to extract
  @param    String            The string that is extracted or ID_NOT_FOUND
  @param    Size              Optional pointer to hold size of returned string (bytes including the terminating CHAR16 NULL)

  @retval   EFI_STATUS

**/
EFI_STATUS
GetOptionalStringByIndex (
  IN      CHAR8   *OptionalStrStart,
  IN      UINT8   Index,
  OUT     CHAR16  **String,
  OUT     UINTN   *Size   OPTIONAL
  )
{
  UINTN  StrSize;
  CHAR8  *WhichStr;

  StrSize = 0;
  if (Index != 0) {
    do {
      Index--;
      OptionalStrStart += StrSize;
      StrSize           = AsciiStrSize (OptionalStrStart);
    } while (OptionalStrStart[StrSize] != 0 && Index != 0);
  }

  if ((Index != 0) || (StrSize == 1) || (StrSize == 0)) {
    //
    // Meet the end of strings set but Index is non-zero, or
    // found an empty string, or Index passed in was 0
    //
    DEBUG ((DEBUG_ERROR, "SMBIOS string not found, returning \"%s\"\n", ID_NOT_FOUND));
    StrSize  = sizeof (ID_NOT_FOUND);
    WhichStr = ID_NOT_FOUND;
  } else {
    WhichStr = OptionalStrStart;
  }

  *String = AllocatePool (StrSize * sizeof (CHAR16));  // 0 page will catch AllocatePool failures
  if (*String == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiStrToUnicodeStrS (WhichStr, *String, StrSize);

  if (Size != NULL) {
    *Size = StrSize * sizeof (CHAR16);
  }

  return EFI_SUCCESS;
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
MfciIdSupportGetManufacturer (
  CHAR16  **Manufacturer,
  UINTN   *ManufacturerSize   OPTIONAL
  )
{
  EFI_STATUS               Status;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER  *Record;
  SMBIOS_TYPE              Type;
  SMBIOS_TABLE_TYPE1       *Type1Record;

  if (Manufacturer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;      // Reset handle
  Type         = SMBIOS_TYPE_SYSTEM_INFORMATION; // Smbios type1
  Status       = mSmbiosProtocol->GetNext (mSmbiosProtocol, &SmbiosHandle, &Type, &Record, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Type1Record = (SMBIOS_TABLE_TYPE1 *)Record;
  Status      = GetOptionalStringByIndex ((CHAR8 *)((UINT8 *)Type1Record + Type1Record->Hdr.Length), Type1Record->Manufacturer, Manufacturer, ManufacturerSize);
  return Status;
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
MfciIdSupportGetProductName (
  CHAR16  **ProductName,
  UINTN   *ProductNameSize  OPTIONAL
  )
{
  EFI_STATUS               Status;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER  *Record;
  SMBIOS_TYPE              Type;
  SMBIOS_TABLE_TYPE1       *Type1Record;

  if (ProductName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;      // Reset handle
  Type         = SMBIOS_TYPE_SYSTEM_INFORMATION; // Smbios type1
  Status       = mSmbiosProtocol->GetNext (mSmbiosProtocol, &SmbiosHandle, &Type, &Record, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Type1Record = (SMBIOS_TABLE_TYPE1 *)Record;
  Status      = GetOptionalStringByIndex ((CHAR8 *)((UINT8 *)Type1Record + Type1Record->Hdr.Length), Type1Record->ProductName, ProductName, ProductNameSize);
  return Status;
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
MfciIdSupportGetSerialNumber (
  CHAR16  **SerialNumber,
  UINTN   *SerialNumberSize  OPTIONAL
  )
{
  EFI_STATUS               Status;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER  *Record;
  SMBIOS_TYPE              Type;
  SMBIOS_TABLE_TYPE1       *Type1Record;

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;      // Reset handle
  Type         = SMBIOS_TYPE_SYSTEM_INFORMATION; // Smbios type1
  Status       = mSmbiosProtocol->GetNext (mSmbiosProtocol, &SmbiosHandle, &Type, &Record, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Type1Record = (SMBIOS_TABLE_TYPE1 *)Record;
  Status      = GetOptionalStringByIndex ((CHAR8 *)((UINT8 *)Type1Record + Type1Record->Hdr.Length), Type1Record->SerialNumber, SerialNumber, SerialNumberSize);
  return Status;
}

EFI_STATUS
ReturnEmptyChar16 (
  CHAR16  **String,
  UINTN   *StringSize  OPTIONAL
  )
{
  CHAR16  *EmptyString;

  EmptyString = L"";
  *String     = AllocatePool (sizeof (*EmptyString));
  if (*String == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (*String, EmptyString, sizeof (*EmptyString));

  if (StringSize != NULL) {
    *StringSize = sizeof (*EmptyString);
  }

  return EFI_SUCCESS;
}

/**
 * Get OEM1
 *
 * @param Oem1
 * @param Oem1Size
 *
 * Get OEM1, an empty string in this SMBIOS example
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
MfciIdSupportGetOem1 (
  CHAR16  **Oem1,
  UINTN   *Oem1Size  OPTIONAL
  )
{
  return ReturnEmptyChar16 (Oem1, Oem1Size);
}

/**
 * Get OEM2, an empty string in this SMBIOS example
 *
 * @param Oem2
 * @param Oem2Size
 *
 * It is the callers responsibility to free the buffer returned.
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
MfciIdSupportGetOem2 (
  CHAR16  **Oem2,
  UINTN   *Oem2Size  OPTIONAL
  )
{
  return ReturnEmptyChar16 (Oem2, Oem2Size);
}

/**
  Constructor for MfciIdSupportLib.

  @param  ImageHandle   ImageHandle of the loaded driver.
  @param  SystemTable   Pointer to the EFI System Table.

  @retval EFI_SUCCESS   The handlers were registered successfully.
**/
EFI_STATUS
EFIAPI
MfciIdSupportConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&mSmbiosProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Could not locate SMBIOS protocol.  %r\n", Status));
  }

  return Status;
}
