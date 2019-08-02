/** @file
  This module installs Crypto protocols used by Project Mu Firmware

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Guid/EventGroup.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

#include "MuCryptoDxe.h"


/**
  The module Entry Point of the Project Mu Crypto Dxe Driver.  

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
MuCryptoDxeEntry(
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  
  InstallPkcs7Support(ImageHandle);

  InstallPkcs5Support(ImageHandle);

  return EFI_SUCCESS;
}
