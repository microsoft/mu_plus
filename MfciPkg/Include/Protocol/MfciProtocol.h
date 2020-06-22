/** @file
  Declares the interface to both query the in-effect MFCI Policy
  as well as to register notifications when the policy changes.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MFCI_PROTOCOL_H__
#define __MFCI_PROTOCOL_H__

typedef struct _MFCI_PROTOCOL  MFCI_PROTOCOL;

/**
  GetMfciPolicy()

  This function returns the MFCI Policy in effect for the current boot

  @param[in] This                - Current MFCI policy protocol installed.

  @retval Other                  - Bits definition can be found in MfciPolicyType.h.

**/
typedef
MFCI_POLICY_TYPE
(EFIAPI *GET_MFCI_POLICY) (
IN CONST MFCI_PROTOCOL   *This
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
typedef
EFI_STATUS
(EFIAPI *MFCI_POLICY_CHANGE_CALLBACK) (
  IN CONST MFCI_POLICY_TYPE   NewPolicy,
  IN CONST MFCI_POLICY_TYPE   PreviousPolicy
  );

/**
  Library function to register a new MFCI policy change callback.
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
typedef
EFI_STATUS
(EFIAPI *REGISTER_MFCI_POLICY_CHANGE_CALLBACK) (
IN CONST MFCI_PROTOCOL   *This,
IN MFCI_POLICY_CHANGE_CALLBACK   Callback
);


struct _MFCI_PROTOCOL
{
  GET_MFCI_POLICY                       GetMfciPolicy;
  REGISTER_MFCI_POLICY_CHANGE_CALLBACK  RegisterMfciPolicyChangeCallback;
};

extern EFI_GUID gMfciProtocolGuid;

#endif // __MFCI_PROTOCOL_H__
