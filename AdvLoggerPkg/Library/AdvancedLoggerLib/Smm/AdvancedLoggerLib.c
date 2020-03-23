/** @file
  SMM implementation of Advanced Logger Library.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <AdvancedLoggerInternal.h>

#include <Protocol/AdvancedLogger.h>

#include <Library/AdvancedLoggerLib.h>
#include <Library/DebugLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "../AdvancedLoggerCommon.h"

STATIC ADVANCED_LOGGER_INFO *mLoggerInfo = NULL;
STATIC BOOLEAN               mInitialized = FALSE;

/**
    Get the Logger Information Block published by DxeCore.
 **/
STATIC
VOID
SmmInitializeLoggerInfo (
    VOID
    ) {
    ADVANCED_LOGGER_PROTOCOL *LoggerProtocol;
    EFI_STATUS                Status;

    if (gBS == NULL)  {
        return;
    }

    if (!mInitialized) {
        //
        // Locate the Logger Information block.
        //

        mInitialized = TRUE;            // Only one attempt at getting the logger info block.

        Status = gBS->LocateProtocol (&gAdvancedLoggerProtocolGuid,
                                       NULL,
                                      (VOID **) &LoggerProtocol);
        if (!EFI_ERROR(Status)) {
            mLoggerInfo = (ADVANCED_LOGGER_INFO *) LoggerProtocol->Context;
            if (mLoggerInfo->Signature != ADVANCED_LOGGER_SIGNATURE) {
                mLoggerInfo = NULL;
            }
        }

        //
        // If mLoggerInfo is NULL at this point, there is no Advanced Logger.
        //

        DEBUG((DEBUG_INFO, "%a: LoggerInfo=%p, code=%r\n", __FUNCTION__, mLoggerInfo, Status));
    }
}

/**
  Return the Logger Information block

**/
ADVANCED_LOGGER_INFO *
EFIAPI
AdvancedLoggerGetLoggerInfo (
    VOID
) {

    SmmInitializeLoggerInfo ();

    return mLoggerInfo;
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
