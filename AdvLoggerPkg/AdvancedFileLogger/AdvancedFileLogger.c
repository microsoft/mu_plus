/** @file AdvancedFileLogger.c

    This file contains functions for managing Log devices.

    Copyright (C) Microsoft Corporation. All rights reserved.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AdvancedFileLogger.h"

//
// Global variables.
//
VOID        *mFileSystemRegistration = NULL;
LIST_ENTRY  mLoggingDeviceHead       = INITIALIZE_LIST_HEAD_VARIABLE (mLoggingDeviceHead);
UINT32      mWritingSemaphore        = 0;

EFI_EVENT  mReadyToBootEvent      = NULL;
EFI_EVENT  mExitBootServicesEvent = NULL;

/**
    WriteLogFiles

    Write current log file to all of the logged file systems

  **/
VOID
WriteLogFiles (
  VOID
  )
{
  LIST_ENTRY  *Link;
  LOG_DEVICE  *LogDevice;
  UINT64      TimeEnd;
  UINT64      TimeStart;

  //
  // Use an atomic lock to catch a re-entrant call. Non-zero means we've entered
  // a second time.
  //
  DEBUG ((DEBUG_INFO, "Entry to WriteLogFiles.\n"));

  if (InterlockedCompareExchange32 (&mWritingSemaphore, 0, 1) != 0) {
    DEBUG ((DEBUG_ERROR, "WriteLogFiles blocked.\n"));
    return;
  }

  TimeStart = GetPerformanceCounter ();

  #define WRITING_ALL_LOG_FILES  "AdvLogger All files"

  PERF_INMODULE_BEGIN (WRITING_ALL_LOG_FILES);

  EFI_LIST_FOR_EACH (Link, &(mLoggingDeviceHead)) {
    LogDevice = LOG_DEVICE_FROM_LINK (Link);

    WriteALogFile (LogDevice);
  }

  PERF_INMODULE_END (WRITING_ALL_LOG_FILES);

  TimeEnd = GetPerformanceCounter ();
  DEBUG ((DEBUG_INFO, "Time to write logs: %ld ms\n", (GetTimeInNanoSecond (TimeEnd-TimeStart) / (1000 * 1000))));

  //
  // Release the lock.
  //
  InterlockedCompareExchange32 (&mWritingSemaphore, 1, 0);
  DEBUG ((DEBUG_INFO, "Exit from WriteLogFiles.\n"));
}

/**
    OnResetNotification

    Write the log files if the reset occurs as a reasonable TPL.

  **/
STATIC
VOID
EFIAPI
OnResetNotification (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  )
{
  EFI_TPL  OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (OldTpl);

  DEBUG ((DEBUG_INFO, "OnResetNotification\n"));
  if (OldTpl <= TPL_CALLBACK) {
    WriteLogFiles ();
  } else {
    DEBUG ((DEBUG_ERROR, "Unable to write log at reset\n"));
  }

  // Close ready to boot and exit boot services event.
  gBS->CloseEvent (mReadyToBootEvent);
  gBS->CloseEvent (mExitBootServicesEvent);

  return;
}

/**
    OnResetNotificationProtocolInstalled

    This function gets called when the Reset Notification
    protocol is installed.  Register for Reset Notifications.


    @param  Event   - The event used to trigger this callback
    @param  Context - NULL (no context is supplied)

    @returns          No return information

  **/
VOID
EFIAPI
OnResetNotificationProtocolInstalled (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_RESET_NOTIFICATION_PROTOCOL  *ResetNotificationProtocol;
  EFI_STATUS                       Status;

  DEBUG ((DEBUG_INFO, "OnResetNotification protocol detected\n"));
  //
  // Get a pointer to the report status code protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEdkiiPlatformSpecificResetFilterProtocolGuid,
                  NULL,
                  (VOID **)&ResetNotificationProtocol
                  );

  if (!EFI_ERROR (Status)) {
    //
    // Register our reset notification request
    //
    DEBUG ((DEBUG_INFO, "%a: Located Reset notification protocol. Registering handler\n", __func__));
    Status = ResetNotificationProtocol->RegisterResetNotify (ResetNotificationProtocol, OnResetNotification);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: failed to register Reset Notification handler (%r)\n", __func__, Status));
    }

    if (Event != NULL) {
      gBS->CloseEvent (Event);
    }
  } else {
    DEBUG ((DEBUG_ERROR, "%a: Unable to locate Reset Notification Protocol.\n", __func__));
  }

  return;
}

/**
    RegisterLogDevice

    Attempt to Enable Logging on this device.

    @param      Handle   - Handle that has the FileSystemProtocol installed

  **/
VOID
RegisterLogDevice (
  IN EFI_HANDLE  Handle
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  CHAR16                    *DevicePathString;
  LOG_DEVICE                *LogDevice;
  EFI_STATUS                Status;
  UINT64                    TimeStart;
  UINT64                    TimeEnd;

  TimeStart = GetPerformanceCounter ();

  LogDevice = (LOG_DEVICE *)AllocateZeroPool (sizeof (LOG_DEVICE));
  ASSERT (LogDevice);
  if (NULL == LogDevice) {
    DEBUG ((DEBUG_ERROR, "%a: Out of memory:\n", __func__));
    return;
  }

  LogDevice->Signature = LOG_DEVICE_SIGNATURE;
  LogDevice->Handle    = Handle;
  LogDevice->Valid     = TRUE;

  Status = EnableLoggingOnThisDevice (LogDevice);

  if (EFI_ERROR (Status)) {
    FreePool (LogDevice);
  } else {
    InsertTailList (&mLoggingDeviceHead, &LogDevice->Link);
    DevicePath       = DevicePathFromHandle (Handle);
    DevicePathString = ConvertDevicePathToText (DevicePath, TRUE, TRUE);
    if (DevicePathString != NULL) {
      DEBUG ((DEBUG_INFO, "File system registered on device:\n%s\n", DevicePathString));
      FreePool (DevicePathString);
    }
  }

  TimeEnd = GetPerformanceCounter ();
  DEBUG ((DEBUG_INFO, "Time to initialize logs: %ld ms\n\n", (GetTimeInNanoSecond (TimeEnd-TimeStart) / (1024 * 1024))));
}

/**
  New File System Discovered.

  Register this device as a possible UEFI Log device.

  @param    Event           Not Used.
  @param    Context         Not Used.

  @retval   none

  This may be called for multiple device arrivals, so the Event is not closed.

**/
VOID
EFIAPI
OnFileSystemNotification (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a: Entry...\n", __func__));

  for ( ; ;) {
    //
    // Get the next handle
    //
    Status = gBS->LocateHandleBuffer (
                    ByRegisterNotify,
                    NULL,
                    mFileSystemRegistration,
                    &HandleCount,
                    &HandleBuffer
                    );
    //
    // If not found, or any other error, we're done
    //
    if (EFI_ERROR (Status)) {
      break;
    }

    // Spec says we only get one at a time using ByRegisterNotify
    ASSERT (HandleCount == 1);

    DEBUG ((DEBUG_INFO, "%a: processing a potential log device on handle %p\n", __func__, HandleBuffer[0]));

    RegisterLogDevice (HandleBuffer[0]);

    FreePool (HandleBuffer);
  }

  WriteLogFiles ();
}

/**
    Write the log files on request.

    @param    Event           Not Used.
    @param    Context         Not Used.

    @retval   none

    This may be called for multiple WriteLogNotifications, so the Event is not closed.

 **/
VOID
EFIAPI
OnWriteLogNotification (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  WriteLogFiles ();
}

/**
    Write the log files at ReadyToBoot.

    @param    Event           Not Used.
    @param    Context         Not Used.

   @retval   none

    This may be called for multiple ReadyToBoot notifications, so the Event is not closed.

 **/
VOID
EFIAPI
OnReadyToBootNotification (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  WriteLogFiles ();
}

/**
    Write the log files at ExitBootServices.

    @param    Event           Not Used.
    @param    Context         Not Used.

    @retval   none

    There is only one call to ExitBootServices, so close the Event.

 **/
VOID
EFIAPI
OnPreExitBootServicesNotification (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
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
  EFI_EVENT   FileSystemCallBackEvent;
  EFI_STATUS  Status;

  //
  // Always register for file system notifications.  They may arrive at any time.
  //
  DEBUG ((DEBUG_INFO, "Registering for file systems notifications\n"));
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnFileSystemNotification,
                  NULL,
                  &FileSystemCallBackEvent
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to create callback event (%r)\n", __func__, Status));
    goto Cleanup;
  }

  Status = gBS->RegisterProtocolNotify (
                  &gEfiSimpleFileSystemProtocolGuid,
                  FileSystemCallBackEvent,
                  &mFileSystemRegistration
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to register for file system notifications (%r)\n", __func__, Status));
    gBS->CloseEvent (FileSystemCallBackEvent);
    goto Cleanup;
  }

  //
  // Process any existing File System that were present before the registration.
  //
  OnFileSystemNotification (FileSystemCallBackEvent, NULL);

Cleanup:
  return Status;
}

/**
   ProcessResetEventRegistration

   Process registration of Reset Notification

   @param    VOID

   @retval   EFI_SUCCESS     Registration successful
             error           Unsuccessful at registering for Reset Notifications

 **/
EFI_STATUS
ProcessResetEventRegistration (
  VOID
  )
{
  EFI_RESET_NOTIFICATION_PROTOCOL  *ResetNotificationProtocol;
  EFI_EVENT                        ResetNotificationEvent;
  VOID                             *ResetNotificationRegistration;
  EFI_STATUS                       Status;

  //
  // Try to get a pointer to the Reset Notification Protocol. If successful,
  // register our reset handler here. Otherwise, register a protocol notify
  // handler and we'll register when the protocol is installed.
  //
  Status = gBS->LocateProtocol (
                  &gEdkiiPlatformSpecificResetFilterProtocolGuid,
                  NULL,
                  (VOID **)&ResetNotificationProtocol
                  );

  if (!EFI_ERROR (Status)) {
    //
    // Register our reset notification request
    //
    DEBUG ((DEBUG_INFO, "%a: Located Reset notification protocol. Registering handler\n", __func__));
    Status = ResetNotificationProtocol->RegisterResetNotify (ResetNotificationProtocol, OnResetNotification);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: failed to register Reset Notification handler (%r)\n", __func__, Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "%a: Reset Notification protocol not installed. Registering for notification\n", __func__));
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    OnResetNotificationProtocolInstalled,
                    NULL,
                    &ResetNotificationEvent
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: failed to create Reset Protocol protocol callback event (%r)\n", __func__, Status));
    } else {
      Status = gBS->RegisterProtocolNotify (
                      &gEdkiiPlatformSpecificResetFilterProtocolGuid,
                      ResetNotificationEvent,
                      &ResetNotificationRegistration
                      );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: failed to register for Reset Protocol notification (%r)\n", __func__, Status));
        gBS->CloseEvent (ResetNotificationEvent);
      }
    }
  }

  return Status;
}

/**
    ProcessSyncRequestRegistration

    This function creates a group event handler for gAdvancedLoggerWriteLogFiles, which
    allows for new functions to decide when the in memory log should be written to the disk.


    @param    VOID

    @retval   EFI_SUCCESS     Registration successful

  **/
EFI_STATUS
ProcessSyncRequestRegistration (
  VOID
  )
{
  EFI_EVENT   InitEvent;
  EFI_STATUS  Status;

  //
  // Register notify function for writing the log files.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnWriteLogNotification,
                  gImageHandle,
                  &gAdvancedFileLoggerWriteLogFiles,
                  &InitEvent
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Create Event Ex for file logger write. Code = %r\n", __func__, Status));
  }

  return Status;
}

/**
    ProcessReadyToBootRegistration

    This function creates a group event handler for ReadyToBoot to flush the
    log file to media/


    @param    VOID

    @returns  nothing

  **/
EFI_STATUS
ProcessReadyToBootRegistration (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT8       FlushFlags;

  FlushFlags = FixedPcdGet8 (PcdAdvancedFileLoggerFlush);

  Status = EFI_SUCCESS;
  if (FlushFlags & ADV_PCD_FLUSH_TO_MEDIA_FLAGS_READY_TO_BOOT) {
    //
    // Register notify function for writing the log files.
    //
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    OnReadyToBootNotification,
                    gImageHandle,
                    &gEfiEventReadyToBootGuid,
                    &mReadyToBootEvent
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Create Event Ex for ReadyToBoot. Code = %r\n", __func__, Status));
    }
  }

  return Status;
}

/**
    ProcessPreExitBootServicesRegistration

    This function creates a group event handler for gAdvancedLoggerWriteLogFiles


    @param    VOID

    @returns  nothing

  **/
EFI_STATUS
ProcessPreExitBootServicesRegistration (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT8       FlushFlags;

  FlushFlags = FixedPcdGet8 (PcdAdvancedFileLoggerFlush);

  Status = EFI_SUCCESS;
  if (FlushFlags & ADV_PCD_FLUSH_TO_MEDIA_FLAGS_EXIT_BOOT_SERVICES) {
    //
    // Register notify function for writing the log files.
    //
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    OnPreExitBootServicesNotification,
                    gImageHandle,
                    &gMuEventPreExitBootServicesGuid,
                    &mExitBootServicesEvent
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Create Event Ex for ExitBootServices. Code = %r\n", __func__, Status));
    }
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
AdvancedFileLoggerEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                   Status;
  ADVANCED_FILE_LOGGER_POLICY  AdvFileLoggerPolicy;
  UINT16                       PolicySize = ADVANCED_FILE_LOGGER_POLICY_SIZE;

  DEBUG ((DEBUG_INFO, "%a: enter...\n", __func__));

  // Step 0. Check advanced file logger policy, default to enabled.

  Status = GetPolicy (&gAdvancedFileLoggerPolicyGuid, NULL, (VOID *)&AdvFileLoggerPolicy, &PolicySize);
  if (EFI_ERROR (Status)) {
    // If we fail to get policy, default to enabled.
    DEBUG ((DEBUG_WARN, "%a: Unable to get file logger - %r defaulting to enabled!\n", __func__, Status));
  } else if (AdvFileLoggerPolicy.FileLoggerEnable == FALSE) {
    DEBUG ((DEBUG_INFO, "%a: File logger disabled in policy, exiting.\n", __func__));
    return EFI_SUCCESS;
  } else {
    DEBUG ((DEBUG_INFO, "%a: File logger enabled in policy.\n", __func__));
  }

  //
  // Step 1. Register for file system notifications
  //
  Status = ProcessFileSystemRegistration ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Step 2. Register for Reset Event
  //
  Status = ProcessResetEventRegistration ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Step 3. Register for general flush log files Notifications.
  //
  Status = ProcessSyncRequestRegistration ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Step 4. Register for ReadyToBoot Notifications.
  //
  Status = ProcessReadyToBootRegistration ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Step 5. Register for PreExitBootServices Notifications.
  //
  Status = ProcessPreExitBootServicesRegistration ();

Exit:

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Leaving, code = %r\n", __func__, Status));
  } else {
    DEBUG ((DEBUG_INFO, "%a: Leaving, code = %r\n", __func__, Status));
  }

  // Always return EFI_SUCCESS.  This means any partial registration of functions
  // will still exist, reducing the complexity of the uninstall process after a partial
  // install.

  return EFI_SUCCESS;
}
