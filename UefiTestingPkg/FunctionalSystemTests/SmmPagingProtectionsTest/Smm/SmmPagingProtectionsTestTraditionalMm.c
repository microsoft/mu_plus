/** @file -- SmmPagingProtectionsTestSmm.c
This is the SMM portion of the SmmPagingProtectionsTest driver for traditional MM instance.
This driver will be signalled by the DXE portion and will perform requested operations
to probe the extent of the SMM memory protections (like NX).

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

// MS_CHANGE - Entire file created.

#include <PiSmm.h>
#include <Library/SmmMemLib.h>

#include "SmmPagingProtectionsTestDriver.h"

/**
  This function check if the buffer is valid per processor architecture and not overlap with SMRAM.

  @param Buffer  The buffer start address to be checked.
  @param Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid per processor architecture and not overlap with SMRAM.
  @retval FALSE This buffer is not valid per processor architecture or overlap with SMRAM.
**/
BOOLEAN
EFIAPI
IsBufferOutsideMmValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  return SmmIsBufferOutsideSmmValid (Buffer, Length);
}

/**
  The module Entry Point of the driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
TraditionalMmPagingProtectionsTestEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  return SmmPagingProtectionsTestInitialization ();
}
