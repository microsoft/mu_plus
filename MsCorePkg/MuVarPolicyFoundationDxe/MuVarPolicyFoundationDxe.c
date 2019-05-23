/** @file -- MuVarPolicyFoundationDxe.c
This DXE driver will publish policies and state variables to support a couple of different design patterns:
- Locking policies and performing other tests based on whether a DXE phase has passed
    - EndOfDxe
    - ReadyToBoot
    - ExitBootServices
- Setting up a reference variable that can only be written once that can be used in other variable policies

Copyright (C) Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/VariableLock.h>
#include <Protocol/VariablePolicy.h>

#include <Guid/EventGroup.h>
#include <Guid/MuVarPolicyFoundationDxe.h>

EFI_EVENT                 mEndOfDxeEvent;
EFI_EVENT                 mReadyToBootEvent;
EFI_EVENT                 mExitBootServicesEvent;

BOOLEAN                   mEndOfDxeIndicatorSet = FALSE;
BOOLEAN                   mReadyToBootIndicatorSet = FALSE;
BOOLEAN                   mExitBootServicesIndicatorSet = FALSE;

STATIC VARIABLE_POLICY_PROTOCOL *mVariablePolicy = NULL;

/**
  This is an implementation of the RequestToLock interface that pipes everything through
  the Variable Policy engine. Will construct a policy and attempt to add it to the policy table.

  The policy will lock the variable at EndOfDxe.

  @param[in] This          The EDKII_VARIABLE_LOCK_PROTOCOL instance.
  @param[in] VariableName  A pointer to the variable name that will be made read-only subsequently.
  @param[in] VendorGuid    A pointer to the vendor GUID that will be made read-only subsequently.

  @retval EFI_SUCCESS           The variable specified by the VariableName and the VendorGuid was marked
                                as pending to be read-only.
  @retval EFI_INVALID_PARAMETER VariableName or VendorGuid is NULL.
                                Or VariableName is an empty string.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource to hold the lock request.
**/
EFI_STATUS
EFIAPI
VariableLockRequestToLock (
  IN CONST EDKII_VARIABLE_LOCK_PROTOCOL *This,
  IN       CHAR16                       *VariableName,
  IN       EFI_GUID                     *VendorGuid
  )
{
  EFI_STATUS                         Status;
  BOOLEAN                            State;
  VARIABLE_POLICY_ENTRY              *NewEntry;
  VOID                               *EntryName;
  VOID                               *PhaseIndicator;
  UINTN                              NameSize;
  UINTN                              PolicySize;
  UINTN                              LockOnVarStateNameSize;
  CHAR16                             *EndOfDxeIndicatorStr;
  VARIABLE_LOCK_ON_VAR_STATE_POLICY  *LockOnVarStatePolicy;

  NewEntry = NULL;
  EndOfDxeIndicatorStr = END_OF_DXE_INDICATOR_VAR_NAME;

  // Check all the things for the goodness.
  if (VariableName == NULL || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Locate the protocol
  if (mVariablePolicy == NULL) {
    Status = gBS->LocateProtocol(&gVariablePolicyProtocolGuid, NULL, (VOID **) &mVariablePolicy);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "[%a] Error locating Variable Policy protocol: %r\n", __FUNCTION__, Status));
      return Status;
    }
  }

  // Make sure Variable Policy Engine (VPE) is enabled
  State = FALSE;
  Status = mVariablePolicy->IsVariablePolicyEnabled(&State);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "[%a] Error checking Variable Policy Engine is enabled: %r\n", __FUNCTION__, Status));
    return Status;
  }
  if (State == FALSE) {
    DEBUG((DEBUG_ERROR, "[%a] Variable Policy Engine is disabled, not registering any policy entries\n", __FUNCTION__));
    return EFI_DEVICE_ERROR;
  }

  // Now we assume that we're working with something useful.
  // Let's start by figuring out how much memory we would need.
  // Limit by the max size that a policy structure can support: MAX_UINT16.
  // StrnSizeS returns a size, but takes in max number of chars for... reasons.
  NameSize = StrnSizeS( VariableName, MAX_UINT16 / sizeof(*VariableName) );
  LockOnVarStateNameSize = StrnSizeS( EndOfDxeIndicatorStr, MAX_UINT16 / sizeof(*EndOfDxeIndicatorStr) );
  PolicySize = NameSize + sizeof(VARIABLE_POLICY_ENTRY) + LockOnVarStateNameSize + sizeof(VARIABLE_LOCK_ON_VAR_STATE_POLICY);

  // Allocate the memory and forward the request.
  NewEntry = AllocateZeroPool( PolicySize );
  if (NewEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  LockOnVarStatePolicy = (VOID*)(NewEntry + 1);
  PhaseIndicator = (VOID*)(LockOnVarStatePolicy + 1);
  EntryName = (VOID*)(((UINT8 *)PhaseIndicator) + LockOnVarStateNameSize);

  DEBUG(( DEBUG_VERBOSE, "%a -> RegisterVariablePolicy for %g:%s.\n",
          __FUNCTION__, VendorGuid, VariableName ));

  NewEntry->Version         = VARIABLE_POLICY_ENTRY_REVISION;
  NewEntry->Size            = (UINT16)PolicySize;
  NewEntry->OffsetToName    = (UINT16) (sizeof(VARIABLE_POLICY_ENTRY) + sizeof(VARIABLE_LOCK_ON_VAR_STATE_POLICY) + LockOnVarStateNameSize);
  CopyGuid( &NewEntry->Namespace, VendorGuid );
  NewEntry->MinSize = 0;
  NewEntry->MaxSize = MAX_UINT32;
  NewEntry->AttributesMustHave = 0;
  NewEntry->AttributesCantHave = 0;
  NewEntry->LockPolicyType  = VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE;
  CopyGuid( &LockOnVarStatePolicy->Namespace, &gMuVarPolicyDxePhaseGuid );
  LockOnVarStatePolicy->Value = 1;
  CopyMem( PhaseIndicator, EndOfDxeIndicatorStr, LockOnVarStateNameSize );
  CopyMem( EntryName, VariableName, NameSize );

  // IMPORTANT NOTE: If we do this, it will potentially override wildcard policies for the variable.
  //                 Though, if successful, it would still be write protected, so this is probably a no-op.
  Status = mVariablePolicy->RegisterVariablePolicy(NewEntry);
  if (EFI_ERROR( Status )) {
    DEBUG(( DEBUG_ERROR, "[%a] RequestToLock failed in RegisterVariablePolicy! %r\n", __FUNCTION__, Status ));
    ASSERT_EFI_ERROR( Status );
  }

  // Always put away your toys.
  if (NewEntry != NULL) {
    FreePool( NewEntry );
  }

  return Status;
} // VariableLockRequestToLock

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
  IN CHAR16   *IndicatorName,
  IN UINT32   IndicatorAttributes
  )
{
  EFI_STATUS        Status = EFI_SUCCESS;
  PHASE_INDICATOR   Indicator = TRUE;

  DEBUG(( DEBUG_VERBOSE, "%a - Setting indicator '%s'...\n", __FUNCTION__, IndicatorName ));

  //
  // Attempt to create the variable.
  Status = gRT->SetVariable( IndicatorName,
                             &gMuVarPolicyDxePhaseGuid,
                             IndicatorAttributes,
                             sizeof( Indicator ),
                             &Indicator );
  if (EFI_ERROR( Status ))
  {
    DEBUG(( DEBUG_ERROR, "%a - Error creating indicator! %r\n", __FUNCTION__, Status ));
    ASSERT_EFI_ERROR( Status );
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
  IN      EFI_EVENT                 Event,
  IN      VOID                      *Context
  )
{
  EFI_STATUS  Status;

  Status = SetPhaseIndicator( END_OF_DXE_INDICATOR_VAR_NAME, END_OF_DXE_INDICATOR_VAR_ATTR );

  // If successful, close the event so it doesn't get signalled repeatedly.
  if (!EFI_ERROR( Status ))
  {
    mEndOfDxeIndicatorSet = TRUE;
    gBS->CloseEvent( mEndOfDxeEvent );
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
  IN      EFI_EVENT                 Event,
  IN      VOID                      *Context
  )
{
  EFI_STATUS  Status;

  Status = SetPhaseIndicator( READY_TO_BOOT_INDICATOR_VAR_NAME, READY_TO_BOOT_INDICATOR_VAR_ATTR );

  // If successful, close the event so it doesn't get signalled repeatedly.
  if (!EFI_ERROR( Status ))
  {
    // If EndOfDxe was never signalled, make a last-ditch effort to signal it.
    if (!mEndOfDxeIndicatorSet) {
      SetEndOfDxeIndicator(Event, Context);
    }
    mReadyToBootIndicatorSet = TRUE;
    gBS->CloseEvent( mReadyToBootEvent );
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
  IN      EFI_EVENT                 Event,
  IN      VOID                      *Context
  )
{
  EFI_STATUS  Status;

  Status = SetPhaseIndicator( EXIT_BOOT_SERVICES_INDICATOR_VAR_NAME, EXIT_BOOT_SERVICES_INDICATOR_VAR_ATTR );

  // If successful, close the event so it doesn't get signalled repeatedly.
  if (!EFI_ERROR( Status ))
  {
    // If EndOfDxe was never signalled, make a last-ditch effort to signal it.
    if (!mEndOfDxeIndicatorSet) {
      SetEndOfDxeIndicator(Event, Context);
    }
    // If ReadyToBoot was never signalled, make a last-ditch effort to signal it.
    if (!mReadyToBootIndicatorSet) {
      SetReadyToBootIndicator(Event, Context);
    }
    mExitBootServicesIndicatorSet = TRUE;
    gBS->CloseEvent( mExitBootServicesEvent );
  }
} // SetExitBootServicesIndicator()

EDKII_VARIABLE_LOCK_PROTOCOL mVariableLock = { VariableLockRequestToLock };

/**
  The driver's entry point.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
MuVarPolicyFoundationDxeMain (
  IN    EFI_HANDLE                  ImageHandle,
  IN    EFI_SYSTEM_TABLE            *SystemTable
  )
{
  EFI_STATUS                  FinalStatus;
  EFI_STATUS                  PolicyStatus;
  EFI_STATUS                  VarLockStatus;
  VARIABLE_POLICY_ENTRY       PolicyEntry;
  EFI_STATUS                  EndOfDxeStatus;
  EFI_STATUS                  ReadyToBootStatus;
  EFI_STATUS                  ExitBootServicesStatus;

  FinalStatus = EFI_SUCCESS;

  DEBUG(( DEBUG_VERBOSE, "%a()\n", __FUNCTION__ ));

  //
  // First, make sure that we can locate and set the required policy.
  PolicyStatus = gBS->LocateProtocol( &gVariablePolicyProtocolGuid, NULL, (VOID **) &mVariablePolicy );
  if (!EFI_ERROR( PolicyStatus )) {
    // IMPORTANT NOTE: On the whole, it is a *bad* idea to use LOCK_ON_CREATE for a namespace policy.
    //                 However, since these are are all forced to be Volatile variables and since you can't create
    //                 volatile variables after ExitBootServices (and the variables will disappear on reboot),
    //                 this isn't the end of the world.
    PolicyEntry.Version             = VARIABLE_POLICY_ENTRY_REVISION;
    PolicyEntry.Size                = sizeof(VARIABLE_POLICY_ENTRY);
    PolicyEntry.OffsetToName        = sizeof(VARIABLE_POLICY_ENTRY);
    CopyGuid( &PolicyEntry.Namespace, &gMuVarPolicyDxePhaseGuid );
    PolicyEntry.MinSize             = sizeof(PHASE_INDICATOR);
    PolicyEntry.MaxSize             = sizeof(PHASE_INDICATOR);
    PolicyEntry.AttributesMustHave  = DXE_PHASE_INDICATOR_ATTR;
    PolicyEntry.AttributesCantHave  = (UINT32)(~DXE_PHASE_INDICATOR_ATTR);
    PolicyEntry.LockPolicyType      = VARIABLE_POLICY_TYPE_LOCK_ON_CREATE;

    PolicyStatus = mVariablePolicy->RegisterVariablePolicy( &PolicyEntry );
  }
  if (EFI_ERROR( PolicyStatus )) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to set namespace VariablePolicy! %r\n", __FUNCTION__, PolicyStatus ));
  }

  // Register a policy to describe the write-once state variable namespace.
  if (!EFI_ERROR( PolicyStatus )) {
    CopyGuid( &PolicyEntry.Namespace, &gMuVarPolicyWriteOnceStateVarGuid );
    PolicyEntry.MinSize             = sizeof(POLICY_LOCK_VAR);
    PolicyEntry.MaxSize             = sizeof(POLICY_LOCK_VAR);
    PolicyEntry.AttributesMustHave  = WRTIE_ONCE_STATE_VAR_ATTR;
    PolicyEntry.AttributesCantHave  = (UINT32)(~WRTIE_ONCE_STATE_VAR_ATTR);

    PolicyStatus = mVariablePolicy->RegisterVariablePolicy( &PolicyEntry );
  }
  if (EFI_ERROR( PolicyStatus )) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to register WriteOnce state var policy! %r\n", __FUNCTION__, PolicyStatus ));
  }

  if (!EFI_ERROR( PolicyStatus )) {
    //
    // Register EndOfDxe callback.
    EndOfDxeStatus = gBS->CreateEventEx( EVT_NOTIFY_SIGNAL,
                                         TPL_CALLBACK - 1,          // At end of EndOfDxe
                                         SetEndOfDxeIndicator,
                                         NULL,
                                         &gEfiEndOfDxeEventGroupGuid,
                                         &mEndOfDxeEvent );
    if (EFI_ERROR( EndOfDxeStatus ))
    {
      DEBUG(( DEBUG_ERROR, "%a - EndOfDxe callback registration failed! %r\n", __FUNCTION__, EndOfDxeStatus ));
    }

    //
    // Register ReadyToBoot callback.
    ReadyToBootStatus = gBS->CreateEventEx( EVT_NOTIFY_SIGNAL,
                                            TPL_CALLBACK - 1,       // At end of ReadyToBoot
                                            SetReadyToBootIndicator,
                                            NULL,
                                            &gEfiEventReadyToBootGuid,
                                            &mReadyToBootEvent );
    if (EFI_ERROR( ReadyToBootStatus ))
    {
      DEBUG(( DEBUG_ERROR, "%a - ReadyToBoot callback registration failed! %r\n", __FUNCTION__, ReadyToBootStatus ));
    }

    //
    // Register ExitBootServices callback.
    ExitBootServicesStatus = gBS->CreateEventEx( EVT_NOTIFY_SIGNAL,
                                                 TPL_CALLBACK,        // SOMEWHERE in ExitBootServices
                                                 SetExitBootServicesIndicator,
                                                 NULL,
                                                 &gEfiEventExitBootServicesGuid,
                                                 &mExitBootServicesEvent );
    if (EFI_ERROR( ExitBootServicesStatus ))
    {
      DEBUG(( DEBUG_ERROR, "%a - ExitBootServices callback registration failed! %r\n", __FUNCTION__, ExitBootServicesStatus ));
    }
  }

  // Install VarLock Protocol here as well, since it's tied to phase indicator variables
  VarLockStatus = gBS->InstallMultipleProtocolInterfaces ( &ImageHandle,
                                                           &gEdkiiVariableLockProtocolGuid,
                                                           &mVariableLock,
                                                           NULL);
  if (EFI_ERROR( VarLockStatus ))
  {
    DEBUG(( DEBUG_ERROR, "%a - EdkiiVariableLockProtocol installation failed! %r\n", __FUNCTION__, VarLockStatus ));
  }

  // This driver is architecturally important.
  // As such, we should make sure that telemetry is logged if a failure ever occurs.
  if (EFI_ERROR( PolicyStatus ) || EFI_ERROR( VarLockStatus ) || EFI_ERROR( EndOfDxeStatus ) ||
      EFI_ERROR( ReadyToBootStatus ) || EFI_ERROR( ExitBootServicesStatus )) {
    // We will have already logged a more detailed error message.
    ASSERT( FALSE );

    // TODO VARPOL: Telemetry.
  
    // If any of the callback registrations succeeded, we MUST return EFI_SUCCESS so
    // that the driver remains resident.
    if (EFI_ERROR( PolicyStatus ) ||
        (EFI_ERROR( EndOfDxeStatus ) && EFI_ERROR(ReadyToBootStatus) && EFI_ERROR(ExitBootServicesStatus))) {
      FinalStatus = EFI_ABORTED;
    }
  }

  return FinalStatus;
} // MuVarPolicyFoundationDxeMain()
