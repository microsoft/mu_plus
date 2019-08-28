/** @file -- MemoryTypeInformationChangeLib.c
 Reports the memory type information change as a telemetry event.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/MuTelemetryHelperLib.h>
#include <Pi/PiStatusCode.h>

/**
  Report the change in memory type allocations.
  This implementation reports the change as a telemetry event.
  
  @param[in] Type                   EFI memory type defined in UEFI specification
  @param[in] PreviousNumberOfPages  Number of pages retrieved from
                                    gEfiMemoryTypeInformationGuid HOB
  @param[in] NextNumberOfPages      Number of pages calculated to be used on next boot
  
  @retval    Status                 Status of the report
**/
RETURN_STATUS
ReportMemoryTypeInformationChange (
  IN UINT32                Type,
  IN UINT32                PreviousNumberOfPages,
  IN UINT32                NextNumberOfPages
  )
{
  //
  // This telemetry event is logged by a lib linked to BdsDxe, thus a EFI_SOFTWARE_DXE_BS_DRIVER
  // subclass; the class EFI_SW_EC_MEMORY_TYPE_INFORMATION_CHANGE is custom for this event.
  //
  return LogTelemetry (TRUE,
                       NULL,
                       (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_EC_MEMORY_TYPE_INFORMATION_CHANGE),
                       NULL,
                       NULL,
                       Type,
                       PreviousNumberOfPages + (((UINT64) NextNumberOfPages) << 32));
}
