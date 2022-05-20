/** @file
  AdvLoggerSerialPortLib is an implementation of SerialPortLib that doesn't write to a serial
  port - only to the AdvLogger, which might write to a serial port. Allows AdvLogger to
  intercept FSP serial port writes.  Can be attached to other serial port writers in place
  of serial port lib.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <AdvancedLoggerInternal.h>

#include <Protocol/SerialIo.h>

#include <Library/AdvancedLoggerLib.h>

/**
  Initialize the serial device hardware.

  This is done by AdvancedLoggerLib.

  @retval RETURN_SUCCESS

**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  return EFI_SUCCESS;
}

/**
  Write data from buffer to serial device.

  Writes NumberOfBytes data bytes from Buffer to the AdvancedLogger

  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the serial device.

  @retval NumberOfBytes    Force success for write.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  AdvancedLoggerWrite (0xFFFFFFFF, (CHAR8 *)Buffer, NumberOfBytes);

  return NumberOfBytes;
}

/**
  Read data from serial device and save the data in buffer.

  Not Supported

  @retval 0                Read data not supported, no data is to be read.

**/
UINTN
EFIAPI
SerialPortRead (
  OUT UINT8  *Buffer,
  IN  UINTN  NumberOfBytes
  )
{
  return 0;
}

/**
  Polls a serial device to see if there is any data waiting to be read.

  Not supported.  Returns no data waiting.

  @retval FALSE            There is no data waiting to be read from the serial device.

**/
BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  )
{
  return FALSE;
}

/**
  Sets the control bits on a serial device.

  Not Supported.

  @retval RETURN_UNSUPPORTED    The serial device does not support this operation.

**/
RETURN_STATUS
EFIAPI
SerialPortSetControl (
  IN UINT32  Control
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Retrieve the status of the control bits on a serial device.

  Not Supported.

  @retval RETURN_UNSUPPORTED    The serial device does not support this operation.

**/
RETURN_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32  *Control
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Sets the baud rate, receive FIFO depth, transmit/receive time out, parity,
  data bits, and stop bits on a serial device.

  Not Supported.

  @retval RETURN_UNSUPPORTED        The serial device does not support this operation.

**/
RETURN_STATUS
EFIAPI
SerialPortSetAttributes (
  IN OUT UINT64              *BaudRate,
  IN OUT UINT32              *ReceiveFifoDepth,
  IN OUT UINT32              *Timeout,
  IN OUT EFI_PARITY_TYPE     *Parity,
  IN OUT UINT8               *DataBits,
  IN OUT EFI_STOP_BITS_TYPE  *StopBits
  )
{
  return EFI_UNSUPPORTED;
}

/**
  FSP Special Sauce.
**/
UINT8
EFIAPI
GetDebugInterfaceFlags (
  VOID
  )
{
  return 0x02;                     // UART
}
