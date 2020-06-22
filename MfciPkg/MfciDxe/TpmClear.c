/** @file
  Registers for MFCI Policy change notification and if the
  Tpm Clear bit is set, clears the TPM using Platform Hierarchy with NULL auth

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>    // gBS

#include <MfciPolicyType.h>
#include <Protocol/MfciProtocol.h>

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/Tpm2CommandLib.h>
#include <Protocol/Tcg2Protocol.h>


// forward declaration, documentation below with implementation
EFI_STATUS
SimpleTpmClear (
  VOID
  );


/**
  Callback invocation for MFCI policy changes.
  This function will be called prior to system reset when a MFCI policy change is detected.

  Callbacks should perform all actions specified in the actions bit ranges of NewPolicy
  These actions can be performed synchronously, or pended to subsequent boot(s), but are
  expected to be completed before the system reaches EndOfDxe()

  @param[in] NewPolicy          The policy that will become active after the reset
  @param[in] PreviousPolicy     The policy active for the current boot

  @retval EFI_SUCCESS           The callback function has been done successfully.
  @retval EFI_UNSUPPORTED       There are no actions to perform for this transition.
  @retval Others                Some part of the R&R has not been completed.

**/
EFI_STATUS
EFIAPI MfciPolicyChangeCallbackTpm (
  IN CONST MFCI_POLICY_TYPE   NewPolicy,
  IN CONST MFCI_POLICY_TYPE   PreviousPolicy
)
{
  EFI_STATUS Status;

  if ( (NewPolicy & STD_ACTION_TPM_CLEAR) != 0) {
    Status = SimpleTpmClear();
    // HINT: reset TPM to default?  Enable it if disabled?
  }
  else {
    Status = EFI_UNSUPPORTED;
  }
  return Status;
}


EFI_STATUS
EFIAPI
InitTpmListener (
  VOID
)
{
  EFI_STATUS Status;
  MFCI_PROTOCOL *MfciPolicyProtocol = NULL;

  DEBUG(( DEBUG_INFO, "%a() - Enter\n", __FUNCTION__ ));

  Status = gBS->LocateProtocol( &gMfciProtocolGuid, NULL, &MfciPolicyProtocol );
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Locating MFCI Policy failed - %r\n", __FUNCTION__, Status ));
    return Status;
  }

  REGISTER_MFCI_POLICY_CHANGE_CALLBACK RegisterMfciPolicyChangeCallback = MfciPolicyProtocol->RegisterMfciPolicyChangeCallback;
  Status = RegisterMfciPolicyChangeCallback( MfciPolicyProtocol, MfciPolicyChangeCallbackTpm );
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Registering TpmClear Callback failed - %r\n", __FUNCTION__, Status ));
    return Status;
  }

  return Status;
}


/**
  This function will perform a traditional TPM clear.
  It removes all data from the Storage and Endorsement Hierarchies,
  but does not alter the Platform Hierarchy.

  Requires PH to be enabled and Auth to be NULL.

  @retval     EFI_SUCCESS   Clear has been performed successfully.
  @retval     Others        Something went wrong.

**/
EFI_STATUS
SimpleTpmClear (
  VOID
  )
{
  EFI_STATUS      Status;

  DEBUG(( DEBUG_INFO, "TpmClear::%a()\n", __FUNCTION__ ));

  // Disable "clear" protections (use NULL auth).
  Status = Tpm2ClearControl( TPM_RH_PLATFORM, NULL, NO );
  if (EFI_ERROR( Status ))
  {
    DEBUG(( DEBUG_ERROR, "%a - Tpm2ClearControl = %r\n", __FUNCTION__, Status ));
  }

  // If we're successful, actually clear the TPM (use NULL auth).
  if (!EFI_ERROR( Status ))
  {
    Status = Tpm2Clear( TPM_RH_PLATFORM, NULL );
    if (EFI_ERROR( Status ))
    {
      DEBUG(( DEBUG_ERROR, "%a - Tpm2Clear = %r\n", __FUNCTION__, Status ));
    }
  }

  return Status;
} // SimpleTpmClear()
