/** @file
  DXE implementation of Advanced Logger Library.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <AdvancedLoggerInternal.h>

#include <Guid/EventGroup.h>

#include <Protocol/AdvancedLogger.h>
#include <Library/DebugLib.h>

#include "../AdvancedLoggerCommon.h"

STATIC ADVANCED_LOGGER_INFO    *mLoggerInfo = NULL;
STATIC UINT32                   mBufferSize = 0;
STATIC EFI_PHYSICAL_ADDRESS     mMaxAddress = 0;
STATIC EFI_BOOT_SERVICES       *mBS = NULL;
STATIC EFI_EVENT                mExitBootServicesEvent = NULL;


/**
    CheckAddress

    The address of the ADVANCE_LOGGER_INFO block pointer is captured before END_OF_DXE.  The
    pointers LogBuffer and LogCurrent, and LogBufferSize, could be written to by untrusted code.  Here, we check that
    the pointers are within the allocated mLoggerInfo space, and that LogBufferSize, which is used in multiple places
    to see if a new message will fit into the log buffer, is valid.

    @param          NONE

    @return         BOOLEAN     TRUE - mInforBlock passes security checks
    @return         BOOLEAN     FALSE- mInforBlock failed security checks

**/
STATIC
BOOLEAN
ValidateInfoBlock (
    VOID
  ) {

    if (mLoggerInfo == NULL) {
        return FALSE;
    }

    if (mLoggerInfo->Signature != ADVANCED_LOGGER_SIGNATURE) {
        return FALSE;
    }

    if (mLoggerInfo->LogBuffer != (PA_FROM_PTR(mLoggerInfo) + sizeof(ADVANCED_LOGGER_INFO))) {
        return FALSE;
    }

    if ((mLoggerInfo->LogCurrent >= mMaxAddress) ||
        (mLoggerInfo->LogCurrent < mLoggerInfo->LogBuffer)) {
        return FALSE;
    }

    if (mBufferSize == 0) {
        mBufferSize = mLoggerInfo->LogBufferSize;
    } else {
        if (mLoggerInfo->LogBufferSize != mBufferSize) {
            return FALSE;
        }
    }

    return TRUE;
}

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

    if ((mLoggerInfo == NULL) && (mBS != NULL)) {
        Status = mBS->LocateProtocol (&gAdvancedLoggerProtocolGuid,
                                       NULL,
                                       (VOID **) &LoggerProtocol);
        if (!EFI_ERROR(Status) && (LoggerProtocol != NULL)) {
            mLoggerInfo = (ADVANCED_LOGGER_INFO *) LoggerProtocol->Context;
            if (mLoggerInfo != NULL) {
                mMaxAddress = PA_FROM_PTR(mLoggerInfo) + mLoggerInfo->LogBufferSize;
            }
        }
    }

    if (!ValidateInfoBlock()) {
        mLoggerInfo = NULL;
    }

    return mLoggerInfo;
}

/**
    Inform all instances of Advanced Logger that ExitBoot Services has occurred.

    @param    Event           Not Used.
    @param    Context         Not Used.

   @retval   none
 **/
VOID
EFIAPI
OnExitBootServicesNotification (
    IN EFI_EVENT        Event,
    IN VOID             *Context
  )
{

    //
    // Runtime logging is currently not supported, so clear mLoggerInfo.  This
    // will also stop serial port output from Runtime Advanced Logger instances.
    //
    mLoggerInfo = NULL;
    mBS = NULL;
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
    EFI_STATUS                      Status;

    //
    // Cache BootServices pointer as AdvLogger calls may be made before
    // the constructor runs.
    //
    mBS = SystemTable->BootServices;
    AdvancedLoggerGetLoggerInfo ();

    ASSERT (mLoggerInfo != NULL);

    if (mLoggerInfo != 0) {
        //
        // Register notify function for ExitBootServices.
        //
        Status = mBS->CreateEvent ( EVT_SIGNAL_EXIT_BOOT_SERVICES,
                                    TPL_CALLBACK,
                                    OnExitBootServicesNotification,
                                    NULL,
                                   &mExitBootServicesEvent);

        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a - Create Event for Address Change failed. Code = %r\n", __FUNCTION__, Status));
        }
    }

    return EFI_SUCCESS;
}

/**
    The destructor closes the registered event in case of driver failure.

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
