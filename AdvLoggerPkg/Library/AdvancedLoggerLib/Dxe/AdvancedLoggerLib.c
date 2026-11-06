/** @file
  DXE implementation of Advanced Logger Library.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <AdvancedLoggerInternal.h>

#include <Protocol/AdvancedLogger.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC ADVANCED_LOGGER_PROTOCOL  *mLoggerProtocol = NULL;

/**
  Locate the Advanced Logger protocol.

  @param ImageHandle  The image handle.
  @param SystemTable  The system table.

  @retval EFI_SUCCESS  Initialization completed, even if the protocol is absent.
**/
EFI_STATUS
EFIAPI
DxeAdvancedLoggerLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mLoggerProtocol = NULL;
  if (gBS != NULL) {
    Status = gBS->LocateProtocol (
                    &gAdvancedLoggerProtocolGuid,
                    NULL,
                    (VOID **)&mLoggerProtocol
                    );

    if (!EFI_ERROR (Status) && (mLoggerProtocol != NULL)) {
      if ((mLoggerProtocol->Signature != ADVANCED_LOGGER_PROTOCOL_SIGNATURE) ||
          (mLoggerProtocol->Version != ADVANCED_LOGGER_PROTOCOL_VERSION))
      {
        ASSERT (mLoggerProtocol->Signature == ADVANCED_LOGGER_PROTOCOL_SIGNATURE);
        ASSERT (mLoggerProtocol->Version == ADVANCED_LOGGER_PROTOCOL_VERSION);
        Status = EFI_NOT_FOUND;
      }
    }

    if (EFI_ERROR (Status)) {
      mLoggerProtocol = NULL;
    }
  }

  return EFI_SUCCESS;
}

/**
  Forward a message to the protocol cached by the constructor, if available.
**/
VOID
EFIAPI
AdvancedLoggerWrite (
  IN       UINTN  DebugLevel,
  IN CONST CHAR8  *Buffer,
  IN       UINTN  NumberOfBytes
  )
{
  if (mLoggerProtocol != NULL) {
    mLoggerProtocol->AdvancedLoggerWriteProtocol (mLoggerProtocol, DebugLevel, Buffer, NumberOfBytes);
  }
}
