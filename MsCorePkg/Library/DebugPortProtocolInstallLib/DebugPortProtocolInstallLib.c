/** @file

Shim lib used to install a DebugPort protocol against linked DebugLib

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

