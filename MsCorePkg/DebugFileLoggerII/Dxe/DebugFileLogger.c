/** @file DebugFileLogger.c

    Copyright (C) Microsoft Corporation. All rights reserved.
    SPDX-License-Identifier: BSD-2-Clause-Patent

    This file contains functions for logging debug print messages to a file.

**/

#include "DebugFileLogger.h"

//
// Global variables.
//

VOID                       *mFileSystemRegistration = NULL;
CHAR8                      *mLoggingBuffer = NULL;
UINT64                      mLoggingBuffer_BytesWritten = 0;
UINT64                      mLoggingBuffer_Size = 0;
LIST_ENTRY                  mLoggingDeviceHead = INITIALIZE_LIST_HEAD_VARIABLE (mLoggingDeviceHead);
UINT32                      mLoggingSemaphore = 0;
EFI_RSC_HANDLER_PROTOCOL   *mRscHandlerProtocol = NULL;
UINT32                      mWritingSemaphore = 0;

/**

    Initialize the debug logging buffer and capture PEI logging if it's present.

    @param    None

    @retval   EFI_SUCCESS     The buffer was successfully initialized.
    @retval   Others          The operation failed.

**/
STATIC
EFI_STATUS
LoggingBufferInit (
    VOID
    )
{
    UINTN                           CharCount = 0;
    EFI_HOB_GUID_TYPE              *GuidHob = NULL;
    VOID                           *PeiBuffer = NULL;
    EFI_DEBUG_FILELOGGING_HEADER   *PeiBufferHeader = NULL;
    EFI_STATUS                      Status = EFI_SUCCESS;

    //
    // Locate the MS debug logger hand-off-block.
    //
    GuidHob = GetFirstGuidHob (&gMuDebugLoggerGuid);
    if (GuidHob != NULL) {
        PeiBufferHeader = (EFI_DEBUG_FILELOGGING_HEADER*)(GET_GUID_HOB_DATA(GuidHob));
        if (PeiBufferHeader != NULL) {
            PeiBuffer = PeiBufferHeader + 1;
        }
    }

    //
    // Allocate a buffer for DXE logging.
    //
    mLoggingBuffer = (CHAR8 *) AllocatePages(EFI_SIZE_TO_PAGES(DEBUG_LOG_FILE_SIZE));

    //
    // If the allocation succeeded, capture the PEI log if possible and continue
    // with DXE logging.
    //
    if (mLoggingBuffer != NULL) {
        mLoggingBuffer_Size = DEBUG_LOG_FILE_SIZE;

        //
        // No MS debug logging HoB.
        //
        if (GuidHob == NULL) {
            DEBUG((DEBUG_WARN, "%a: Failed to locate Pei File Logger HOB.\n", __FUNCTION__));
            mLoggingBuffer_BytesWritten = AsciiSPrint (mLoggingBuffer,
                                                       EFI_STATUS_CODE_DATA_MAX_SIZE,
                                                       "ERROR: PEI HoB is missing.\r\n"); // NOTE: We're ignoring the NULL terminator.

            goto CleanUp;
        }

        //
        // PEI log wasn't found.
        //
        if (PeiBufferHeader == NULL) {
            DEBUG((DEBUG_WARN, "%a: Failed to locate Pei File logger buffer.\n", __FUNCTION__));
            mLoggingBuffer_BytesWritten = AsciiSPrint (mLoggingBuffer,
                                                       EFI_STATUS_CODE_DATA_MAX_SIZE,
                                                      "ERROR: PEI log is missing.\r\n"); // NOTE: We're ignoring the NULL terminator.

            goto CleanUp;
        }

        //
        // We found the PEI log, capture it in the DXE logging buffer.
        //
        PeiBufferHeader->BytesWritten &= ~EFI_DEBUG_FILE_LOGGER_OVERFLOW;  // Turn off overflow indicator
        DEBUG((DEBUG_INFO, "%a: PEI log contains %d bytes.\n", __FUNCTION__, PeiBufferHeader->BytesWritten));
        PeiBuffer = PeiBufferHeader + 1;

        //
        // Truncate the PEI log if it's larger than the space we have available.
        //
        CharCount = MIN(PeiBufferHeader->BytesWritten, DEBUG_LOG_FILE_SIZE);
        CopyMem(mLoggingBuffer, PeiBuffer, CharCount);
        mLoggingBuffer_BytesWritten = CharCount;

    } else {
        //
        // If we failed to allocate a DXE debug log buffer, at least ensure that
        // the PEI log (if it exists) gets flushed to the logging file.
        //
        if (PeiBufferHeader != NULL) {
            mLoggingBuffer = (CHAR8 *)PeiBuffer;
            mLoggingBuffer_BytesWritten = PeiBufferHeader->BytesWritten;
            mLoggingBuffer_Size = PEI_BUFFER_SIZE_DEBUG_FILE_LOGGING;
        } else {
            mLoggingBuffer = NULL;
            mLoggingBuffer_BytesWritten = 0;
            mLoggingBuffer_Size = 0;
            Status = EFI_OUT_OF_RESOURCES;
        }
    }

CleanUp:

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: failed to initialize debug logging buffer (%r)\n", __FUNCTION__, Status));
    }

    return Status;
}

/**
    WriteLogFiles

    Write current log file to all of the logged file systems

  **/
VOID
WriteLogFiles (
    VOID
    )
{
    LIST_ENTRY     *Link;
    LOG_DEVICE     *LogDevice;
    UINT64          TimeEnd;
    UINT64          TimeStart;

    //
    // Use an atomic lock to catch a re-entrant call. Non-zero means we've entered
    // a second time.
    //
    DEBUG((DEBUG_INFO, "Entry to WriteLogFiles.\n"));
    if (InterlockedCompareExchange32 (&mWritingSemaphore, 0, 1) != 0) {
        DEBUG((DEBUG_ERROR, "WriteLogFiles blocked.\n"));
        return;
    }

    TimeStart = GetPerformanceCounter ();

    EFI_LIST_FOR_EACH(Link, &(mLoggingDeviceHead)) {
        LogDevice = LOG_DEVICE_FROM_LINK (Link);

        // Each logging device coud arrive with different data, so always pass
        // the current buffer size.
        WriteALogFile (LogDevice, mLoggingBuffer, mLoggingBuffer_BytesWritten);
    }

    TimeEnd = GetPerformanceCounter ();
    DEBUG((DEBUG_ERROR, "Time to write logs: %ld ms\n\n", (GetTimeInNanoSecond(TimeEnd-TimeStart) / (1024 * 1024)) ));

    //
    // Release the lock.
    //
    InterlockedCompareExchange32 (&mWritingSemaphore, 1, 0);
    DEBUG((DEBUG_INFO, "Exit from WriteLogFiles.\n"));
}

/**
    OnResetNotification

    Write the log files if the reset occurs as a reasonable TPL.

  **/
STATIC
VOID
EFIAPI
OnResetNotification (
    IN EFI_RESET_TYPE ResetType,
    IN EFI_STATUS     ResetStatus,
    IN UINTN          DataSize,
    IN VOID          *ResetData OPTIONAL
    )
{
    EFI_TPL         OldTpl;

    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    gBS->RestoreTPL (OldTpl);

    DEBUG((DEBUG_INFO, "OnResetNotification\n"));
    if (OldTpl <= TPL_CALLBACK) {
        WriteLogFiles ();
    } else {
        DEBUG((DEBUG_ERROR, "Unable to write log at reset\n"));
    }

    return;
}

/**
    Write the log files at PostReadyToBoot.

    @param    Event           Not Used.
    @param    Context         Not Used.

   @retval   none
 **/
VOID
EFIAPI
OnPostReadyToBootNotification (
    IN EFI_EVENT        Event,
    IN VOID             *Context
  )
{

    WriteLogFiles ();

    //
    //  Leave Event open to process subsequent ReadyToBoot notifications.
    //
}

/**
    Capture the data from a logging event and store the information in ASCII form in the logging buffer.

    @param    CodeType    Indicates the type of status code being reported.
    @param    Value       Describes the current status of a hardware or software entity.
                          This included information about the class and subclass that is used to
                          classify the entity as well as an operation.
    @param    Instance    The enumeration of a hardware or software entity within
                          the system. Valid instance numbers start with 1.
    @param    CallerId    This optional parameter may be used to identify the caller.
                          This parameter allows the status code driver to apply different rules to
                          different callers.
    @param    Data        This optional parameter may be used to pass additional data.

    @retval   EFI_STATUS              Event data was successfully captured.
    @retval   EFI_DEVICE_ERROR        Function has been called a second+ time (reentrancy error)
    @retval   EFI_OUT_OF_RESOURCES    Insufficient resources to complete the operation.
    @retval   Others                  The operation failed.

**/
EFI_STATUS
EFIAPI
DxeLoggingBufferCaptureEvent (
    IN EFI_STATUS_CODE_TYPE         CodeType,
    IN EFI_STATUS_CODE_VALUE        Value,
    IN UINT32                       Instance,
    IN EFI_GUID                    *CallerId,
    IN EFI_STATUS_CODE_DATA        *Data OPTIONAL
    )
{
    UINTN         BytesWritten = 0;
    CHAR8        *LoggingBufferWriteLoc;
    EFI_STATUS    Status = EFI_SUCCESS;

    //
    // Use an atomic lock to catch a re-entrant call. Non-zero means we've entered
    // a second time.
    //
    if (InterlockedCompareExchange32 (&mLoggingSemaphore, 0, 1) != 0) {
        DEBUG((DEBUG_ERROR, "CaptureEvent blocked.\n"));
        return EFI_DEVICE_ERROR;
    }

    // If we don't have enough space to log the current event, at least store what
    // we have thus far to the log file.
    if ((mLoggingBuffer_BytesWritten + EFI_STATUS_CODE_DATA_MAX_SIZE) > mLoggingBuffer_Size) {
        DEBUG((DEBUG_ERROR, "%a: buffer full, truncating at %d bytes.\n", __FUNCTION__, mLoggingBuffer_BytesWritten));
        //  -- this will be running at high level.  need to instead trigger event at callback level DflDxeLoggingBufferWriteToFile(0, NULL);
        Status = EFI_OUT_OF_RESOURCES;
        mRscHandlerProtocol->Unregister (DxeLoggingBufferCaptureEvent);
        goto CleanUp;
    }

    //
    // Capture the event information as ASCII text.
    //
    LoggingBufferWriteLoc = mLoggingBuffer + mLoggingBuffer_BytesWritten;
    BytesWritten = WriteStatusCodeToBuffer (CodeType,
                                            Value,
                                            Instance,
                                            (CONST EFI_GUID*)CallerId,
                                            Data,
                                            LoggingBufferWriteLoc,
                                            EFI_STATUS_CODE_DATA_MAX_SIZE);

    mLoggingBuffer_BytesWritten += BytesWritten;

CleanUp:

    //
    // Release the lock.
    //
    InterlockedCompareExchange32 (&mLoggingSemaphore, 1, 0);
    return Status;
}

/**
    OnRscHandlerProtocolInstalled

    This function gets called when the Report Status Code protocol
    is installed.  Register for ReportStatusCode data (Debug messages etc).


    @param  Event   - The event used to trigger this callback
    @param  Context - NULL (not context is supplied)

    @returns          No return information

  **/
VOID
EFIAPI
OnRscHandlerProtocolInstalled (
    IN  EFI_EVENT   Event,
    IN  VOID        *Context
    )
{
    EFI_STATUS Status;

    //
    // Get a pointer to the report status code protocol.
    //
    Status = gBS->LocateProtocol (&gEfiRscHandlerProtocolGuid,
                                   NULL,
                                   (VOID**)&mRscHandlerProtocol);

    if (!EFI_ERROR(Status)) {
        //
        // Register our logging event handler callback function.
        //
        DEBUG((DEBUG_INFO, "%a: Located RSC handler protocol. Registering handler\n", __FUNCTION__));
        Status = mRscHandlerProtocol->Register(DxeLoggingBufferCaptureEvent, TPL_HIGH_LEVEL);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: failed to register RSC handler (%r)\n", __FUNCTION__, Status));
        }
    } else {
        DEBUG((DEBUG_INFO, "%a: RSC handler protocol not installed.\n", __FUNCTION__));
    }

    if (Event != NULL) {
        gBS->CloseEvent(Event);
    }

    return;
}

/**
    OnResetNotificationProtocolInstalled

    This function gets called when the Reset Notification
    protocol is installed.  Register for Reset Notifications.


    @param  Event   - The event used to trigger this callback
    @param  Context - NULL (not context is supplied)

    @returns          No return information

  **/
VOID
EFIAPI
OnResetNotificationProtocolInstalled (
    IN  EFI_EVENT   Event,
    IN  VOID        *Context
    )
{
    EFI_RESET_NOTIFICATION_PROTOCOL *ResetNotificationProtocol;
    EFI_STATUS                       Status;

    DEBUG((DEBUG_INFO, "OnResetNotification protocol detected\n"));
    //
    // Get a pointer to the report status code protocol.
    //
    Status = gBS->LocateProtocol (&gEfiResetNotificationProtocolGuid,
                                   NULL,
                                   (VOID**)&ResetNotificationProtocol);

    if (!EFI_ERROR(Status)) {
        //
        // Register our reset notification request
        //
        DEBUG((DEBUG_INFO, "%a: Located Reset notification protocol. Registering handler\n", __FUNCTION__));
        Status = ResetNotificationProtocol->RegisterResetNotify (ResetNotificationProtocol, OnResetNotification);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: failed to register Reset Notification handler (%r)\n", __FUNCTION__, Status));
        }

        if (Event != NULL) {
            gBS->CloseEvent (Event);
        }
    } else {
        DEBUG((DEBUG_ERROR, "%a: Unable to locate Reset Notification Protocol.\n", __FUNCTION__));
    }

    return;
}

/**
    RegisterLogDevice

    Register this file system as a possible UEFI log device.

    1. Verify if \Logs directory exists
    2. If not exists and is USB device, not a log device
    3. Verify log files exist, creating them if necessary

    Log files are created the first time a particular file system is seen.  This is
    to reduce the possibility of corrupting the EFI System Partition that is shared
    with the OS. One normal boots, the log is just written to an existing log file.


    @param      Handle   - Handle that has the FileSystemProtocol installed

  **/
VOID
RegisterLogDevice (
    IN EFI_HANDLE Handle
    )
{
    LOG_DEVICE     *LogDevice;
    EFI_STATUS      Status;
    UINT64          TimeStart;
    UINT64          TimeEnd;

    TimeStart = GetPerformanceCounter ();

    LogDevice = (LOG_DEVICE *) AllocatePool (sizeof(LOG_DEVICE));
    ASSERT (LogDevice);
    if (NULL == LogDevice) {
        DEBUG((DEBUG_ERROR, "%a: Out of memory:\n", __FUNCTION__));
        return;
    }

    LogDevice->Signature = LOG_DEVICE_SIGNATURE;
    LogDevice->Handle = Handle;
    LogDevice->FileIndex = 0;
    LogDevice->CurrentOffset = 0;
    LogDevice->Valid = TRUE;

    Status = EnableLoggingOnThisDevice (LogDevice);

    if (EFI_ERROR(Status)) {
        FreePool (LogDevice);
    } else {
        InsertTailList (&mLoggingDeviceHead, &LogDevice->Link);
    }

    TimeEnd = GetPerformanceCounter();
    DEBUG((DEBUG_ERROR, "Time to init logs: %ld ms\n\n", (GetTimeInNanoSecond(TimeEnd-TimeStart) / (1024 * 1024)) ));
}

/**
  New File System Discovered.

  Register this device as a possible UEFI Log device.

  @param    Event           Not Used.
  @param    Context         Not Used.

  @retval   none

**/
VOID
EFIAPI
OnFileSystemNotification (
    IN  EFI_EVENT   Event,
    IN  VOID       *Context
    )
{
    UINTN           HandleCount;
    EFI_HANDLE     *HandleBuffer;
    EFI_STATUS      Status;

    DEBUG((DEBUG_INFO, "%a: Entry...\n", __FUNCTION__));

    for (;;) {
        //
        // Get the next handle
        //
        Status = gBS->LocateHandleBuffer (ByRegisterNotify,
                                         &gEfiSimpleFileSystemProtocolGuid,
                                          mFileSystemRegistration,
                                         &HandleCount,
                                         &HandleBuffer);
        //
        // If not found, or any other error, we're done
        //
        if (EFI_ERROR(Status)) {
          break;
        }

        // Spec says we only get one at a time using ByRegisterNotify
        ASSERT (HandleCount == 1);

        DEBUG((DEBUG_INFO,"%a: processing a potential log device\n", __FUNCTION__));

        RegisterLogDevice (HandleBuffer[0]);

        FreePool (HandleBuffer);
    }

    WriteLogFiles ();
}

/**
    ProcessFileSystemRegistration

    This function registers for FileSystemProtocols, and then
    checks for any existing FileSystemProtocols, and adds them
    to the LOG_DEVICE list.

    @param       VOID

    @retval      EFI_SUCCESS     FileSystem protocol registration successful
    @retval      error code      Something went wrong.

 **/
EFI_STATUS
ProcessFileSystemRegistration (
    VOID
    )
{
    EFI_EVENT       FileSystemCallBackEvent;
    EFI_HANDLE     *HandleBuffer;
    UINTN           HandleCount;
    UINTN           i;
    EFI_STATUS      Status;

    //
    // Always register for file system notifications.  They may arrive at any time.
    //
    DEBUG((DEBUG_INFO, "Registering for file systems notifications\n"));
    Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL,
                               TPL_CALLBACK,
                               OnFileSystemNotification,
                               NULL,
                              &FileSystemCallBackEvent);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: failed to create callback event (%r)\n", __FUNCTION__, Status));
        goto Cleanup;
    }

    Status = gBS->RegisterProtocolNotify (&gEfiSimpleFileSystemProtocolGuid,
                                           FileSystemCallBackEvent,
                                          &mFileSystemRegistration);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: failed to register for file system notifications (%r)\n", __FUNCTION__, Status));
        gBS->CloseEvent (FileSystemCallBackEvent);
        goto Cleanup;
    }

    //
    // Process any existing File System that were present before the registration.
    //
    Status = gBS->LocateHandleBuffer (ByProtocol,
                                     &gEfiSimpleFileSystemProtocolGuid,
                                      NULL,
                                     &HandleCount,
                                     &HandleBuffer);
    if (!EFI_ERROR(Status)) {
        for (i = 0; i < HandleCount; i++) {
            RegisterLogDevice (HandleBuffer[i]);
        }
        FreePool (HandleBuffer);
    }

    //
    // File Systems are not required at this time, so return SUCCESS.
    //
    Status = EFI_SUCCESS;

Cleanup:
    return Status;
}

/**
  ProcessRscHandlerRegistration

  Process the registration of the Report Status Code handler


  @parann       VOID

  @retval       EFI_SUCCESS     Processed successfully
  @retval       error           Unsuccessful at registring for Report Status Code events

 **/
EFI_STATUS
ProcessRscHandlerRegistration (
    VOID
    )
{
    EFI_EVENT       RscHandlerCallbackEvent;
    VOID           *RscRegistration;
    EFI_STATUS      Status;

    //
    // Try to get a pointer to the report status code protocol. If successful,
    // register our RSC handler here. Otherwise, register a protocol notify
    // handler and we'll register when the protocol is installed.
    //
    Status = gBS->LocateProtocol (&gEfiRscHandlerProtocolGuid,
                                   NULL,
                                   (VOID**)&mRscHandlerProtocol);

    if (!EFI_ERROR(Status)) {
        DEBUG((DEBUG_INFO, "%a: Located RSC handler protocol. Registering handler\n", __FUNCTION__));
        Status = mRscHandlerProtocol->Register (DxeLoggingBufferCaptureEvent, TPL_HIGH_LEVEL);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: failed to register RSC handler (%r)\n", __FUNCTION__, Status));
            goto Cleanup;
        }
    } else {
        DEBUG((DEBUG_INFO, "%a: RSC handler protocol not installed. Registering for notification\n", __FUNCTION__));
        Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL,
                                   TPL_CALLBACK,
                                   OnRscHandlerProtocolInstalled,
                                   NULL,
                                   &RscHandlerCallbackEvent);

        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: failed to create RSC handler protocol callback event (%r)\n", __FUNCTION__, Status));
            goto Cleanup;
        }

        Status = gBS->RegisterProtocolNotify (&gEfiRscHandlerProtocolGuid,
                                               RscHandlerCallbackEvent,
                                              &RscRegistration);

        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: failed to register for RSC handler protocol notification (%r)\n", __FUNCTION__, Status));
            gBS->CloseEvent (RscHandlerCallbackEvent);
        }
    }

Cleanup:
    return Status;
}

/**
   ProcessResetEventRegistration

   Process registration of ResetNotification

   @param   VOID

   @retval   EFI_SUCCESS     Registration successful
             error           Unsuccessful at registering for Reset Notifications

 **/
EFI_STATUS
ProcessResetEventRegistration (
    VOID
    )
{
    EFI_RESET_NOTIFICATION_PROTOCOL *ResetNotificationProtocol;
    EFI_EVENT                        ResetNotificationEvent;
    VOID                            *ResetNotificationRegistration;
    EFI_STATUS                       Status;


    //
    // Try to get a pointer to the report status code protocol. If successful,
    // register our RSC handler here. Otherwise, register a protocol notify
    // handler and we'll register when the protocol is installed.
    //
    Status = gBS->LocateProtocol (&gEfiResetNotificationProtocolGuid,
                                  NULL,
                                  (VOID**)&ResetNotificationProtocol);

    if (!EFI_ERROR(Status)) {
        //
        // Register our reset notification request
        //
        DEBUG((DEBUG_INFO, "%a: Located Reset notification protocol. Registering handler\n", __FUNCTION__));
        Status = ResetNotificationProtocol->RegisterResetNotify (ResetNotificationProtocol, OnResetNotification);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: failed to register Reset Notification handler (%r)\n", __FUNCTION__, Status));
        }
    } else {
        DEBUG((DEBUG_INFO, "%a: Reset Notification protocol not installed. Registering for notification\n", __FUNCTION__));
        Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL,
                                   TPL_CALLBACK,
                                   OnResetNotificationProtocolInstalled,
                                   NULL,
                                  &ResetNotificationEvent);

        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: failed to create Reset Protocol protocol callback event (%r)\n", __FUNCTION__, Status));
        } else {
            Status = gBS->RegisterProtocolNotify (&gEfiResetNotificationProtocolGuid,
                                                   ResetNotificationEvent,
                                                  &ResetNotificationRegistration);

            if (EFI_ERROR(Status)) {
                DEBUG((DEBUG_ERROR, "%a: failed to register for Reset Protocol notification (%r)\n", __FUNCTION__, Status));
                gBS->CloseEvent (ResetNotificationEvent);
            }
        }
    }

    return Status;
}

/**
    ProcessPostReadyToBootRegistration

    This function creates a group event handler for PostReadyToBoot


    @param    VOID

    @returns  EFI_SUCCESS   - Successfully registered for PostReadyToBoot
    @returns  other         - failure code from CreateEventEx

  **/
EFI_STATUS
ProcessPostReadyToBootRegistration (
    VOID
    )
{
    EFI_EVENT       InitEvent;
    EFI_STATUS      Status;

    //
    // Register notify function for writing the log files.
    //
    Status = gBS->CreateEventEx ( EVT_NOTIFY_SIGNAL,
                                  TPL_CALLBACK,
                                  OnPostReadyToBootNotification,
                                  gImageHandle,
                                 &gEfiEventPostReadyToBootGuid,
                                 &InitEvent );

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a - Create Event Ex for PostReadyToBoot. Code = %r\n", __FUNCTION__, Status));
    }

    return Status;
}

/**
    Main entry point for this driver.

    @param    ImageHandle     Image handle of this driver.
    @param    SystemTable     Pointer to the system table.

    @retval   EFI_STATUS      Initialization was successful.
    @retval   Others          The operation failed.

**/
EFI_STATUS
EFIAPI
DebugFileLoggerDxeEntry (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE    *SystemTable
    )
{
    EFI_STATUS      Status;


    DEBUG((DEBUG_INFO, "%a: VII enter...\n",  __FUNCTION__));

    //
    // Step 1. Initialize the debug logging buffer.
    //
    Status = LoggingBufferInit ();
    if (EFI_ERROR(Status)) {
        goto Exit;
    }

    //
    // Step 2. Register for file system notifications
    //
    Status = ProcessFileSystemRegistration ();
    if (EFI_ERROR(Status)) {
        goto Exit;
    }

    //
    // Step 3. Register for logging output
    //
    Status = ProcessRscHandlerRegistration ();
    if (EFI_ERROR(Status)) {
        goto Exit;
    }

    //
    // Step 4. Register for Reset Event
    //
    Status = ProcessResetEventRegistration ();
    if (EFI_ERROR(Status)) {
        goto Exit;
    }

    //
    // Step 5. Register for PostReadytoBoot Notifications.
    //
    Status = ProcessPostReadyToBootRegistration ();

Exit:
    DEBUG((DEBUG_INFO, "%a: Leaving, code = %r\n", __FUNCTION__, Status));

    // Always return EFI_SUCCESS.  This means any partial registration of functions,
    // and hooks in the system table, will still exist, reducing the complexity of
    // un-installation error cases.

    return EFI_SUCCESS;
}
