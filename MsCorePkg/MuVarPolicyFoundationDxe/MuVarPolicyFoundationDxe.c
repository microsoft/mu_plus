/** @file -- MuVarPolicyFoundationDxe.c
This DXE driver will publish policies and state variables to support a couple of different design patterns:
- Locking policies and performing other tests based on whether a DXE phase has passed
    - EndOfDxe
    - ReadyToBoot
    - ExitBootServices
- Setting up a reference variable that can only be written once that can be used in other variable policies

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/VariablePolicy.h>

#include <Library/VariablePolicyHelperLib.h>

#include <Guid/EventGroup.h>
#include <Guid/MuVarPolicyFoundationDxe.h>

EFI_EVENT  mEndOfDxeEvent;
EFI_EVENT  mReadyToBootEvent;
EFI_EVENT  mExitBootServicesEvent;

BOOLEAN  mEndOfDxeIndicatorSet         = FALSE;
BOOLEAN  mReadyToBootIndicatorSet      = FALSE;
BOOLEAN  mExitBootServicesIndicatorSet = FALSE;

STATIC EDKII_VARIABLE_POLICY_PROTOCOL  *mVariablePolicy = NULL;

/**
  Creates an indicator variable with the supplied attributes.

  @param[in]  IndicatorName         The variable name of the indicator being set.
  @param[in]  IndicatorAttributes   The attributes to apply to the indicator variable.

  @retval     EFI_SUCCESS             Indicator was created.
  @retval     EFI_OUT_OF_RESOURCES    Indicator could not be created.

**/
STATIC
EFI_STATUS
SetPhaseIndicator (
  IN CHAR16  *IndicatorName,
  IN UINT32  IndicatorAttributes
  )
{
  EFI_STATUS       Status    = EFI_SUCCESS;
  PHASE_INDICATOR  Indicator = PHASE_INDICATOR_SET;

  DEBUG ((DEBUG_VERBOSE, "%a - Setting indicator '%s'...\n", __FUNCTION__, IndicatorName));

  //
  // Attempt to create the variable.
  Status = gRT->SetVariable (
                  IndicatorName,
                  &gMuVarPolicyDxePhaseGuid,
                  IndicatorAttributes,
                  sizeof (Indicator),
                  &Indicator
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Error creating indicator! %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
    Status = EFI_OUT_OF_RESOURCES;    // Set the correct return value.
    // TODO VARPOL: Telemetry.
  }

  return Status;
} // SetPhaseIndicator()

/**
  EndOfDxe Callback
  Create the indicator variable and lock it.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context

**/
STATIC
VOID
EFIAPI
SetEndOfDxeIndicator (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = SetPhaseIndicator (END_OF_DXE_INDICATOR_VAR_NAME, END_OF_DXE_INDICATOR_VAR_ATTR);

  // If successful, close the event so it doesn't get signalled repeatedly.
  if (!EFI_ERROR (Status)) {
    mEndOfDxeIndicatorSet = TRUE;
    gBS->CloseEvent (mEndOfDxeEvent);
  }
} // SetEndOfDxeIndicator()

/**
  ReadyToBoot Callback
  Create the indicator variable and lock it.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context

**/
STATIC
VOID
EFIAPI
SetReadyToBootIndicator (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = SetPhaseIndicator (READY_TO_BOOT_INDICATOR_VAR_NAME, READY_TO_BOOT_INDICATOR_VAR_ATTR);

  // If successful, close the event so it doesn't get signalled repeatedly.
  if (!EFI_ERROR (Status)) {
    // If EndOfDxe was never signalled, make a last-ditch effort to signal it.
    if (!mEndOfDxeIndicatorSet) {
      SetEndOfDxeIndicator (Event, Context);
    }

    mReadyToBootIndicatorSet = TRUE;
    gBS->CloseEvent (mReadyToBootEvent);
  }
} // SetReadyToBootIndicator()

/**
  ExitBootServices Callback
  Create the indicator variable and lock it.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context

**/
STATIC
VOID
EFIAPI
SetExitBootServicesIndicator (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = SetPhaseIndicator (EXIT_BOOT_SERVICES_INDICATOR_VAR_NAME, EXIT_BOOT_SERVICES_INDICATOR_VAR_ATTR);

  // If successful, close the event so it doesn't get signalled repeatedly.
  if (!EFI_ERROR (Status)) {
    // If EndOfDxe was never signalled, make a last-ditch effort to signal it.
    if (!mEndOfDxeIndicatorSet) {
      SetEndOfDxeIndicator (Event, Context);
    }

    // If ReadyToBoot was never signalled, make a last-ditch effort to signal it.
    if (!mReadyToBootIndicatorSet) {
      SetReadyToBootIndicator (Event, Context);
    }

    mExitBootServicesIndicatorSet = TRUE;
    gBS->CloseEvent (mExitBootServicesEvent);
  }
} // SetExitBootServicesIndicator()

/**
  The driver's entry point.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
MuVarPolicyFoundationDxeMain (
  IN    EFI_HANDLE        ImageHandle,
  IN    EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  FinalStatus;
  EFI_STATUS  PolicyStatus;
  EFI_STATUS  EndOfDxeStatus;
  EFI_STATUS  ReadyToBootStatus;
  EFI_STATUS  ExitBootServicesStatus;

  FinalStatus = EFI_SUCCESS;

  DEBUG ((DEBUG_VERBOSE, "%a()\n", __FUNCTION__));

  //
  // First, make sure that we can locate and set the required policy.
  PolicyStatus = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID **)&mVariablePolicy);
  if (EFI_ERROR (PolicyStatus)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to locate VariablePolicy protocol! %r\n", __FUNCTION__, PolicyStatus));
  }

  if (!EFI_ERROR (PolicyStatus)) {
    // IMPORTANT NOTE: On the whole, it is a *bad* idea to use LOCK_ON_CREATE for a namespace policy.
    //                 However, since these are are all forced to be Volatile variables and since you can't create
    //                 volatile variables after ExitBootServices (and the variables will disappear on reboot),
    //                 this isn't the end of the world.
    PolicyStatus = RegisterBasicVariablePolicy (
                     mVariablePolicy,                                                   // VariablePolicy
                     &gMuVarPolicyDxePhaseGuid,                                         // Namespace
                     NULL,                                                              // Name
                     sizeof (PHASE_INDICATOR),                                          // MinSize
                     sizeof (PHASE_INDICATOR),                                          // MaxSize
                     DXE_PHASE_INDICATOR_ATTR,                                          // AttributesMustHave
                     (UINT32)(~DXE_PHASE_INDICATOR_ATTR),                               // AttributesCantHave
                     VARIABLE_POLICY_TYPE_LOCK_ON_CREATE
                     );                                                                 // LockPolicyType
    if (EFI_ERROR (PolicyStatus)) {
      DEBUG ((DEBUG_ERROR, "%a - Failed to register DxePhase state var policy! %r\n", __FUNCTION__, PolicyStatus));
    }
  }

  // Register a policy to describe the write-once state variable namespace.
  if (!EFI_ERROR (PolicyStatus)) {
    PolicyStatus = RegisterBasicVariablePolicy (
                     mVariablePolicy,                                                   // VariablePolicy
                     &gMuVarPolicyWriteOnceStateVarGuid,                                // Namespace
                     NULL,                                                              // Name
                     sizeof (POLICY_LOCK_VAR),                                          // MinSize
                     sizeof (POLICY_LOCK_VAR),                                          // MaxSize
                     WRITE_ONCE_STATE_VAR_ATTR,                                         // AttributesMustHave
                     (UINT32)(~WRITE_ONCE_STATE_VAR_ATTR),                              // AttributesCantHave
                     VARIABLE_POLICY_TYPE_LOCK_ON_CREATE
                     );                                                                 // LockPolicyType
    if (EFI_ERROR (PolicyStatus)) {
      DEBUG ((DEBUG_ERROR, "%a - Failed to register WriteOnce state var policy! %r\n", __FUNCTION__, PolicyStatus));
    }
  }

  if (!EFI_ERROR (PolicyStatus)) {
    //
    // Register EndOfDxe callback.
    EndOfDxeStatus = gBS->CreateEventEx (
                            EVT_NOTIFY_SIGNAL,
                            TPL_CALLBACK - 1,                       // At end of EndOfDxe
                            SetEndOfDxeIndicator,
                            NULL,
                            &gEfiEndOfDxeEventGroupGuid,
                            &mEndOfDxeEvent
                            );
    if (EFI_ERROR (EndOfDxeStatus)) {
      DEBUG ((DEBUG_ERROR, "%a - EndOfDxe callback registration failed! %r\n", __FUNCTION__, EndOfDxeStatus));
    }

    //
    // Register ReadyToBoot callback.
    ReadyToBootStatus = gBS->CreateEventEx (
                               EVT_NOTIFY_SIGNAL,
                               TPL_CALLBACK - 1,                    // At end of ReadyToBoot
                               SetReadyToBootIndicator,
                               NULL,
                               &gEfiEventReadyToBootGuid,
                               &mReadyToBootEvent
                               );
    if (EFI_ERROR (ReadyToBootStatus)) {
      DEBUG ((DEBUG_ERROR, "%a - ReadyToBoot callback registration failed! %r\n", __FUNCTION__, ReadyToBootStatus));
    }

    //
    // Register ExitBootServices callback.
    ExitBootServicesStatus = gBS->CreateEventEx (
                                    EVT_NOTIFY_SIGNAL,
                                    TPL_CALLBACK,                     // SOMEWHERE in ExitBootServices
                                    SetExitBootServicesIndicator,
                                    NULL,
                                    &gEfiEventExitBootServicesGuid,
                                    &mExitBootServicesEvent
                                    );
    if (EFI_ERROR (ExitBootServicesStatus)) {
      DEBUG ((DEBUG_ERROR, "%a - ExitBootServices callback registration failed! %r\n", __FUNCTION__, ExitBootServicesStatus));
    }
  }

  // This driver is architecturally important.
  // As such, we should make sure that telemetry is logged if a failure ever occurs.
  if (EFI_ERROR (PolicyStatus) || EFI_ERROR (EndOfDxeStatus) ||
      EFI_ERROR (ReadyToBootStatus) || EFI_ERROR (ExitBootServicesStatus))
  {
    // We will have already logged a more detailed error message.
    ASSERT (FALSE);

    // TODO VARPOL: Telemetry.

    // If any of the callback registrations succeeded, we MUST return EFI_SUCCESS so
    // that the driver remains resident.
    if (EFI_ERROR (PolicyStatus) ||
        (EFI_ERROR (EndOfDxeStatus) && EFI_ERROR (ReadyToBootStatus) && EFI_ERROR (ExitBootServicesStatus)))
    {
      FinalStatus = EFI_ABORTED;
    }
  }

  return FinalStatus;
} // MuVarPolicyFoundationDxeMain()
