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

**/
EFI_STATUS
EFIAPI
MsWheaEarlyStorageRead (
  VOID                                *Ptr,
  UINT8                               Size,
  UINT8                               Offset
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

**/
EFI_STATUS
EFIAPI
MsWheaEarlyStorageWrite (
  VOID                                *Ptr,
  UINT8                               Size,
  UINT8                               Offset
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

**/
EFI_STATUS
EFIAPI
MsWheaEarlyStorageClear (
  UINT8                               Size,
  UINT8                               Offset
  )
{
  return EFI_UNSUPPORTED;
}
