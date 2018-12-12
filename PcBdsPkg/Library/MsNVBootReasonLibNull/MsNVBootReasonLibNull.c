/*@file

Empty library file for NV Reboot Reason

Copyright (c) 2015 - 2018, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
