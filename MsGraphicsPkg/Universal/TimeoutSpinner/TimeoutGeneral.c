/** @file TimeoutGeneral.c

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TimeoutSpinner.h"

/**
  DelayStartGeneralSpinner

  External Event Handler to the Start a delayed General Spinner

  @param Event
  @param Context

  @return VOID EFIAPI
**/
VOID
EFIAPI
DelayStartGeneralSpinner (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  SPINNER_CONTAINER  *Spc;

  DEBUG ((DEBUG_INFO, "%a Entered.\n", __FUNCTION__));
  Spc = (SPINNER_CONTAINER *)Context;
  StartSpinnerCommon (Spc);
}

/**
  StartGeneralSpinner

  External Event Handler to the Start a General Spinner

  @param Event
  @param Context

  @return VOID EFIAPI
**/
VOID
EFIAPI
StartGeneralSpinner (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  SPINNER_CONTAINER  *Spc;
  EFI_STATUS         Status;

  DEBUG ((DEBUG_INFO, "%a: Entered.\n", __FUNCTION__));
  Spc = (SPINNER_CONTAINER *)Context;

  Spc->Icon     = LibPcdGetExPtr (&gMsGraphicsPkgTokenSpaceGuid, Spc->IconFileToken);
  Spc->Type     = LibPcdGetEx8 (&gMsGraphicsPkgTokenSpaceGuid, Spc->SpinnerTypeToken);
  Spc->Location = LibPcdGetEx8 (&gMsGraphicsPkgTokenSpaceGuid, Spc->SpinnerLocationToken);

  if ((Spc->Icon == NULL) ||
      ((Spc->Type < Standard) || (Spc->Type > Delay)) ||
      ((Spc->Location < Location_LR_Corner) || (Spc->Location > Location_Center)))
  {
    DEBUG ((DEBUG_ERROR, "%a: Spinner[%d] invalid\n", __FUNCTION__, Spc->Id));
    return;
  }

  if (IsZeroGuid (Spc->Icon)) {
    DEBUG ((DEBUG_INFO, "%a: Spinner[%d] icon not set\n", __FUNCTION__, Spc->Id));
    return;
  }

  if (Spc->Type == Delay) {
    // Start countdown timer to delay spinner display
    Status = gBS->CreateEvent (
                    EVT_TIMER | EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    DelayStartGeneralSpinner,
                    Spc,
                    &Spc->DelayEvent
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Error %r creating delay timer for Spinner(%d)\n", __FUNCTION__, Status, Spc->Id));
    }

    Status = gBS->SetTimer (
                    Spc->DelayEvent,
                    TimerRelative,
                    TIME_TO_SPINNER
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Error %r setting timer. Event = %lx\n", __FUNCTION__, Status, Spc->DelayEvent));
    }
  } else {
    StartSpinnerCommon (Spc);
  }
}

/**
  StopGeneralSpinner

  External Event Handler to Stop a Spinner

  @param Event
  @param Context

  @return VOID EFIAPI
**/
VOID
EFIAPI
StopGeneralSpinner (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  SPINNER_CONTAINER  *Spc;
  EFI_STATUS         Status;

  DEBUG ((DEBUG_INFO, "%a Entered.\n", __FUNCTION__));

  Spc = (SPINNER_CONTAINER *)Context;

  if ((Spc->Type == Delay) && (Spc->DelayEvent != NULL)) {
    //
    // Stop countdown timer if running
    //
    Status = gBS->SetTimer (
                    Spc->DelayEvent,
                    TimerCancel,
                    0
                    );

    gBS->CloseEvent (Spc->DelayEvent);
    Spc->DelayEvent = NULL;

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Error %r Stopping Countdown Timer for spinner(%d)\n", __FUNCTION__, Status, Spc->Id));
    }
  }

  Spc->DelayEvent = NULL;

  StopSpinnerCommon (Spc);
}

/**
  InitializeGeneralSpinner

  Initialize one of the General Spinners

  @param  Spc        - Spinner Containter

  @return EFI_STATUS - Spinner initialized
          other      - Spinner not initialized
**/
EFI_STATUS
InitializeGeneralSpinner (
  IN SPINNER_CONTAINER  *Spc
  )
{
  EFI_STATUS  Status;

  // 1. External event used to initialize and start this General Spinner

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  StartGeneralSpinner,
                  Spc,
                  Spc->StartEventGuid,
                  &Spc->StartEvent
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error %r Creating Start Spinner(%d) Event\n", __FUNCTION__, Status, Spc->Id));
    return Status;
  }

  // 2. External event to stop this general spinner, restore the screen,
  //    and free the general spinner memory

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  StopGeneralSpinner,
                  Spc,
                  Spc->StopEventGuid,
                  &Spc->StopEvent
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error %r Creating Stop Spinner(%d) Event\n", __FUNCTION__, Status, Spc->Id));
    return Status;
  }

  return Status;
}
