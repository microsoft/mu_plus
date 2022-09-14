/** @file -- DxePagingAuditTestApp.c
This Shell App writes page table and memory map information to SFS.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "../../PagingAuditCommon.h"

CHAR8  *mMemoryInfoDatabaseBuffer   = NULL;
UINTN  mMemoryInfoDatabaseSize      = 0;
UINTN  mMemoryInfoDatabaseAllocSize = 0;

/**
  DxePagingAuditTestAppEntryPoint

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
DxePagingAuditTestAppEntryPoint (
  IN     EFI_HANDLE        ImageHandle,
  IN     EFI_SYSTEM_TABLE  *SystemTable
  )
{
  DumpPagingInfo (NULL, NULL);

  return EFI_SUCCESS;
} // DxePagingAuditTestAppEntryPoint()
