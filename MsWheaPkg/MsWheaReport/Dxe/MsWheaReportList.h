/** @file -- MsWheaReportList.h

This header defines API that will save supplied payload on heap for pre-ExitBoot usage.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MS_WHEA_REPORT_LIST__
#define __MS_WHEA_REPORT_LIST__

#include "MsWheaReportCommon.h"

#define MS_WHEA_LIST_ENTRY_SIGNATURE  MS_WHEA_ERROR_SIGNATURE

// The linked list node to recover necessary information from each error block
typedef struct MS_WHEA_LIST_ENTRY_T_DEF {
  UINT32                              Signature;
  UINT32                              PayloadSize;
  VOID                                *PayloadPtr;
  LIST_ENTRY                          Link;
} MS_WHEA_LIST_ENTRY;

/**

This routine accepts the MS WHEA metadata and the reported data and data length, then add the data to the
existed list, First In First Out (FIFO)

@param[in]  MsWheaLinkedList          Supplies a MS WHEA error linked list header.
@param[in]  PayloadPtr                The pointer to reported error block, the content will be copied.
@param[in]  PayloadSize               The size of reported error block.

@retval EFI_SUCCESS                   Entry addition is successful.
@retval EFI_INVALID_PARAMETER         Input has NULL pointer as input.
@retval EFI_OUT_OF_RESOURCES          Not enough space for the requested space.

**/
EFI_STATUS
EFIAPI
MsWheaAddReportEvent(
  IN LIST_ENTRY                       *MsWheaLinkedList,
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
);

/**

This routine accepts the MS WHEA metadata, remove and free an element from the list

@param[in] MsWheaLinkedList           Supplies a MS WHEA linked list header.

@retval EFI_SUCCESS                   Entry addition is successful.
@retval EFI_INVALID_PARAMETER         Input has NULL pointer as input.

**/
EFI_STATUS
EFIAPI
MsWheaDeleteReportEvent(
  IN LIST_ENTRY                       *MsWheaLinkedList
);

#endif // __MS_WHEA_REPORT_LIST__
