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
STATIC UINT32                mBufferSize = 0;
STATIC EFI_PHYSICAL_ADDRESS  mMaxAddress = 0;
STATIC BOOLEAN               mInitialized = FALSE;


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
            if (mLoggerInfo != NULL) {
                 mMaxAddress = PA_FROM_PTR(mLoggerInfo) + mLoggerInfo->LogBufferSize;
            }
        }

        //
        // If mLoggerInfo is NULL at this point, there is no Advanced Logger.
        //

        DEBUG((DEBUG_INFO, "%a: LoggerInfo=%p, code=%r\n", __FUNCTION__, mLoggerInfo, Status));
    }

    if (!ValidateInfoBlock()) {
        mLoggerInfo = NULL;
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
