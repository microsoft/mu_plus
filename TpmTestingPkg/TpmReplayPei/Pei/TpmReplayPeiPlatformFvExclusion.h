/** @file
  TPM Replay FV Exclusion - Module-Wide Header File

  Generic APIs and definitions shared across the module.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TPM_REPLAY_PEI_PLATFORM_FV_EXCLUSION_H_
#define TPM_REPLAY_PEI_PLATFORM_FV_EXCLUSION_H_

#include <PiPei.h>

/**
  Registers the platform-selected firmware volumes for measurement exclusion.

  @retval    EFI_SUCCESS            The exclusions were registered successfully.
  @retval    EFI_OUT_OF_RESOURCES   Insufficient memory resources to allocate a necessary buffer.
  @retval    Others                 An error occurred preventing the excluded FVs from being
                                    return successfully.

**/
EFI_STATUS
InstallPlatformFvExclusions (
  VOID
  );

#endif
