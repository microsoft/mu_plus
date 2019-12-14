/** @file -- DxePagingAuditApp.c
This DXE Driver writes page table and memory map information to SFS when triggered
by an event.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "../DxePagingAuditCommon.h"


/**
  SmmPagingAuditAppEntryPoint

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
PagingAuditDxeEntryPoint (
  IN     EFI_HANDLE         ImageHandle,
  IN     EFI_SYSTEM_TABLE  *SystemTable
)
{
  EFI_STATUS    Status = EFI_SUCCESS;
  DEBUG((DEBUG_ERROR, __FUNCTION__" registered - %r\n", Status));
  EFI_EVENT                         Event;

  Status = gBS->CreateEventEx (
                      EVT_NOTIFY_SIGNAL,
                      TPL_CALLBACK,
                      DumpPagingInfo,
                      NULL,
                      &gEfiEventExitBootServicesGuid,
                      &Event
                      );
  // // DumpPagingInfo();
  DEBUG((DEBUG_ERROR, __FUNCTION__" leave - %r\n", Status));

  return EFI_SUCCESS;
} // PagingAuditDxeEntryPoint()
