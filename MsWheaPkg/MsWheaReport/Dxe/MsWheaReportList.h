/** @file -- MsWheaReportList.h

This header defines API that will save supplied payload on heap for pre-ExitBoot usage.

Copyright (c) 2018, Microsoft Corporation

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
@retval EFI_OUT_OF_RESOURCES          Not enough spcae for the requested space.

**/
EFI_STATUS
EFIAPI
MsWheaAddReportEvent(
  IN LIST_ENTRY                       *MsWheaLinkedList,
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD,
  IN CONST VOID                       *PayloadPtr,
  IN UINT32                           PayloadSize
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
