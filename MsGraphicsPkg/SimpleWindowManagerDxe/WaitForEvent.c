/** @file

  Simple Window Manger (SWM) implementation

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "WindowManager.h"


UINTN                             mResidualTimeout = 0;
EFI_EVENT                         mPowerOffTimerEvent = NULL;


/**
 * Wait for an event, and display the POWER OFF dialog if the Power Timer expires
 *
 * @param NumberOfEvents   The number of events the user is waiting on
 * @param Events           Array of events the user is waiting on
 * @param EventTypes       Array of event types the user is waiting on
 * @param Timeout          Time out (allows for refresh, etc) as long as < POWER OFF timer
 * @param ContinueTimer    Resume the Power Off delay as the previous wait event was just a refresh event
 *
 * @return SWM_EVENT_TYPE  The content of the event type array element of the event
 */
EFI_STATUS
EFIAPI
SWMWaitForEvent (
    IN UINTN           NumberOfEvents,
    IN EFI_EVENT      *Events,
    IN UINTN          *Index,
    IN UINT64          Timeout,
    IN BOOLEAN         ContinueTimer
    ) {

    return WaitForEventInternal (NumberOfEvents,
                                 Events,
                                 Index,
                                 Timeout,
                                 ContinueTimer
                                 );
}

/**
 * Wait for an event, and display the POWER OFF dialog if the Power Timer expires
 *
 * @param NumberOfEvents   The number of events the user is waiting on
 * @param Events           Array of events the user is waiting on
 * @param EventTypes       Array of event types the user is waiting on
 * @param Timeout          Time out (allows for refresh, etc) as long as < POWER OFF timer
 * @param ContinueTimer    Resume the Power Off delay as the previous wait event was just a refresh event
 *
 * @return EFI_STATUS      The return code from WaitForEvent
 */
EFI_STATUS
WaitForEventInternal (
    IN UINTN          NumberOfEvents,
    IN EFI_EVENT     *Events,
    IN UINTN         *Index,
    IN UINT64         TimeoutRequest,
    IN BOOLEAN        ContinueTimer
    ) {
    EFI_STATUS     Status;
    UINTN          EventNum;
#define MAX_SWM_WAIT_EVENTS  10
    EFI_EVENT      WaitList[MAX_SWM_WAIT_EVENTS];
    BOOLEAN        Restart;
    UINT64         Timeout;
    CHAR16        *PowerTitle;
    CHAR16        *PowerBody;
    CHAR16        *PowerCaption;
    SWM_MB_RESULT  SwmResult;

    // Validate caller parameters - leave space in the wait list for the
    // form refresh and time-out timer events.
    //
    if (NumberOfEvents > (MAX_SWM_WAIT_EVENTS - 2))
    {
        return EFI_INVALID_PARAMETER;
    }
    Restart = FALSE;
    do {
        Timeout = EFI_TIMER_PERIOD_SECONDS (PcdGet32 (PcdPowerOffDelay));

        if (mResidualTimeout == 0)
        {
            mResidualTimeout = Timeout;
        }
        if ((TimeoutRequest> 0) && (TimeoutRequest < Timeout))
        {  // timeouts are limited to the POWER timeout.
            Timeout = TimeoutRequest;
        }

        mResidualTimeout -= MIN (mResidualTimeout, Timeout);

        if (!ContinueTimer || (NULL == mPowerOffTimerEvent) || (TRUE == Restart)) {
            //
            //  Create the power off event (first time)
            //
            if (NULL == mPowerOffTimerEvent) {
                Status = gBS->CreateEvent(EVT_TIMER, 0, NULL, NULL, &mPowerOffTimerEvent);
                if (EFI_ERROR(Status)) {
                    DEBUG((DEBUG_ERROR,"Error creating power off timer event. Code = %r\n",Status));
                    goto Exit;
                }
                ContinueTimer = FALSE;  // No possible previous timer
            }
            //
            //  Normal waits will clear the current event, and reset a new power off delay.  If
            //  ContinueTimer is set, then just use the previous timer event.
            if ((!ContinueTimer) || (TRUE == Restart)) {
                Status = gBS->SetTimer(
                        mPowerOffTimerEvent,
                        TimerCancel,
                        0
                        );
                Restart = FALSE;
                gBS->CheckEvent(mPowerOffTimerEvent);  // Clear any pending signal
                //
                // Set the timer event
                //
                Status = gBS->SetTimer (
                    mPowerOffTimerEvent,
                    TimerRelative,
                    Timeout
                    );
                if (EFI_ERROR(Status)) {
                    DEBUG((DEBUG_ERROR,"Error setting power off timer event. Code = %r\n",Status));
                    goto Exit;
                }
            }
        }

        CopyMem (WaitList, Events, (NumberOfEvents * sizeof(EFI_EVENT)));
        EventNum = NumberOfEvents;

        WaitList[EventNum] = mPowerOffTimerEvent;
        EventNum++;

        Status = gBS->WaitForEvent (EventNum, WaitList, Index);

        if (!EFI_ERROR(Status ))
        {
            if (*Index == (EventNum - 1))    // Power Off timer or User Timer
            {
                if (mResidualTimeout == 0)   // Power Off timer
                {
                    DEBUG((DEBUG_INFO,"Displaying POWER OFF Dialog\n"));
                    PowerTitle = HiiGetString (mSWMHiiHandle, STRING_TOKEN (STR_POWER_TIMEOUT_TITLE), NULL);
                    PowerBody = HiiGetString (mSWMHiiHandle, STRING_TOKEN (STR_POWER_TIMEOUT_BODY), NULL);
                    PowerCaption = HiiGetString (mSWMHiiHandle, STRING_TOKEN (STR_POWER_TIMEOUT_CAPTION), NULL);
                    Status = SwmDialogsMessageBox(PowerTitle,        // Dialog body title.
                                                  PowerBody,         // Dialog body text.
                                                  PowerCaption,      // Dialog caption text.
                                                  SWM_MB_CANCEL | SWM_MB_STYLE_ALERT2, // Show Cancel and enforce Timeout
                                                  EFI_TIMER_PERIOD_SECONDS(PcdGet16(PcdPowerOffHold)),
                                                  &SwmResult);        // Return result.
                    if (!EFI_ERROR(Status))
                    {
                        if (SwmResult == SWM_MB_TIMEOUT)
                        {
                            DEBUG((DEBUG_ERROR,"Shutting down system due to Power Off Delay timer.\n"));
                            gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);
                        }
                        DEBUG((DEBUG_INFO,"Power Off delay canceled. Restarting wait\n"));
                        Restart = TRUE;  // Indicate restart of the Power Off timer.
                    }
                }
            }
        }
        else
        {
            DEBUG((DEBUG_ERROR,"Wait error - code=%r\n",Status));
        }

    } while (Restart);
    *Index = MIN (*Index, NumberOfEvents); // Normalize Index to events passed in.
Exit:
    return Status;
}
