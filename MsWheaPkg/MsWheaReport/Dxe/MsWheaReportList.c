/** @file -- MsWheaReportList.c

This source implements common methods to support logging of non-fatal Microsoft
WHEA errors in UEFI.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MsWheaReportList.h"

/**

This function creates a new entry to be added to the linked list

@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error linked list
@param[in]  PayloadPtr                The pointer to payload
@param[in]  PayloadSize               The pointer to payload length

@retval MS_WHEA_LIST_ENTRY *          Pointer to the created list entry.
@retval NULL                          Null pointer due to invalid parameter or out of resources.

**/
STATIC
MS_WHEA_LIST_ENTRY *
CreateNewEntry (
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
  )
{
  MS_WHEA_LIST_ENTRY *MsWheaListEntry = NULL;
  UINT32 Index = 0;

  // Input argument sanity check
  if (MsWheaEntryMD == NULL) {
    goto Cleanup;
  }

  MsWheaListEntry = AllocateZeroPool(sizeof(MS_WHEA_LIST_ENTRY));
  if (MsWheaListEntry == NULL) {
    goto Cleanup;
  }

  MsWheaListEntry->Signature = MS_WHEA_LIST_ENTRY_SIGNATURE;

  MsWheaListEntry->PayloadPtr = AllocateZeroPool(sizeof(MS_WHEA_ERROR_ENTRY_MD));
  if (MsWheaListEntry->PayloadPtr == NULL) {
    FreePool(MsWheaListEntry);
    MsWheaListEntry = NULL;
    goto Cleanup;
  }

  // Copy linked list and payload
  Index = 0;
  CopyMem(&((UINT8*)MsWheaListEntry->PayloadPtr)[Index], MsWheaEntryMD, sizeof(MS_WHEA_ERROR_ENTRY_MD));

  Index += sizeof(MS_WHEA_ERROR_ENTRY_MD);
  ((MS_WHEA_ERROR_ENTRY_MD*)MsWheaListEntry->PayloadPtr)->PayloadSize = Index;

  MsWheaListEntry->PayloadSize = Index;

Cleanup:
  return MsWheaListEntry;
}

/**

This routine accepts the MS WHEA linked list and the reported data and data length, then add the data to the
existed list, First In First Out (FIFO)

@param[in]  MsWheaLinkedList          Supplies a MS WHEA error linked list header.
@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error linked list

@retval EFI_SUCCESS                   Entry addition is successful.
@retval EFI_INVALID_PARAMETER         Input has NULL pointer as input.
@retval EFI_OUT_OF_RESOURCES          Not enough space for the requested space.

**/
EFI_STATUS
EFIAPI
MsWheaAddReportEvent(
  IN LIST_ENTRY                       *MsWheaLinkedList,
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
  )
{
  EFI_STATUS          Status;
  MS_WHEA_LIST_ENTRY  *MsWheaListEntry = NULL;

  // Caller has to supply valid pointers to linked list and data.
  if (MsWheaLinkedList == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  MsWheaListEntry = CreateNewEntry(MsWheaEntryMD);
  if (MsWheaListEntry == NULL) {
    // This error code may not be true, but something is knowingly wrong
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  // Added the initialized entry into the linked list
  InsertTailList(MsWheaLinkedList, &MsWheaListEntry->Link);

  Status = EFI_SUCCESS;

Cleanup:
  if (EFI_ERROR(Status)) {
    // Clean up the memory remainder, if any errors
    if ((MsWheaListEntry != NULL) && (MsWheaListEntry->PayloadPtr != NULL)) {
      FreePool(MsWheaListEntry->PayloadPtr);
    }
    if (MsWheaListEntry != NULL) {
      FreePool(MsWheaListEntry);
    }
  }
  return Status;
}

/**

This routine accepts the MS WHEA linked list, remove and free an element from the list

@param[in] MsWheaLinkedList           Supplies a MS WHEA linked list header.

@retval EFI_SUCCESS                   Entry addition is successful.
@retval EFI_INVALID_PARAMETER         Input has NULL pointer as input.

**/
EFI_STATUS
EFIAPI
MsWheaDeleteReportEvent(
  IN LIST_ENTRY                       *MsWheaLinkedList
  )
{
  EFI_STATUS          Status;
  MS_WHEA_LIST_ENTRY  *MsWheaListEntry = NULL;
  LIST_ENTRY          *HeadList = NULL;

  // Caller has to supply valid pointers to linked list.
  if (MsWheaLinkedList == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  // linked list needs to have content, if not, it shouldn't be an issue;
  if (IsListEmpty(MsWheaLinkedList) != FALSE) {
    Status = EFI_SUCCESS;
    goto Cleanup;
  }

  // Retrieve a temporary data holder
  HeadList = GetFirstNode(MsWheaLinkedList);
  if (HeadList == NULL) {
    // Shouldn't happen, since we've checked the linked list has more than one entry...
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }
  MsWheaListEntry = CR(HeadList, MS_WHEA_LIST_ENTRY, Link, MS_WHEA_LIST_ENTRY_SIGNATURE);

  // House keeping for the linked list
  RemoveEntryList(HeadList);
  if (MsWheaListEntry->PayloadPtr != NULL) {
    FreePool(MsWheaListEntry->PayloadPtr);
  }
  FreePool(MsWheaListEntry);

  Status = EFI_SUCCESS;

Cleanup:
  return Status;
}
