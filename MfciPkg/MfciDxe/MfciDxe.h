/** @file
  Declares the internal interface used to determine the currently in-effect
  MFCI Policy

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __FIRMWARE_PLOICY_DXE_H__
#define __FIRMWARE_PLOICY_DXE_H__

extern MFCI_POLICY_TYPE   mCurrentPolicy;

/**
  This is the definition of MFCI policies that this package natively support.

**/
EFI_STATUS
EFIAPI
InitPublicInterface (
  VOID
);

EFI_STATUS
VerifyTargeting (
  VOID                              *PolicyBlob,
  UINTN                             PolicyBlobSize,
  UINT64                            ExpectedNonce,
  MFCI_POLICY_TYPE                  *ExtractedPolicy
);

EFI_STATUS
EFIAPI
NotifyMfciPolicyChange (
  IN MFCI_POLICY_TYPE           NewPolicy
);

// Initializer for the SecureBoot Callback
EFI_STATUS
EFIAPI
InitSecureBootListener (
  VOID
);

// Initializer for the TpmClear Callback
EFI_STATUS
EFIAPI
InitTpmListener (
  VOID
);

#endif //__FIRMWARE_PLOICY_DXE_H__
