/*@file

Empty library file for NV Reboot Reason

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>                                     // UEFI base types

#include <Library/MsNVBootReasonLib.h>

/**
  Update secure boot violation

  @param[in]        RebootStatus  Reboot Status from BDS

  @retval  EFI_SUCCESS  Update secure boot violation successfully
  @retval  !EFI_SUCCESS Failed to update secure boot violation
**/
EFI_STATUS
UpdateSecureBootViolation (
  IN  EFI_STATUS    RebootStatus
) 
{
  return EFI_SUCCESS;
}

/**
  Set the Reboot Reason

  @param[in]        RebootStatus  Reboot Status from BDS

  @retval  EFI_SUCCESS  Set reboot reason successfully
  @retval  !EFI_SUCCESS Failed to set reboot reason
**/
EFI_STATUS
SetRebootReason (
  IN  EFI_STATUS     RebootStatus
) 
{
  return EFI_SUCCESS;
}

/**
  Remove reboot reason

  @retval  EFI_SUCCESS  Cleaned Reboot reason successfully
  @retval  !EFI_SUCCESS Failed to clean Reboot reason
**/
EFI_STATUS
EFIAPI
ClearRebootReason(
  VOID
)
{
  return EFI_SUCCESS;
}

/**
  Read reboot reason

  @param[out]       Buffer        Buffer to hold returned data
  @param[in, out]   BufferSize    Input as available data buffer size, output as data 
                                  size filled

  @retval  EFI_SUCCESS  Fetched version information successfully
  @retval  !EFI_SUCCESS Failed to fetch version information
**/
EFI_STATUS
EFIAPI
GetRebootReason(
      OUT UINT8                  *Buffer,          OPTIONAL
  IN  OUT UINTN                  *BufferSize
)
{
  return EFI_NOT_FOUND;
}

/**
  Get the current Reboot Reason and update based on OS entry to FrontPage

  @retval  EFI_SUCCESS  Updated reboot reason successfully
  @retval  !EFI_SUCCESS Failed to update reboot reason
**/
EFI_STATUS
EFIAPI
UpdateRebootReason (
  VOID
)
{
  return EFI_SUCCESS;
}
