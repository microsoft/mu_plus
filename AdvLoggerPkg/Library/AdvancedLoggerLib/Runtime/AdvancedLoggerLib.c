/** @file
  DXE implementation of Advanced Logger Library.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <AdvancedLoggerInternal.h>

#include <Guid/EventGroup.h>

#include <Protocol/AdvancedLogger.h>

#include "../AdvancedLoggerCommon.h"

STATIC ADVANCED_LOGGER_INFO    *mLoggerInfo = NULL;
STATIC BOOLEAN                  mAtRuntime = FALSE;
STATIC EFI_EVENT                mExitBootServicesEvent = NULL;
STATIC EFI_BOOT_SERVICES       *mBS = NULL;

/**
    Get the Logger Information block

 **/
ADVANCED_LOGGER_INFO *
EFIAPI
AdvancedLoggerGetLoggerInfo (
    VOID
) {
    ADVANCED_LOGGER_PROTOCOL   *LoggerProtocol;
    EFI_STATUS                  Status;

    if (!mAtRuntime) {
        if ((mLoggerInfo == NULL) && (mBS != NULL)) {
            Status = mBS->LocateProtocol (&gAdvancedLoggerProtocolGuid,
                                           NULL,
                                           (VOID **) &LoggerProtocol);
            if (!EFI_ERROR(Status) && (LoggerProtocol != NULL)) {
                mLoggerInfo = (ADVANCED_LOGGER_INFO *) LoggerProtocol->Context;
            }
        }
    }

    return mLoggerInfo;
}

/**
    Exit Boot Services Event notification handler.

    @param[in]  Event     Event whose notification function is being invoked.
    @param[in]  Context   Pointer to the notification function's context.

**/
STATIC
VOID
EFIAPI
AllOnExitBootServices (
    IN      EFI_EVENT                         Event,
    IN      VOID                              *Context
    )
{
    //
    // Stop Memory logging :
    //
    if (mLoggerInfo != NULL) {
        mLoggerInfo->ExitBootServices = TRUE;
    }

    mLoggerInfo = NULL;
    mAtRuntime = TRUE;
}

/**
    The constructor registers for the ExitBootServices event.

    @param  ImageHandle   The firmware allocated handle for the EFI image.
    @param  SystemTable   A pointer to the EFI System Table.

    @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DxeRuntimeAdvancedLoggerLibConstructor (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    )
{
    mBS = SystemTable->BootServices;

    //
    // Register for the ExitBootServices event to shutdown the in memory logger.
    //
    mBS->CreateEventEx (EVT_NOTIFY_SIGNAL,
                        TPL_CALLBACK,
                        AllOnExitBootServices,
                        NULL,
                       &gEfiEventExitBootServicesGuid,
                       &mExitBootServicesEvent);

    return EFI_SUCCESS;
}

/**
    The destructor function closes the exit boot services event

    @param  ImageHandle   The firmware allocated handle for the EFI image.
    @param  SystemTable   A pointer to the EFI System Table.

    @retval EFI_SUCCESS   The destructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DxeRuntimeAdvancedLoggerLibDestructor (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    )
{

    if (mExitBootServicesEvent != NULL) {
        mBS->CloseEvent (mExitBootServicesEvent);
    }

    return EFI_SUCCESS;
}
