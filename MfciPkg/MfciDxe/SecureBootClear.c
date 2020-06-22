/** @file
  Registers for MFCI Policy change notification and if the Secure
  Boot Clear bit is set, disables Variable Policy and deletes the Secure Boot
  keys

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>    // gBS

#include <MfciPolicyType.h>
#include <Protocol/MfciProtocol.h>
#include <Library/MuSecureBootLib.h>


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
EFIAPI MfciPolicyChangeCallbackSecureBoot (
  IN CONST MFCI_POLICY_TYPE   NewPolicy,
  IN CONST MFCI_POLICY_TYPE   PreviousPolicy
)
{
  EFI_STATUS Status = EFI_UNSUPPORTED;

  if ( (NewPolicy & STD_ACTION_SECURE_BOOT_CLEAR) != 0) {
    Status = DeleteSecureBootVariables();
  }
  else {
    Status = EFI_UNSUPPORTED;
  }
  return Status;
}


EFI_STATUS
EFIAPI
InitSecureBootListener (
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
  Status = RegisterMfciPolicyChangeCallback( MfciPolicyProtocol, MfciPolicyChangeCallbackSecureBoot );
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Registering SecureBootClear Callback failed - %r\n", __FUNCTION__, Status ));
    return Status;
  }

  return Status;
}
