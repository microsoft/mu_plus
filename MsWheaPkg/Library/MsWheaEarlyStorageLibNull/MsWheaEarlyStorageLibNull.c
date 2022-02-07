/** @file -- MsWheaEarlyStorageLib.c

This header defines APIs to utilize special memory for MsWheaReport during
early stage.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/MsWheaEarlyStorageLib.h>

/**

This routine returns the maximum number of bytes that can be stored in the early storage area.

@retval Count    The maximum number of bytes that can be stored in the MS WHEA store.

**/
UINT8
EFIAPI
MsWheaEarlyStorageGetMaxSize (
  VOID
  )
{
  return (UINT8)(0);
}

/**

This routine reads the specified data region from the MS WHEA store.

@param[in]  Ptr                       The pointer to hold intended read data
@param[in]  Size                      The size of intended read data
@param[in]  Offset                    The offset of read data, ranging from 0 to PcdMsWheaReportEarlyStorageCapacity

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_INVALID_PARAMETER         Null pointer or zero or over length request detected
@retval EFI_UNSUPPORTED               The function is unimplemented

**/
EFI_STATUS
EFIAPI
MsWheaEarlyStorageRead (
  VOID   *Ptr,
  UINT8  Size,
  UINT8  Offset
  )
{
  return EFI_UNSUPPORTED;
}

/**

This routine writes the specified data region from the MS WHEA store.

@param[in]  Ptr                       The pointer to hold intended written data
@param[in]  Size                      The size of intended written data
@param[in]  Offset                    The offset of written data, ranging from 0 to PcdMsWheaReportEarlyStorageCapacity

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_INVALID_PARAMETER         Null pointer or zero or over length request detected
@retval EFI_UNSUPPORTED               The function is unimplemented

**/
EFI_STATUS
EFIAPI
MsWheaEarlyStorageWrite (
  VOID   *Ptr,
  UINT8  Size,
  UINT8  Offset
  )
{
  return EFI_UNSUPPORTED;
}

/**

This routine clears the specified data region from the MS WHEA store to PcdMsWheaEarlyStorageDefaultValue.

@param[in]  Size                      The size of intended clear data
@param[in]  Offset                    The offset of clear data, ranging from 0 to PcdMsWheaReportEarlyStorageCapacity

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_INVALID_PARAMETER         Null pointer or zero or over length request detected
@retval EFI_UNSUPPORTED               The function is unimplemented

**/
EFI_STATUS
EFIAPI
MsWheaEarlyStorageClear (
  UINT8  Size,
  UINT8  Offset
  )
{
  return EFI_UNSUPPORTED;
}

/**

This routine checks the checksum of early storage region: starting from the signature of header to
the last byte of active range (excluding checksum field).

@param[in]  Header                    Whea Early Store Header
@param[out] Checksum                  Checksum of the Whea Early Store

@retval EFI_SUCCESS                   Checksum is now valid
@retval EFI_INVALID_PARAMETER         Checksum or Header were NULL
@retval EFI_BAD_BUFFER_SIZE           Header active range was too large
@retval EFI_UNSUPPORTED               The function is unimplemented

**/
EFI_STATUS
EFIAPI
MsWheaESCalculateChecksum16 (
  MS_WHEA_EARLY_STORAGE_HEADER  *Header,
  UINT16                        *Checksum
  )
{
  return EFI_UNSUPPORTED;
}

/**

This is a helper function that returns the maximal capacity for header excluded data.

@retval Count    The maximum number of bytes that can be stored in the MS WHEA store.

**/
UINT8
EFIAPI
MsWheaESGetMaxDataCount (
  VOID
  )
{
  return (UINT8)(0);
}

/**

This routine finds a contiguous memory that has default value of specified size in data region
from the MS WHEA store.

@param[in]  Size                      The size of intended clear data
@param[out] Offset                    The pointer to receive returned offset value, starting from
                                      Early MS_WHEA_EARLY_STORAGE_DATA_OFFSET

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_OUT_OF_RESOURCES          Null pointer or zero or over length request detected
@retval EFI_UNSUPPORTED               The function is unimplemented

**/
EFI_STATUS
EFIAPI
MsWheaESFindSlot (
  IN UINT8  Size,
  IN UINT8  *Offset
  )
{
  return EFI_UNSUPPORTED;
}

/**
This routine adds an MS_WHEA_EARLY_STORAGE_ENTRY_V0 record to the WHEA early store region using the supplied
metadata. The header checksum and active range will be updated in the process.

@param[in]  UINT32    ErrorStatusValue
@param[in]  UINT64    AdditionalInfo1
@param[in]  UINT64    AdditionalInfo2
@param[in]  EFI_GUID  *ModuleId
@param[in]  EFI_GUID  *PartitionId

@retval     EFI_SUCCESS             The record was added
@retval     EFI_OUT_OF_RESOURCES    The CMOS ES region is full
@retval     EFI_UNSUPPORTED         The function is unimplemented

**/
EFI_STATUS
EFIAPI
MsWheaESAddRecordV0 (
  IN  UINT32    ErrorStatusValue,
  IN  UINT64    AdditionalInfo1,
  IN  UINT64    AdditionalInfo2,
  IN  EFI_GUID  *ModuleId OPTIONAL,
  IN  EFI_GUID  *PartitionId OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}
