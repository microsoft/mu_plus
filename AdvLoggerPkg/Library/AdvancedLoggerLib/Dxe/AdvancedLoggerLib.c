/** @file
  DXE implementation of Advanced Logger Library.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/AdvancedLogger.h>
#include <Protocol/DebugPort.h>

#include <Library/UefiBootServicesTableLib.h>

STATIC EFI_DEBUGPORT_PROTOCOL   *mDebugPortProtocol = NULL;
STATIC ADVANCED_LOGGER_PROTOCOL *mLoggerProtocol = NULL;
STATIC BOOLEAN                   mInitialized = FALSE;

/**
  Write data from buffer to possible debugging devices.

  This is the interface from PeiCore
  This is also called by the Ppi

  Writes NumberOfBytes data bytes from Buffer to the debugging devices.

  @param  ErrorLevel       Error level of items top be printed
  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the serial device.


**/
VOID
EFIAPI
AdvancedLoggerWrite (
    IN       UINT32   DebugLevel,
    IN CONST CHAR8   *Buffer,
    IN       UINTN    NumberOfBytes
  ) {
    UINTN           BufferLen;
    EFI_STATUS      Status;

    if (!mInitialized) {
        mInitialized = TRUE;

        // NOTE:
        //
        // When they are present, AdvancedLogger protocol and DebugPort protocol are assumed to
        // be published early in DXE_CORE.  There is no dependency on these protocols, as
        // a shell application could be generated using this library function.  If neither
        // of these protocols are installed, then debug messages will just not appear.
        //
        // The locate protocols here could have been done in the library constructor, but
        // that complicates driver ordering as DebugLib is attached to everything.  There
        // are just some modules that will cause a build error with a constructor conflict.
        // This also allows the first debug message to be logged regardless of the library
        // initialization order.

        Status = gBS->LocateProtocol (&gAdvancedLoggerProtocolGuid,
                                       NULL,
                                      (VOID **) &mLoggerProtocol);
        if (EFI_ERROR(Status)) {
            mLoggerProtocol = NULL;
            Status = gBS->LocateProtocol (&gEfiDebugPortProtocolGuid,
                                           NULL,
                                           (VOID **) &mDebugPortProtocol);
            if (EFI_ERROR(Status)) {
                mDebugPortProtocol = NULL;
            }
        }
    }

    // Log to Advanced Logger first, and if no Advanced Logger, log to DebugPort.  This
    // allows unit tests and shell applications to be compiled in the Advanced Logger
    // environment and still allow debug messages to be logged.

    if (mLoggerProtocol != NULL) {
        mLoggerProtocol->AdvancedLoggerWrite (DebugLevel, Buffer, NumberOfBytes);
    } else {
        if (mDebugPortProtocol != NULL) {
            BufferLen = NumberOfBytes;
            mDebugPortProtocol->Write (mDebugPortProtocol, 500, &BufferLen, (VOID *) Buffer);
        }
    }
}
