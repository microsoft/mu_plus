/** @file
  NULL version of Advanced Logger Hdw Port Library

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>

/**
  Initialize the hardware port.

  If no initialization is required, then return EFI_SUCCESS.
  If the hardware device was successfully initialized, then return EFI_SUCCESS.
  If the Hardware device could not be initialized, then return EFI_DEVICE_ERROR.

  @retval EFI_SUCCESS        The hardware device was initialized.
  @retval EFI_DEVICE_ERROR   The hardware device could not be initialized.

**/
EFI_STATUS
EFIAPI
AdvancedLoggerHdwPortInitialize (
  VOID
  )
{
    return EFI_SUCCESS;
}

/**
  Write data from buffer to hardware device.

  Writes NumberOfBytes data bytes from Buffer to the hardware device.
  The number of bytes actually written to the hardware device is returned.
  If the return value is less than NumberOfBytes, then the write operation failed.
  If Buffer is NULL, then ASSERT().
  If NumberOfBytes is zero, then return 0.

  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the hardware device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes written to the hardware device.
                           If this value is less than NumberOfBytes, then the write operation failed.

**/
UINTN
EFIAPI
AdvancedLoggerHdwPortWrite (
  IN UINTN     DebugLevel,
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
  )
{

    return NumberOfBytes;
}
