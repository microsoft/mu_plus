/** @file -- DxePagingAuditDriver.c
This DXE Driver writes page table and memory map information to SFS when triggered
by an event.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "../../PagingAuditCommon.h"

CHAR8  *mMemoryInfoDatabaseBuffer   = NULL;
UINTN  mMemoryInfoDatabaseSize      = 0;
UINTN  mMemoryInfoDatabaseAllocSize = 0;

/**
   Event notification handler. Will dump paging information to disk.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context

**/
VOID
EFIAPI
DumpPagingInfoEvent (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  DumpPagingInfo (NULL);
}

/**
  PagingAuditDriverEntryPoint

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
PagingAuditDriverEntryPoint (
  IN     EFI_HANDLE        ImageHandle,
  IN     EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  DEBUG ((DEBUG_ERROR, "%a registered - %r\n", __FUNCTION__, Status));
  EFI_EVENT  Event;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  DumpPagingInfoEvent,
                  NULL,
                  &gMuEventPreExitBootServicesGuid,
                  &Event
                  );
  DEBUG ((DEBUG_ERROR, "%a leave - %r\n", __FUNCTION__, Status));

  return EFI_SUCCESS;
} // PagingAuditDxeEntryPoint()
