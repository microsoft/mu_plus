/** @file
  TPM Replay PEI FFS File Logic

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Guid/TpmReplayEventLog.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>

#include "TpmReplayInputChannelInternal.h"

/**
  Retrieves a TPM Replay Event Log from a FFS file.

  @param[out] Data            A pointer to a pointer to the buffer to hold the event log data.
  @param[out] DataSize        The size of the data placed in the data buffer.

  @retval    EFI_SUCCESS            The TPM Replay event log was returned successfully.
  @retval    EFI_INVALID_PARAMETER  A pointer argument given is NULL.
  @retval    EFI_UNSUPPORTED        The function is not implemented yet. The arguments are not used.
  @retval    EFI_COMPROMISED_DATA   The event log data found is not valid.
  @retval    EFI_NOT_FOUND          The event log data was not found in a FFS file.

**/
EFI_STATUS
GetTpmReplayEventLogFfsFile (
  OUT VOID   **Data,
  OUT UINTN  *DataSize
  )
{
  EFI_STATUS                 Status;
  EFI_STATUS                 FindFvStatus;
  UINTN                      FvInstance;
  EFI_PEI_FILE_HANDLE        FileHandle;
  EFI_PEI_FV_HANDLE          FvHandle;
  EFI_FV_INFO                FvInfo;
  EFI_COMMON_SECTION_HEADER  *SectionHeader;
  VOID                       *SectionData;

  if ((Data == NULL) || (DataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  for (FvInstance = 0, FindFvStatus = EFI_SUCCESS; !EFI_ERROR (FindFvStatus); FvInstance++) {
    FindFvStatus = PeiServicesFfsFindNextVolume (FvInstance, &FvHandle);
    if (!EFI_ERROR (FindFvStatus)) {
      ASSERT (((EFI_FIRMWARE_VOLUME_HEADER *)FvHandle)->Signature == EFI_FVH_SIGNATURE);

      Status = PeiServicesFfsGetVolumeInfo (FvHandle, &FvInfo);
      if (!EFI_ERROR (Status)) {
        DEBUG ((DEBUG_VERBOSE, "[%a] Current FV Name = %g\n", __FUNCTION__, &FvInfo.FvName));

        FileHandle = NULL;
        Status     = PeiServicesFfsFindFileByName (&gTpmReplayVendorGuid, FvHandle, &FileHandle);
        if (!EFI_ERROR (Status)) {
          Status = PeiServicesFfsFindSectionData (EFI_SECTION_RAW, FileHandle, &SectionData);
          if (!EFI_ERROR (Status)) {
            SectionHeader = (EFI_COMMON_SECTION_HEADER *)((UINTN)SectionData - sizeof (EFI_RAW_SECTION2));
            if (IS_SECTION2 (SectionHeader)) {
              *DataSize = SECTION2_SIZE (SectionHeader);
            } else {
              SectionHeader = (EFI_COMMON_SECTION_HEADER *)((UINTN)SectionData - sizeof (EFI_RAW_SECTION));
              *DataSize     = SECTION_SIZE (SectionHeader);
            }

            if (SectionHeader->Type != EFI_SECTION_RAW) {
              ASSERT (SectionHeader->Type == EFI_SECTION_RAW);
              return EFI_COMPROMISED_DATA;
            }

            *Data = SectionData;

            return EFI_SUCCESS;
          }
        }
      }
    }
  }

  return EFI_NOT_FOUND;
}
