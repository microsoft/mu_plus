/** @file
  TPM Replay DXE - Main Module File

  Defines the entry point and main execution flow.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

/**
  Driver entry point.

  This function controls the main execution flow of this module.

  @param[in] ImageHandle    Handle for the image of this driver
  @param[in] SystemTable    Pointer to the EFI System Table

  @retval    EFI_SUCCESS    This entry point always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
TpmReplayDxeEntryPoint (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
