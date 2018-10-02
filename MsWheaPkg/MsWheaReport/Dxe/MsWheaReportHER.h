/** @file -- MsWheaReportHER.h

This header defines API that will save supplied payload via HwErrRec.

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

#ifndef __MS_WHEA_REPORT_HER__
#define __MS_WHEA_REPORT_HER__

#include "MsWheaReportCommon.h"

/**

Clear all the HwErrRec entries on flash.

@retval EFI_SUCCESS                     Entry addition is successful.
@retval Others                          See GetVariable/SetVariable for more details

**/
EFI_STATUS
EFIAPI
MsWheaClearAllEntries (
  VOID
);

/**

This routine accepts the pointer to the MS WHEA entry metadata, error specific data payload and its size 
then store on the flash as HwErrRec awaiting to be picked up by OS (Refer to UEFI Spec 2.7A)

@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error metadata
@param[in]  PayloadPtr                The pointer to reported error block payload, the content will be copied
@param[in]  PayloadSize               The size of reported error block payload

@retval EFI_SUCCESS                   Entry addition is successful.
@retval EFI_INVALID_PARAMETER         Input has NULL pointer as input.
@retval EFI_OUT_OF_RESOURCES          Not enough spcae for the requested space.

**/
EFI_STATUS
EFIAPI
MsWheaReportHERAdd (
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD,
  IN CONST VOID                       *PayloadPtr,
  IN UINT32                           PayloadSize
);

#endif // __MS_WHEA_REPORT_HER__
