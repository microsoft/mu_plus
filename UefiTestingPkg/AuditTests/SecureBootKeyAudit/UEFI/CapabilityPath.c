/** @file
  Advertised Secure Boot image-algorithm path selection.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>

#include "CapabilityPath.h"

UINT8
GetAdvertisedImageAlgorithmMask (
  IN CONST ECIT_CLAIM_RESULT  *ClaimResults,
  IN UINTN                    ClaimResultCount
  )
{
  UINT8  Mask;

  if ((ClaimResults == NULL) || (ClaimResultCount != EcitClaimCount)) {
    return 0;
  }

  Mask = 0;
  if (ClaimResults[EcitImageVerificationRsaSha256].Claim == EcitClaimAdvertised) {
    Mask |= AUDIT_IMAGE_ALGORITHM_RSA;
  }

  if (ClaimResults[EcitImageVerificationEcdsaSha256].Claim == EcitClaimAdvertised) {
    Mask |= AUDIT_IMAGE_ALGORITHM_ECDSA;
  }

  if (ClaimResults[EcitImageVerificationMldsa87].Claim == EcitClaimAdvertised) {
    Mask |= AUDIT_IMAGE_ALGORITHM_ML_DSA;
  }

  return Mask;
}

UINT8
GetAdvertisedUpdateAlgorithmMask (
  IN CONST ECIT_CLAIM_RESULT  *ClaimResults,
  IN UINTN                    ClaimResultCount
  )
{
  UINT8  Mask;

  if ((ClaimResults == NULL) || (ClaimResultCount != EcitClaimCount)) {
    return 0;
  }

  Mask = 0;
  if (ClaimResults[EcitDbUpdateVerificationRsaSha256].Claim == EcitClaimAdvertised) {
    Mask |= AUDIT_IMAGE_ALGORITHM_RSA;
  }

  if (ClaimResults[EcitDbUpdateVerificationEcdsaSha256].Claim == EcitClaimAdvertised) {
    Mask |= AUDIT_IMAGE_ALGORITHM_ECDSA;
  }

  if (ClaimResults[EcitDbUpdateVerificationMldsa87].Claim == EcitClaimAdvertised) {
    Mask |= AUDIT_IMAGE_ALGORITHM_ML_DSA;
  }

  return Mask;
}

EFI_STATUS
BuildImageCapabilityPath (
  IN UINT8                   AdvertisedMask,
  IN BOOLEAN                 ProbeAll,
  OUT AUDIT_CAPABILITY_PATH  *Path
  )
{
  UINT8  RequiredMask;

  if ((Path == NULL) || ((AdvertisedMask & ~AUDIT_IMAGE_ALGORITHM_ALL) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Path, sizeof (*Path));
  RequiredMask = ProbeAll ? AUDIT_IMAGE_ALGORITHM_ALL : AdvertisedMask;
  if (RequiredMask == 0) {
    return EFI_NOT_FOUND;
  }

  if ((RequiredMask & AUDIT_IMAGE_ALGORITHM_RSA) != 0) {
    Path->Algorithms[Path->Count++] = AUDIT_IMAGE_ALGORITHM_RSA;
  }

  if ((RequiredMask & AUDIT_IMAGE_ALGORITHM_ECDSA) != 0) {
    Path->Algorithms[Path->Count++] = AUDIT_IMAGE_ALGORITHM_ECDSA;
  }

  if ((RequiredMask & AUDIT_IMAGE_ALGORITHM_ML_DSA) != 0) {
    Path->Algorithms[Path->Count++] = AUDIT_IMAGE_ALGORITHM_ML_DSA;
  }

  return EFI_SUCCESS;
}
