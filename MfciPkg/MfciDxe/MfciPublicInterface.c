/** @file
  Provides the public interface used to determine the currently in-effect
  MFCI Policy

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <MfciPolicyType.h>
#include <MfciVariables.h>

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Guid/MuVarPolicyFoundationDxe.h>

#include <Protocol/MfciProtocol.h>
#include <Protocol/MfciPolicyChangeNotify.h>

#include "MfciDxe.h"


/**
  GetMfciPolicy()

  This function returns the MFCI Policy in effect for the current boot

  @param[in] This                - Current MFCI policy ppi installed.

  @retval Other                  - Bits definition can be found in MfciPolicyType.h.

**/
MFCI_POLICY_TYPE
EFIAPI
InternalGetMfciPolicy (
  IN CONST MFCI_PROTOCOL   *This
  );

/**
  Library function to register a new MFCI Policy change callback.
  This function will take care of not only the callback registration, but
  it will enforce security protections to make sure that the callback doesn't stay
  resident after the time that it should be executed legitimately.

  NOTE: This callback doesn't make sense post-EndOfDxe.

  @param[in] Callback           Pointer to the callback function being registered.

  @retval EFI_SUCCESS           Callback was successfully registered.
  @retval EFI_ALREADY_STARTED   We have passed EndOfDxe and this callback no longer
                                makes sense.
  @retval Others                Callback registration failed.
**/
EFI_STATUS
EFIAPI
InternalRegisterMfciPolicyChangeNotifyCallback (
  IN CONST MFCI_PROTOCOL     *This,
  IN MFCI_POLICY_CHANGE_CALLBACK    Callback
  );

/**
  NOTE: This function should ultimately be copied to a phase indicator lib.

  Helper function to determine whether a given phase change indicator
  has been registered, indicating that the specified boot phase has elapsed.

  @param[in]  PhaseName   CHAR16 string for the phase indicator being checked.

  @retval     EFI_SUCCESS             Phase indicator found.
  @retval     EFI_NOT_FOUND           Phase indicator was not found.
  @retval     EFI_SECURITY_VIOLATION  Phase indicator was found, but is 
                                      formatted incorrectly.
  @retval     EFI_ABORTED             An error occurred checking for phase indicator.
**/
STATIC
EFI_STATUS
GetPhaseIndicatorStatus (
  IN CONST CHAR16       *PhaseName
  );

/**
  This callback is registered on EndOfDxe to clean up the notification context.

  This is done for security so that after EndOfDxe an ellicit attempt may not be
  made to trick drivers in a MFCI Policy change.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
STATIC
VOID
EFIAPI
CleanupMfciPolicyChangeNotify (
  IN EFI_EVENT      Event,
  IN VOID           *Context
  );

MFCI_PROTOCOL gMfciProtocol = {
  .GetMfciPolicy = InternalGetMfciPolicy,
  .RegisterMfciPolicyChangeCallback = InternalRegisterMfciPolicyChangeNotifyCallback
};

EFI_HANDLE mMfciPolicyHandle = NULL;

//==============================================================================
// POLICY CHANGE NOTIFY STRUCTURE
// This structure will be registered as the context for the EndOfDxe callback.
// It is used to clean up the notification if MFCI Policy does not trigger the callback.
typedef struct _MFCI_PROTOCOL_CONTEXT
{
  EFI_HANDLE                              ProtocolInstallHandle;
  MFCI_POLICY_CHANGE_NOTIFY_PROTOCOL  Protocol;
} MFCI_PROTOCOL_CONTEXT;


/**
  This helper function walks the list of notification handles, invoking their callback
  functions.

  @param[in]  NewPolicy     The new policy to pass to the callbacks
**/
EFI_STATUS
EFIAPI
NotifyMfciPolicyChange (
  IN MFCI_POLICY_TYPE           NewPolicy
  )
{
  EFI_STATUS                              Status;
  UINTN                                   Index, HandleCount;
  EFI_HANDLE                              *HandleBuffer = NULL;
  MFCI_POLICY_CHANGE_NOTIFY_PROTOCOL  *NotifyProtocol;

  DEBUG(( DEBUG_INFO, "%a - Notifying MFCI Policy change from 0x%x to 0x%x.\n",
          __FUNCTION__, mCurrentPolicy, NewPolicy ));

  //
  // Find all instances of the notify protocol.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gMfciPolicyChangeNotifyProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  //
  // If we're good, let's walk each and dispatch the notification function.
  //
  if (!EFI_ERROR( Status )) {
    //
    // Walk each client and notify them that the policy is changing across the reset
    //
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gMfciPolicyChangeNotifyProtocolGuid,
                      (VOID**)&NotifyProtocol
                      );
      if (!EFI_ERROR( Status )) {
        NotifyProtocol->Callback (NewPolicy, mCurrentPolicy);
      }
      else {
        DEBUG(( DEBUG_ERROR,
                "%a - MFCI Policy Change Notify registered, but could not be found!\n",
                __FUNCTION__ ));
        ASSERT( FALSE );
        break;
      }
    }

    //
    // Make sure to free the buffer. Not that the intention isn't to
    // IMMEDIATELY bail.
    //
    FreePool( HandleBuffer );
    HandleBuffer = NULL;
  }

  return Status;
} // NotifyMfciPolicyChange()

/**
  GetMfciPolicy()

  This function returns the MFCI Policy in effect for the current boot

  @param[in] This                - Current MFCI policy ppi installed.

  @retval Other                  - Bits definition can be found in MfciPolicyType.h.

**/
MFCI_POLICY_TYPE
EFIAPI
InternalGetMfciPolicy (
  IN CONST MFCI_PROTOCOL   *This
  )
{
  if (This == NULL) {
    // Do not give out any if input parameter is insane.
    ASSERT (FALSE);
    goto Cleanup;
  }

Cleanup:
  return mCurrentPolicy;
}

/**
  Library function to register a new MFCI Policy change callback.
  This function will take care of not only the callback registration, but
  it will enforce security protections to make sure that the callback doesn't stay
  resident after the time that it should be executed legitimately.

  NOTE: This callback doesn't make sense post-EndOfDxe.

  @param[in] Callback           Pointer to the callback function being registered.

  @retval EFI_SUCCESS           Callback was successfully registered.
  @retval EFI_ALREADY_STARTED   We have passed EndOfDxe and this callback no longer
                                makes sense.
  @retval Others                Callback registration failed.
**/
EFI_STATUS
EFIAPI
InternalRegisterMfciPolicyChangeNotifyCallback (
  IN CONST MFCI_PROTOCOL     *This,
  IN MFCI_POLICY_CHANGE_CALLBACK    Callback
  )
{
  EFI_STATUS        Status;
  MFCI_PROTOCOL_CONTEXT   *ProtocolContext = NULL;
  EFI_EVENT         EndOfDxeCleanupEvent = NULL;

  DEBUG(( DEBUG_VERBOSE, "%a()\n", __FUNCTION__));

  //
  // First, make sure that we're not past EndOfDxe.
  // If we are, don't do anything.
  Status = GetPhaseIndicatorStatus( END_OF_DXE_INDICATOR_VAR_NAME );
  if (Status != EFI_NOT_FOUND) {
    DEBUG(( DEBUG_INFO, "%a - Skipping registration. Past EndOfDxe.\n", 
            __FUNCTION__));
    return EFI_ALREADY_STARTED;
  }

  //
  // Now that we know we're registering the callback for real,
  // let's allocate some space for it to live.
  ProtocolContext = AllocateZeroPool( sizeof( *ProtocolContext ) );
  if (ProtocolContext == NULL) {
    DEBUG(( DEBUG_ERROR, 
            "%a - Failed to allocate memory for the notification protocol context!!\n", 
            __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set up the protocol.
  ProtocolContext->Protocol.Callback = Callback;

  //
  // Install the protocol.
  Status = gBS->InstallProtocolInterface( &ProtocolContext->ProtocolInstallHandle,
                                          &gMfciPolicyChangeNotifyProtocolGuid,
                                          EFI_NATIVE_INTERFACE,
                                          &ProtocolContext->Protocol );
  DEBUG(( DEBUG_VERBOSE, "%a - InstallProtocolInterface() = %r\n", 
          __FUNCTION__, Status ));

  //
  // Register the EndOfDxe callback to clean up the
  // notification if MFCI Policy doesn't use it.
  if (!EFI_ERROR( Status )) {
    Status = gBS->CreateEventEx( EVT_NOTIFY_SIGNAL,
                                 TPL_CALLBACK,
                                 CleanupMfciPolicyChangeNotify,     // Callback
                                 ProtocolContext,                 // Context
                                 &gEfiEndOfDxeEventGroupGuid,     // GUID
                                 &EndOfDxeCleanupEvent );         // Event Handle
    DEBUG(( DEBUG_VERBOSE, "%a - CreateEventEx() = %r\n", 
            __FUNCTION__, Status ));
  }

  //
  // If there was an error, attempt to unregister and deallocate.
  if (EFI_ERROR( Status )) {
    // If the protocol was installed, uninstall it.
    if (ProtocolContext->ProtocolInstallHandle != NULL) {
      gBS->UninstallProtocolInterface( ProtocolContext->ProtocolInstallHandle,
                                       &gMfciPolicyChangeNotifyProtocolGuid,
                                       &ProtocolContext->Protocol );
    }

    // If the context was allocated, destroy it.
    if (ProtocolContext != NULL) {
      DEBUG_CODE(
        ZeroMem( ProtocolContext, sizeof( *ProtocolContext ) );
      );
      FreePool( ProtocolContext );
      ProtocolContext = NULL;
    }
  }

  return Status;
}

/**
  This callback is registered on EndOfDxe to clean up the notification context.

  This is done for security so that after EndOfDxe an ellicit attempt may not be
  made to trick drivers in a MFCI Policy change.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
STATIC
VOID
EFIAPI
CleanupMfciPolicyChangeNotify (
  IN EFI_EVENT      Event,
  IN VOID           *Context
  )
{
  MFCI_PROTOCOL_CONTEXT   *ProtocolContext = Context;

  DEBUG(( DEBUG_VERBOSE, "%a(0x%X, 0x%X)\n", __FUNCTION__,
          ProtocolContext->ProtocolInstallHandle,
          &ProtocolContext->Protocol ));

  //
  // Close the event so we don't trigger multiple times.
  gBS->CloseEvent( Event );

  //
  // Uninstall protocol on handle.
  gBS->UninstallProtocolInterface( ProtocolContext->ProtocolInstallHandle,
                                   &gMfciPolicyChangeNotifyProtocolGuid,
                                   &ProtocolContext->Protocol );

  //
  // Free memory for the context.
  DEBUG_CODE(
    ZeroMem( ProtocolContext, sizeof( *ProtocolContext ) );
  );
  FreePool( ProtocolContext );
  ProtocolContext = NULL;

  return;
} // CleanupMfciPolicyChangeNotify()

/**
  NOTE: This function should ultimately be copied to a phase indicator lib.

  Helper function to determine whether a given phase change indicator
  has been registered, indicating that the specified boot phase has elapsed.

  @param[in]  PhaseName   CHAR16 string for the phase indicator being checked.

  @retval     EFI_SUCCESS             Phase indicator found.
  @retval     EFI_NOT_FOUND           Phase indicator was not found.
  @retval     EFI_SECURITY_VIOLATION  Phase indicator was found, but is 
                                      formatted incorrectly.
  @retval     EFI_ABORTED             An error occurred checking for phase indicator.
**/
STATIC
EFI_STATUS
GetPhaseIndicatorStatus (
  IN CONST CHAR16       *PhaseName
  )
{
  EFI_STATUS          Status;
  UINT32              Attributes;
  UINTN               DataSize;
  PHASE_INDICATOR     Indicator;

  DEBUG(( DEBUG_VERBOSE, "%a(%s)\n", __FUNCTION__, PhaseName ));

  //
  // Attempt to get the indicator.
  DataSize = sizeof( Indicator );
  Status = gRT->GetVariable( (CHAR16*)PhaseName,
                             &gMuVarPolicyDxePhaseGuid,
                             &Attributes,
                             &DataSize,
                             &Indicator );
  DEBUG(( DEBUG_VERBOSE, "%a - GetVariable() = %r\n", __FUNCTION__, Status ));

  //
  // If successfully found, see whether there was a problem with the data.
  if (!EFI_ERROR( Status )) {
    if (Attributes != DXE_PHASE_INDICATOR_ATTR ||
        DataSize   != sizeof( Indicator )      ||
        Indicator  != TRUE) {
      DEBUG(( DEBUG_ERROR, "%a - Variable found but doesn't look right!!\n",
              __FUNCTION__));
      Status = EFI_SECURITY_VIOLATION;
    }
  }
  //
  // Otherwise, if there was a non-EFI_NOT_FOUND error, abort.
  else if (Status != EFI_NOT_FOUND) {
    DEBUG(( DEBUG_ERROR, "%a - Unrecognized error %r!!\n", __FUNCTION__, Status ));
    Status = EFI_ABORTED;
  }

  return Status;
} // GetPhaseIndicatorStatus()

EFI_STATUS
EFIAPI
InitPublicInterface (
  VOID
)
{
  EFI_STATUS Status;
  Status = gBS->InstallProtocolInterface (&mMfciPolicyHandle,
                    &gMfciProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &gMfciProtocol);

  DEBUG ((DEBUG_INFO, "Installing MFCI policy interface - %r\n", Status));

  return Status;
}
