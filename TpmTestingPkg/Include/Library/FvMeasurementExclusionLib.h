/** @file
  Firmware Volume Measurement Exclusion Library

  Provides a simple interface for platforms to have a list of firmware volumes
  excluded from measurement by the traditional TCG driver infrastructure.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FV_MEASUREMENT_EXCLUSION_LIB_H
#define FV_MEASUREMENT_EXCLUSION_LIB_H

#include <Ppi/FirmwareVolumeInfoMeasurementExcluded.h>

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
  );

#endif
