/** @file -- TimeoutSpinner

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  TimeoutSpinner

**/

#include "TimeoutSpinner.h"

STATIC EFI_EVENT  gUpdateTimeoutSpinnerEvent;      // Event to Update the spinners on screen

SPINNER_CONTAINER  gAllSpinners[] = {
  // General Spinners
  { .Toc                  = NULL,
    .StartEventGuid       = &gGeneralSpinner1StartEventGroupGuid,
    .StopEventGuid        = &gGeneralSpinner1CompleteEventGroupGuid,
    .Id                   = 1,
    .IconFileToken        = PcdToken (PcdGeneral1File),
    .SpinnerTypeToken     = PcdToken (PcdGeneral1Type),
    .SpinnerLocationToken = PcdToken (PcdGeneral1Location) },
  { .Toc                  = NULL,
    .StartEventGuid       = &gGeneralSpinner2StartEventGroupGuid,
    .StopEventGuid        = &gGeneralSpinner2CompleteEventGroupGuid,
    .Id                   = 2,
    .IconFileToken        = PcdToken (PcdGeneral2File),
    .SpinnerTypeToken     = PcdToken (PcdGeneral2Type),
    .SpinnerLocationToken = PcdToken (PcdGeneral2Location) },
  { .Toc                  = NULL,
    .StartEventGuid       = &gGeneralSpinner3StartEventGroupGuid,
    .StopEventGuid        = &gGeneralSpinner3CompleteEventGroupGuid,
    .Id                   = 3,
    .IconFileToken        = PcdToken (PcdGeneral3File),
    .SpinnerTypeToken     = PcdToken (PcdGeneral3Type),
    .SpinnerLocationToken = PcdToken (PcdGeneral3Location) },
  { .Toc                  = NULL,
    .StartEventGuid       = &gGeneralSpinner4StartEventGroupGuid,
    .StopEventGuid        = &gGeneralSpinner4CompleteEventGroupGuid,
    .Id                   = 4,
    .IconFileToken        = PcdToken (PcdGeneral4File),
    .SpinnerTypeToken     = PcdToken (PcdGeneral4Type),
    .SpinnerLocationToken = PcdToken (PcdGeneral4Location) },

  // The NVMe spinner
  { .Toc                  = NULL,
    .StartEventGuid       = &gNVMeEnableStartEventGroupGuid,
    .StopEventGuid        = &gNVMeEnableCompleteEventGroupGuid,
    .Id                   = 5,
    .IconFileToken        = PcdToken (PcdGeneral5File),
    .SpinnerTypeToken     = PcdToken (PcdGeneral5Type),
    .SpinnerLocationToken = PcdToken (PcdGeneral5Location) }
};             // Array of Spinner Containers

/**
  UpdateSpinners

  Update/Draw Spinner  - Runs on refresh timer tick

  @param Event
  @param Context

  @return VOID EFIAPI
**/
VOID
EFIAPI
UpdateSpinners (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN              i;
  EFI_STATUS         Status;
  TIMEOUT_CONTAINER  *Toc;
  BOOLEAN            Updated;

  Updated = FALSE;
  for (i = 0; i < (sizeof (gAllSpinners) / sizeof (gAllSpinners[0])); i++) {
    Toc = gAllSpinners[i].Toc;

    if (NULL != Toc) {
      Updated = TRUE;
      // Start or update spinner
      Status = UpdateSpinnerGraphic (Toc);

      // On error shut down this spinner
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Error Updating Spinner\n"));
        EfiEventGroupSignal (gAllSpinners[i].StopEventGuid);
      }
    }
  }

  // if no timers active, turn off this timer tick.
  if (!Updated) {
    // Stop update timer if running
    Status = gBS->SetTimer (
                    gUpdateTimeoutSpinnerEvent,
                    TimerCancel,
                    0
                    );
  }
}

/**
  StartSpinnerCommon

  Start Spinner Common routine

  @param Spc         Spinner Container

  @return VOID
**/
VOID
StartSpinnerCommon (
  IN SPINNER_CONTAINER  *Spc
  )
{
  EFI_STATUS  Status;

  // Setup everything for spinner
  Status = SetupTimeoutContainer (Spc);

  // On error SetupTimeoutContainer has already freed all allocated pools, so just return
  if (EFI_ERROR (Status)) {
    return;
  }

  // Start the global spinner update timer -- OK if already started
  Status = gBS->SetTimer (
                  gUpdateTimeoutSpinnerEvent,
                  TimerPeriodic,
                  SPINNER_TICK_RATE
                  );

  // On error shut down this spinner
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error Starting Update Event\n"));
    EfiEventGroupSignal (Spc->StopEventGuid);
  }
}

/**
  StopSpinnerCommon

  Stop Spinner Common routine

  @param Spc

  @return VOID
**/
VOID
StopSpinnerCommon (
  IN SPINNER_CONTAINER  *Spc
  )
{
  // Delete spinner and free stored pixel buffer
  RestoreBackground (Spc->Toc);
  FreeSpinnerMemory (Spc->Toc);
  Spc->Toc = NULL;
}

/**
  TimeoutSpinnerEntry

  The Timeout Spinner driver entry point

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     Always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
TimeoutSpinnerEntry (
  IN    EFI_HANDLE        ImageHandle,
  IN    EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN              i;
  EFI_STATUS         Status;
  SPINNER_CONTAINER  *Spc;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

  // Common event for spinner updates
  //
  // Event used internally to timeout spinner
  // Will draw/update progress circle every Spinner Tick
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  UpdateSpinners,
                  NULL,
                  &gUpdateTimeoutSpinnerEvent
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error %r Creating Update Event\n", __FUNCTION__, Status));
    return EFI_SUCCESS;
  }

  for (i = 0; i < (sizeof (gAllSpinners) / sizeof (gAllSpinners[0])); i++) {
    Spc = &gAllSpinners[i];

    Status = InitializeGeneralSpinner (Spc);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Spinner[%d] failed to initialize\n", __FUNCTION__, Spc->Id));
    }
  }

  return EFI_SUCCESS;
}
