/** @file
  SMM implementation of Advanced Logger Library.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <AdvancedLoggerInternal.h>

#include <Protocol/AdvancedLogger.h>

#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>

STATIC BOOLEAN                   mInitialized = FALSE;
STATIC ADVANCED_LOGGER_PROTOCOL  *mSmmLoggerProtocol;

/**
    Get the Logger Information Block published by SmmCore.
 **/
STATIC
VOID
SmmInitializeLoggerInfo (
  VOID
  )
{
  EFI_STATUS  Status;

  if (gSmst == NULL) {
    return;
  }

  if (!mInitialized) {
    //
    // Locate the Logger Information block.
    //

    mInitialized = TRUE;                // Only one attempt at getting the logger info block.

    Status = gSmst->SmmLocateProtocol (
                      &gAdvancedLoggerProtocolGuid,
                      NULL,
                      (VOID **)&mSmmLoggerProtocol
                      );

    if (EFI_ERROR (Status)) {
      mSmmLoggerProtocol = NULL;
    } else {
      ASSERT (mSmmLoggerProtocol->Signature == ADVANCED_LOGGER_PROTOCOL_SIGNATURE);
      ASSERT (mSmmLoggerProtocol->Version == ADVANCED_LOGGER_PROTOCOL_VERSION);
    }

    //
    // If mSmmLoggerProtocol is NULL at this point, there is no Advanced Logger for SMM modules.
    //

    DEBUG ((DEBUG_INFO, "%a: SmmLoggerProtocol=%p, code=%r\n", __FUNCTION__, mSmmLoggerProtocol, Status));
  }
}

/**
  Advanced Logger Write

  @param  ErrorLevel      The error level of the debug message.
  @param  Buffer          The debug message to log.
  @param  NumberOfBytes   Number of bytes in the debug message.

**/
VOID
EFIAPI
AdvancedLoggerWrite (
  IN        UINTN  ErrorLevel,
  IN  CONST CHAR8  *Buffer,
  IN        UINTN  NumberOfBytes
  )
{
  SmmInitializeLoggerInfo ();

  if (mSmmLoggerProtocol != NULL) {
    mSmmLoggerProtocol->AdvancedLoggerWriteProtocol (mSmmLoggerProtocol, ErrorLevel, Buffer, NumberOfBytes);
  }
}

/**
  The constructor function initializes Logger Information pointer to ensure that the
  pointer is initialized in DXE - either by the constructor, or the first DEBUG message.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SmmAdvancedLoggerLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  SmmInitializeLoggerInfo ();

  return EFI_SUCCESS;
}
