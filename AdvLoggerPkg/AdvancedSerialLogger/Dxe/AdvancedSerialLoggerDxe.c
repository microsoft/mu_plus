/** @file AdvancedFileLogger.c

    This file contains functions for managing Log devices.

    Copyright (C) Microsoft Corporation. All rights reserved.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AdvancedSerialLoggerDxe.h"

#define ADV_LOG_REFRESH_INTERVAL  (200 * 10 * 1000)   // Refresh interval: 200ms in 100ns units
#define ADV_LOG_MESSAGES_PER_EVENT 1000

//
// Global variables.
//
STATIC ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY  mAccessEntry;
STATIC EFI_EVENT                                  mWriteToSerialPortTimerEvent = NULL;
STATIC EFI_EVENT                                  mExitBootServicesEvent = NULL;
STATIC EFI_EVENT                                  mResetNotificationEvent = NULL;
STATIC EFI_RESET_NOTIFICATION_PROTOCOL           *mResetNotificationProtocol = NULL;
STATIC ADVANCED_LOGGER_INFO                      *mLoggerInfo;

/**
  WriteToSerialPort

  Writes the currently unwritten part of the log to the serial port.


  @retval  EFI_SUCCESS      The log was updated
  @retval  other            An error occurred.  The log device was disabled

  **/
VOID
WriteToSerialPort (
    IN UINTN MaxLineCount
  )
{
    UINTN               LineCount;
    EFI_STATUS          Status;
    UINTN               WriteSize;

#if 0

    // Currently, this is only a DXE driver, so all logging will end
    // at EXIT BOOT SERVICES by default.

    if (mLoggerInfo != NULL) {
        DisableFlags = FixedPcdGet8(PcdAdvancedSerialLoggerDisable);
        if (DisableFlags & ADV_PCD_DISABLE_SERIAL_FLAGS_EXIT_BOOT_SERVICES) {
            if (mLoggerInfo->AtRuntime) {
                return;
            }
        }

        if (DisableFlags & ADV_PCD_DISABLE_SERIAL_FLAGS_VIRTUAL_ADDRESS_CHANGE) {
            if (mLoggerInfo->GoneVirtual) {
                return;
            }
        }
    }
#endif

    LineCount = 0;
    Status = AdvancedLoggerAccessLibGetNextFormattedLine (&mAccessEntry);
    while ((Status == EFI_SUCCESS) && (LineCount < MaxLineCount)) {

        WriteSize = mAccessEntry.MessageLen;
        if (WriteSize > 0) {
            // Only selected messages go to the serial port.

            if (mAccessEntry.DebugLevel & PcdGet32(PcdAdvancedSerialLoggerDebugPrintErrorLevel)) {
                Status = SerialPortWrite((UINT8 *) mAccessEntry.Message, mAccessEntry.MessageLen);
                if (EFI_ERROR(Status)) {
                    DEBUG((DEBUG_ERROR, "%a: Failed to write to serial port: %r\n", __FUNCTION__, Status));
                    break;
                }
            }
        }

        Status = AdvancedLoggerAccessLibGetNextFormattedLine (&mAccessEntry);
        LineCount++;
    }

    return;
};

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

    WriteToSerialPort (MAX_UINTN);

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
    IN  EFI_EVENT   Event,
    IN  VOID        *Context
    )
{
    EFI_STATUS                       Status;

    DEBUG((DEBUG_INFO, "OnResetNotification protocol detected\n"));
    //
    // Get a pointer to the report status code protocol.
    //
    Status = gBS->LocateProtocol (&gEfiResetNotificationProtocolGuid,
                                   NULL,
                                   (VOID**)&mResetNotificationProtocol);

    if (!EFI_ERROR(Status)) {
        //
        // Register our reset notification request
        //
        DEBUG((DEBUG_INFO, "%a: Located Reset notification protocol. Registering handler\n", __FUNCTION__));
        Status = mResetNotificationProtocol->RegisterResetNotify (mResetNotificationProtocol, OnResetNotification);
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
    Write the log on certain time intervals.

    @param    Event           Not Used.
    @param    Context         Not Used.

    @retval   none

    This may be called for multiple WriteLogNotifications, so the Event is not closed.

 **/
VOID
EFIAPI
OnWriteSerialTimerCallback (
    IN EFI_EVENT        Event,
    IN VOID             *Context
  )
{

    WriteToSerialPort (ADV_LOG_MESSAGES_PER_EVENT);
}

/**
    Write the log files at PreExitBootServices.

    @param    Event           Not Used.
    @param    Context         Not Used.

    @retval   none

    There is only one call to PreExitBootServices, so close the Event.

 **/
VOID
EFIAPI
OnExitBootServicesNotification (
    IN EFI_EVENT        Event,
    IN VOID             *Context
  )
{

    WriteToSerialPort (MAX_UINTN);

    gBS->CloseEvent (Event);
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
    VOID                            *ResetNotificationRegistration;
    EFI_STATUS                       Status;


    //
    // Try to get a pointer to the Reset Notification Protocol. If successful,
    // register our reset handler here. Otherwise, register a protocol notify
    // handler and we'll register when the protocol is installed.
    //
    Status = gBS->LocateProtocol (&gEfiResetNotificationProtocolGuid,
                                  NULL,
                                  (VOID**)&mResetNotificationProtocol);

    if (!EFI_ERROR(Status)) {
        //
        // Register our reset notification request
        //
        DEBUG((DEBUG_INFO, "%a: Located Reset notification protocol. Registering handler\n", __FUNCTION__));
        Status = mResetNotificationProtocol->RegisterResetNotify (mResetNotificationProtocol, OnResetNotification);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: failed to register Reset Notification handler (%r)\n", __FUNCTION__, Status));
        }
    } else {
        DEBUG((DEBUG_INFO, "%a: Reset Notification protocol not installed. Registering for notification\n", __FUNCTION__));
        Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL,
                                   TPL_CALLBACK,
                                   OnResetNotificationProtocolInstalled,
                                   NULL,
                                  &mResetNotificationEvent);

        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: failed to create Reset Protocol protocol callback event (%r)\n", __FUNCTION__, Status));
        } else {
            Status = gBS->RegisterProtocolNotify (&gEfiResetNotificationProtocolGuid,
                                                   mResetNotificationEvent,
                                                  &ResetNotificationRegistration);

            if (EFI_ERROR(Status)) {
                DEBUG((DEBUG_ERROR, "%a: failed to register for Reset Protocol notification (%r)\n", __FUNCTION__, Status));
                gBS->CloseEvent (mResetNotificationEvent);
            }
        }
    }

    return Status;
}

/**
Process Timer registration.

This function creates a group event handler for writing to the serial port on Timer Ticks


@param    VOID

@returns  nothing

**/
EFI_STATUS
ProcessTimerRegistration (
    VOID
  )
{
EFI_STATUS      Status;

//
// Create a timer event to regularly sample active surface frames and confirm someone hasn't used the framebuffer pointer directly to step on the surface.
//
Status = gBS->CreateEvent (EVT_TIMER | EVT_NOTIFY_SIGNAL,
                           TPL_CALLBACK,
                           OnWriteSerialTimerCallback,
                           NULL,
                           &mWriteToSerialPortTimerEvent
                          );
if (EFI_ERROR (Status))
{
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Failed to create timer event for sampling surface frame (%r).\n", __FUNCTION__, Status));
    goto Exit;
}

// Start a periodic timer to sample active surface frames.
//
Status = gBS->SetTimer (mWriteToSerialPortTimerEvent,
                        TimerPeriodic,
                        ADV_LOG_REFRESH_INTERVAL
                       );

if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: Failed to create timer event for writing log data (%r).\n", __FUNCTION__, Status));
}

Exit:
return Status;
}

/**
    ProcessPreExitBootServicesRegistration

    This function creates a group event handler for gAdvancedLoggerWriteLogFiles


    @param    VOID

    @returns  nothing

  **/
EFI_STATUS
ProcessExitBootServicesRegistration (
    VOID
    )
{
    EFI_STATUS      Status;

    //
    // Register notify function for writing the log files.
    //
    // - Note: Nonstandard TPL used in order to be the last running item in the ExitBootServices
    //         callback list to insure log messages are written to the the log.
    Status = gBS->CreateEventEx ( EVT_NOTIFY_SIGNAL,
                                 (TPL_APPLICATION + 1),
                                  OnExitBootServicesNotification,
                                  gImageHandle,
                                 &gEfiEventExitBootServicesGuid,
                                 &mExitBootServicesEvent );

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a - Create Event Ex for ExitBootServices. Code = %r\n", __FUNCTION__, Status));
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
AdvancedSerialLoggerEntry (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE    *SystemTable
    )
{
    ADVANCED_LOGGER_PROTOCOL  *LoggerProtocol;
    EFI_STATUS                 Status;

    DEBUG((DEBUG_INFO, "%a: enter...\n",  __FUNCTION__));

    Status = gBS->LocateProtocol (&gAdvancedLoggerProtocolGuid,
                                   NULL,
                                  (VOID **) &LoggerProtocol);
    if (EFI_ERROR(Status)) {
        goto Exit;
    }

    SerialPortInitialize();

    mLoggerInfo = (ADVANCED_LOGGER_INFO *) LoggerProtocol->Context;

    //
    // Step 1 - Start the first group of messages
    //
    WriteToSerialPort (ADV_LOG_MESSAGES_PER_EVENT);

    //
    // Step 2 - Register for timer events
    //
    Status = ProcessTimerRegistration ();
    if (EFI_ERROR(Status)) {
        goto Exit;
    }

    //
    // Step 3 - Register for ExitBootServices
    //
    Status = ProcessExitBootServicesRegistration ();
    if (EFI_ERROR(Status)) {
        goto Exit;
    }

    //
    // Step 4. Register for Reset Event
    //
    Status = ProcessResetEventRegistration ();

Exit:

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: Leaving, code = %r\n", __FUNCTION__, Status));

        if (mWriteToSerialPortTimerEvent != NULL) {
            gBS->CloseEvent (mWriteToSerialPortTimerEvent);
        }

        if (mExitBootServicesEvent != NULL) {
            gBS->CloseEvent (mExitBootServicesEvent);
        }

        if (mResetNotificationProtocol != NULL) {
             mResetNotificationProtocol->UnregisterResetNotify (mResetNotificationProtocol, OnResetNotification);
        }

        if (mResetNotificationEvent != NULL) {
            gBS->CloseEvent (mResetNotificationEvent);
        }

    } else {
        DEBUG((DEBUG_INFO, "%a: Leaving, code = %r\n", __FUNCTION__, Status));
    }

    // Always return EFI_SUCCESS.  This means any partial registration of functions
    // will still exist, reducing the complexity of the uninstall process after a partial
    // install.

    return EFI_SUCCESS;
}
