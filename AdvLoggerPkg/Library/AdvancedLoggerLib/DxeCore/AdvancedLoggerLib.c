/** @file
  DXE_CORE implementation of Advanced Logger Library.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <AdvancedLoggerInternal.h>

#include <Protocol/AdvancedLogger.h>

#include <Library/AdvancedLoggerHdwPortLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/TimerLib.h>

#include "../AdvancedLoggerCommon.h"

//
// Protocol interface that connects the DXE library instances with the AdvancedLogger
//
STATIC ADVANCED_LOGGER_INFO    *mLoggerInfo = NULL;
STATIC UINT32                   mBufferSize = 0;
STATIC EFI_PHYSICAL_ADDRESS     mMaxAddress = 0;

STATIC ADVANCED_LOGGER_PROTOCOL mLoggerProtocol = {
                                                   ADVANCED_LOGGER_PROTOCOL_SIGNATURE,
                                                   0,
                                                   AdvancedLoggerWrite,
                                                   0
};

/**
    ValidateInfoBlock

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

    if (mLoggerInfo->LogBuffer != PA_FROM_PTR(mLoggerInfo + 1)) {
        return FALSE;
    }

    if ((mLoggerInfo->LogCurrent > mMaxAddress) ||
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
    EFI_HOB_GUID_TYPE          *GuidHob;
    ADVANCED_LOGGER_PTR        *LogPtr;

    if (mLoggerInfo == NULL) {
        //
        // Locate the Logger Information block.
        //
        GuidHob = GetFirstGuidHob (&gAdvancedLoggerHobGuid);
        if (GuidHob != NULL) {
            LogPtr = (ADVANCED_LOGGER_PTR * ) GET_GUID_HOB_DATA(GuidHob);
            mLoggerInfo = ALI_FROM_PA(LogPtr->LogBuffer);
            mLoggerProtocol.Context = (VOID *) mLoggerInfo;
            if (!mLoggerInfo->HdwPortInitialized) {
                AdvancedLoggerHdwPortInitialize();
                mLoggerInfo->HdwPortInitialized = TRUE;
            }

            if (mLoggerInfo != NULL) {
                mMaxAddress = mLoggerInfo->LogBuffer + mLoggerInfo->LogBufferSize;
            }
        }
    }

    if (!ValidateInfoBlock()) {
        mLoggerInfo = NULL;
    }

    return mLoggerInfo;
}

/**
    OnRuntimeArchNotification

    Collect the UEFI system time.

  **/
STATIC
VOID
EFIAPI
OnRealTimeClockArchNotification (
    IN  EFI_EVENT   Event,
    IN  VOID        *Context
    )
{
    EFI_STATUS         Status;
    EFI_SYSTEM_TABLE  *SystemTable;

    SystemTable = (EFI_SYSTEM_TABLE *) Context;

    DEBUG((DEBUG_INFO, "%a: getting real time\n", __FUNCTION__));

    SystemTable->BootServices->CloseEvent (Event);

    Status = SystemTable->RuntimeServices->GetTime ((EFI_TIME *) &mLoggerInfo->Time, NULL);
    if (EFI_ERROR (Status)) {
        DEBUG((DEBUG_INFO, "%a: error getting real time. Code=%r\n", __FUNCTION__, Status));
    } else {
        mLoggerInfo->TicksAtTime = GetPerformanceCounter();
    }

    return;
}

/**
    OnVariableWriteNotification

    Writes the log locator variable.

  **/
STATIC
VOID
EFIAPI
OnVariableWriteNotification (
    IN  EFI_EVENT   Event,
    IN  VOID        *Context
    )
{
    IN EFI_SYSTEM_TABLE  *SystemTable;

    SystemTable = (EFI_SYSTEM_TABLE *) Context;

    DEBUG((DEBUG_INFO, "%a: writing locator variable\n", __FUNCTION__));

    SystemTable->RuntimeServices->SetVariable (
        ADVANCED_LOGGER_LOCATOR_NAME,
        &gAdvancedLoggerHobGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        sizeof(mLoggerInfo),
        (VOID *) &mLoggerInfo );

    SystemTable->BootServices->CloseEvent (Event);

    return;
}

/**
    ProcessProtocolRegistration

    This function registers for Variable Write being available.

    @param       VOID

    @retval      EFI_SUCCESS     Variable Write protocol registration successful
    @retval      error code      Something went wrong.

 **/
EFI_STATUS
ProcessProtocolRegistration (
    IN EFI_SYSTEM_TABLE  *SystemTable,
    IN EFI_GUID          *ProtocolGuid,
    IN EFI_EVENT_NOTIFY   NotifyFunction
    )
{
    EFI_STATUS      Status;
    EFI_EVENT       ProtocolEvent;
    VOID           *ProtocolRegistration;

    //
    // Register for protocol notification.
    //
    DEBUG((DEBUG_INFO, "%a: Registering for %g\n", __FUNCTION__, ProtocolGuid));
    Status = SystemTable->BootServices->CreateEvent (
                               EVT_NOTIFY_SIGNAL,
                               TPL_CALLBACK,
                               NotifyFunction,
                               SystemTable,
                               &ProtocolEvent);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: failed to create notification callback event (%r)\n", __FUNCTION__, Status));
        goto Cleanup;
    }

    Status = SystemTable->BootServices->RegisterProtocolNotify (
                                           ProtocolGuid,
                                           ProtocolEvent,
                                          &ProtocolRegistration);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: failed to register for notification (%r)\n", __FUNCTION__, Status));
        SystemTable->BootServices->CloseEvent (ProtocolEvent);
        goto Cleanup;
    }

    Status = EFI_SUCCESS;

Cleanup:
    return Status;
}

/**
  DxeCore Advanced Logger initialization.
 **/
EFI_STATUS
EFIAPI
DxeCoreAdvancedLoggerLibConstructor (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
  ) {
    ADVANCED_LOGGER_INFO *LoggerInfo;
    EFI_STATUS            Status;

    LoggerInfo = AdvancedLoggerGetLoggerInfo ();    // Sets mLoggerInfo if LoggerInfo found

    //
    // For an implementation of the AdvancedLogger with a PEI implementation, there will be a
    // Logger Information block published and available.
    //
    if (LoggerInfo == NULL) {
        LoggerInfo = (ADVANCED_LOGGER_INFO *) AllocateReservedPages(FixedPcdGet32 (PcdAdvancedLoggerPages));
        if (LoggerInfo != NULL) {
            ZeroMem ((VOID *) LoggerInfo, sizeof(ADVANCED_LOGGER_INFO));
            LoggerInfo->Signature = ADVANCED_LOGGER_SIGNATURE;
            LoggerInfo->Version = ADVANCED_LOGGER_VERSION;
            LoggerInfo->LogBuffer = PA_FROM_PTR(LoggerInfo + 1);
            LoggerInfo->LogBufferSize = EFI_PAGES_TO_SIZE (FixedPcdGet32 (PcdAdvancedLoggerPages)) - sizeof(ADVANCED_LOGGER_INFO);
            LoggerInfo->LogCurrent = LoggerInfo->LogBuffer;
            mLoggerInfo = LoggerInfo;
            mMaxAddress = PA_FROM_PTR(mLoggerInfo) + mLoggerInfo->LogBufferSize;
            mLoggerProtocol.Context = (VOID *) mLoggerInfo;
        } else {
            DEBUG((DEBUG_ERROR, "%a: Error allocating Advanced Logger Buffer\n", __FUNCTION__));
        }
    } else {
        mLoggerProtocol.Context = (VOID *) mLoggerInfo;
    }

    if (mLoggerInfo != NULL) {
        mLoggerInfo->TimerFrequency = GetPerformanceCounterProperties (NULL, NULL);
        Status = SystemTable->BootServices->InstallProtocolInterface (&ImageHandle,
                                                                      &gAdvancedLoggerProtocolGuid,
                                                                       EFI_NATIVE_INTERFACE,
                                                                      &mLoggerProtocol);

        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: Error installing protocol - %r\n", __FUNCTION__, Status));
            // If the protocol doesn't install, don't fail.
        }
    }

    DEBUG((DEBUG_INFO, "%a Initialized. mLoggerInfo = %p\n", __FUNCTION__, mLoggerInfo));

    ProcessProtocolRegistration (
        SystemTable,
        &gEfiRealTimeClockArchProtocolGuid,
        OnRealTimeClockArchNotification
        );

    if (FeaturePcdGet(PcdAdvancedLoggerLocator)) {
        ProcessProtocolRegistration (
            SystemTable,
            &gEfiVariableWriteArchProtocolGuid,
            OnVariableWriteNotification
            );
    }

    return EFI_SUCCESS;
}
