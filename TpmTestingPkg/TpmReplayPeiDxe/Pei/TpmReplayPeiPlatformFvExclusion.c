/** @file
  TPM Replay PEI FV Exclusion - Main File

  Contains logic to exclude FVs for a platform so measurements can be made
  on a clean slate.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TpmReplayPeiPlatformFvExclusion.h"

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FvMeasurementExclusionLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/FirmwareVolumeInfoMeasurementExcluded.h>

/**
  Install PPIs for FVs that should be excluded from main TCG measurement flows.

  @param[in]  ExcludedFvs       Pointer to an array of EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_FV
  @param[in]  ExcludedFvsCount  Number of elements in ExcludedFvs

  @retval     EFI_SUCCESS             The function completed successfully and installed the exclusion list.
  @retval     EFI_INVALID_PARAMETER   ExcludedFvsCount is 0.
  @retval     EFI_OUT_OF_RESOURCES    Could not allocate memory for the new PPI.
  @retval     Others                  Unexpected error while creating and installing the exclusion list.
**/
EFI_STATUS
RegisterFvMeasurementExclusions (
  IN  CONST EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_FV  *ExcludedFvs,
  IN  UINTN                                                       ExcludedFvsCount
  )
{
  EFI_STATUS                                             Status;
  EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_PPI  *MeasurementExcludedFVsPpi;
  EFI_PEI_PPI_DESCRIPTOR                                 *MeasurementExcludedFVsPpiList;

  // The PPI requires at least one Excluded FV to make any sense.
  if (ExcludedFvsCount == 0) {
    ASSERT (ExcludedFvsCount > 0);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Allocate the Excluded FV PPI structure and the PPI List
  // Note that the PPI structure already includes one EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_FV
  // so subtract one in the allocation size arithmetic.
  //
  MeasurementExcludedFVsPpi = AllocatePool (
                                sizeof (EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_PPI) -
                                sizeof (EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_FV)  +
                                (sizeof (EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_FV) * ExcludedFvsCount)
                                );
  if (MeasurementExcludedFVsPpi == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  MeasurementExcludedFVsPpiList = AllocatePool (sizeof (EFI_PEI_PPI_DESCRIPTOR));
  if (MeasurementExcludedFVsPpiList == NULL) {
    FreePool (MeasurementExcludedFVsPpi);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Fill in the excluded FV info
  //
  MeasurementExcludedFVsPpi->Count = (UINT32)ExcludedFvsCount;
  CopyMem (
    &MeasurementExcludedFVsPpi->Fv,
    ExcludedFvs,
    ExcludedFvsCount * sizeof (EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_FV)
    );

  //
  // Fill in the PPI list info
  //
  MeasurementExcludedFVsPpiList->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  MeasurementExcludedFVsPpiList->Guid  = &gEfiPeiFirmwareVolumeInfoMeasurementExcludedPpiGuid;
  MeasurementExcludedFVsPpiList->Ppi   = MeasurementExcludedFVsPpi;

  //
  // Publish the PPI
  //
  Status = PeiServicesInstallPpi ((CONST EFI_PEI_PPI_DESCRIPTOR *)(MeasurementExcludedFVsPpiList));
  if (EFI_ERROR (Status)) {
    FreePool (MeasurementExcludedFVsPpiList);
    FreePool (MeasurementExcludedFVsPpi);
  }

  return Status;
}

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
  )
{
  EFI_STATUS                                                  Status;
  CONST EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_FV  *ExcludedFvs;
  UINTN                                                       ExcludedFvsCount;

  DEBUG ((DEBUG_ERROR, "[%a] - Entry\n", __FUNCTION__));

  Status = GetPlatformFvExclusions (&ExcludedFvs, &ExcludedFvsCount);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = RegisterFvMeasurementExclusions (ExcludedFvs, ExcludedFvsCount);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
