/** @file
  Firmware Volume Measurement Measurement Exclusion Library NULL instance.

  This library instance does not exclude any firmware volumes.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/FvMeasurementExclusionLib.h>

/**
  Gets a list of FVs excluded from measurement.

  @param[out] ExcludedFvs          A pointer to an array of excluded FV structures.
  @param[out] ExcludedFvsCount     The number of excluded FV structures.

  @retval    EFI_SUCCESS    The excluded FVs were returned successfully.
  @retval    Others         An error occurred preventing the excluded FVs from being
                            return successfully.

**/
EFI_STATUS
EFIAPI
GetPlatformFvExclusions (
  OUT CONST EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_FV  **ExcludedFvs,
  OUT UINTN                                                       *ExcludedFvsCount
  )
{
  if (ExcludedFvs != NULL) {
    *ExcludedFvs = NULL;
  }

  if (ExcludedFvsCount != NULL) {
    *ExcludedFvsCount = 0;
  }

  return EFI_SUCCESS;
}
