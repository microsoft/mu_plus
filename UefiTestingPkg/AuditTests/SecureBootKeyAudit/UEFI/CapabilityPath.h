/** @file
  Advertised Secure Boot image-algorithm path selection.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef CAPABILITY_PATH_H_
#define CAPABILITY_PATH_H_

#include <Uefi.h>

#include "AuditTypes.h"

#define AUDIT_IMAGE_ALGORITHM_RSA     BIT0
#define AUDIT_IMAGE_ALGORITHM_ECDSA   BIT1
#define AUDIT_IMAGE_ALGORITHM_ML_DSA  BIT2
#define AUDIT_IMAGE_ALGORITHM_ALL     \
  (AUDIT_IMAGE_ALGORITHM_RSA | AUDIT_IMAGE_ALGORITHM_ECDSA | AUDIT_IMAGE_ALGORITHM_ML_DSA)

#define AUDIT_MAX_CAPABILITY_PATH_LENGTH  3

typedef struct {
  UINT8    Count;
  UINT8    Algorithms[AUDIT_MAX_CAPABILITY_PATH_LENGTH];
} AUDIT_CAPABILITY_PATH;

UINT8
GetAdvertisedImageAlgorithmMask (
  IN CONST ECIT_CLAIM_RESULT  *ClaimResults,
  IN UINTN                    ClaimResultCount
  );

UINT8
GetAdvertisedUpdateAlgorithmMask (
  IN CONST ECIT_CLAIM_RESULT  *ClaimResults,
  IN UINTN                    ClaimResultCount
  );

EFI_STATUS
BuildImageCapabilityPath (
  IN UINT8                   AdvertisedMask,
  IN BOOLEAN                 ProbeAll,
  OUT AUDIT_CAPABILITY_PATH  *Path
  );

#endif
