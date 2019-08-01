/** @file -- DxePagingAuditApp.c
This Shell App writes page table and memory map information to SFS.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "../DxePagingAuditCommon.h"


/**
  SmmPagingAuditAppEntryPoint

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
PagingAuditDxeAppEntryPoint (
  IN     EFI_HANDLE         ImageHandle,
  IN     EFI_SYSTEM_TABLE  *SystemTable
)
{
  EFI_STATUS    Status = EFI_SUCCESS;
  DEBUG((DEBUG_ERROR, __FUNCTION__" entered - %r\n", Status));

  DumpPagingInfo (NULL, NULL);

  DEBUG((DEBUG_ERROR, __FUNCTION__" leave - %r\n", Status));

  return EFI_SUCCESS;
} // PagingAuditDxeAppEntryPoint()
