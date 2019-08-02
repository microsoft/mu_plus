/** @file

Shim lib used to install a DebugPort protocol against linked DebugLib

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Protocol/DebugPort.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

/**
 * DebugPortReset - no-op, nothing to reset
 *
 * @param This
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DebugPortReset (
  IN EFI_DEBUGPORT_PROTOCOL   *This
  )
{
  return EFI_SUCCESS;
}

/**
 * DebugPortWrite - map debug port write to DebugPrint
 *
 *
 * @param This
 * @param Timeout
 * @param BufferSize
 * @param Buffer
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DebugPortWrite (
  IN EFI_DEBUGPORT_PROTOCOL   *This,
  IN UINT32                   Timeout,
  IN OUT UINTN                *BufferSize,
  OUT VOID                    *Buffer
  )
{
    UINT8 DbgString[100];       //Some convenient size
    UINTN Size;
    UINTN CopySize;
    CHAR8 *Data;

    // Break up the array of char8's as there is no implied NULL
    // in the input buffer.
    Data = (CHAR8 *) Buffer;
    Size = *BufferSize;
    while (Size > 0) {
        CopySize = MIN(Size,sizeof(DbgString)-1);
        CopyMem (DbgString,Data,CopySize);
        DbgString[CopySize] = '\0';
        DebugPrint(DEBUG_ERROR,"%a",DbgString);
        Size -= CopySize;
        Data += CopySize;
    }
    return EFI_SUCCESS;
}

/**
 * DebugPortRead - not supported.  Return Timeout
 *
 *
 * @param This
 * @param Timeout
 * @param BufferSize
 * @param Buffer
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DebugPortRead (
  IN EFI_DEBUGPORT_PROTOCOL   *This,
  IN UINT32                   Timeout,
  IN OUT UINTN                *BufferSize,
  IN VOID                     *Buffer
  )
{
    *BufferSize = 0;
    return EFI_TIMEOUT;
}

/**
 * DebugPortPoll -- Not supported, return NotReady.
 *
 * @param This
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DebugPortPoll (
  IN EFI_DEBUGPORT_PROTOCOL   *This
  )
{
  return EFI_NOT_READY;
}

EFI_DEBUGPORT_PROTOCOL    mDebugPortInterface =
    {
    DebugPortReset,
    DebugPortWrite,
    DebugPortRead,
    DebugPortPoll
  };

/**
  Install the Debug Port protocol

 * @return EFI_STATUS EFIAPI
--*/
EFI_STATUS
EFIAPI
InstallDebugPortProtocol (
    IN EFI_HANDLE          ImageHandle,
    IN EFI_SYSTEM_TABLE   *SystemTable
) {

    EFI_STATUS              Status;

    Status = SystemTable->BootServices->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiDebugPortProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mDebugPortInterface);

  if (EFI_ERROR(Status))
  {
    ASSERT_EFI_ERROR(Status);
  }

    return EFI_SUCCESS;
}

