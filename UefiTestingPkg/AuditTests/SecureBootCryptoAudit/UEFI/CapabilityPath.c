/** @file
  Advertised Secure Boot image-algorithm path selection.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>

#include "CapabilityPath.h"

/**
  Returns the mask of advertised image-verification algorithms.

  @param[in] ClaimResults      ECIT claim results to inspect.
  @param[in] ClaimResultCount  Number of entries in ClaimResults.

  @return The advertised algorithm mask, or zero for invalid claim results.
**/
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

/**
  Returns the mask of advertised authenticated-update algorithms.

  @param[in] ClaimResults      ECIT claim results to inspect.
  @param[in] ClaimResultCount  Number of entries in ClaimResults.

  @return The mask of complete advertised update profiles, or zero for invalid
          claim results.
**/
UINT8
GetAdvertisedUpdateAlgorithmMask (
  IN CONST ECIT_CLAIM_RESULT  *ClaimResults,
  IN UINTN                    ClaimResultCount
  )
{
  UINT8  Mask;

  if ((ClaimResults == NULL) || (ClaimResultCount != EcitClaimCount) ||
      (ClaimResults[EcitDbUpdateAuthorizationX509].Claim != EcitClaimAdvertised))
  {
    return 0;
  }

  Mask = 0;
  if ((ClaimResults[EcitDbUpdateVerificationRsaSha256].Claim == EcitClaimAdvertised) &&
      (ClaimResults[EcitDbUpdateVerificationSha256].Claim == EcitClaimAdvertised) &&
      (ClaimResults[EcitDbUpdateVerificationRsaSha256Profile].Claim == EcitClaimAdvertised))
  {
    Mask |= AUDIT_IMAGE_ALGORITHM_RSA;
  }

  if ((ClaimResults[EcitDbUpdateVerificationEcdsaSha256].Claim == EcitClaimAdvertised) &&
      (ClaimResults[EcitDbUpdateVerificationSha256].Claim == EcitClaimAdvertised) &&
      (ClaimResults[EcitDbUpdateVerificationEcdsaSha256Profile].Claim == EcitClaimAdvertised))
  {
    Mask |= AUDIT_IMAGE_ALGORITHM_ECDSA;
  }

  if ((ClaimResults[EcitDbUpdateVerificationMldsa87].Claim == EcitClaimAdvertised) &&
      (ClaimResults[EcitDbUpdateVerificationSha512].Claim == EcitClaimAdvertised) &&
      (ClaimResults[EcitDbUpdateVerificationMldsa87Sha512Profile].Claim == EcitClaimAdvertised))
  {
    Mask |= AUDIT_IMAGE_ALGORITHM_ML_DSA;
  }

  return Mask;
}

/**
  Builds the ordered image-verification capability path.

  @param[in]  AdvertisedMask  Mask of algorithms advertised by ECIT.
  @param[in]  ProbeAll        TRUE to include every supported algorithm.
  @param[out] Path            Receives the ordered capability path.

  @retval EFI_SUCCESS            The capability path was built.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          No algorithms were selected.
**/
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
