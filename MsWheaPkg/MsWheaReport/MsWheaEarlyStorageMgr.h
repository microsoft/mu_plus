/** @file -- MsWheaEarlyStorageMgr.h

This header defines APIs to manipulate early storage for MsWheaReport usage.

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

#ifndef __MS_WHEA_EARLY_STORAGE_MGR__
#define __MS_WHEA_EARLY_STORAGE_MGR__

#include "MsWheaReportCommon.h"

/**

This routine initialized the Early Storage MS WHEA store.

**/
VOID
EFIAPI
MsWheaESInit (
  VOID
);

/**
This routine will store data onto Early Storage data region based on supplied MS WHEA metadata

@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error metadata

@retval EFI_SUCCESS                   Operation is successful
@retval Others                        See MsWheaStoreCMOSFindSlot and MsWheaCMOSStoreWriteData for more details
**/
EFI_STATUS
EFIAPI
MsWheaESStoreEntry (
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
);

/**
This routine processes the stored errors on Early Storage data region

@param[in]  ReportFn                  Callback function when a MS WHEA metadata is ready to report

@retval EFI_SUCCESS                   Operation is successful
@retval Others                        See implementation specific functions and MS_WHEA_ERR_REPORT_PS_FN 
                                      definition for more details
**/
EFI_STATUS
EFIAPI
MsWheaESProcess (
  IN MS_WHEA_ERR_REPORT_PS_FN         ReportFn
);

#endif // __MS_WHEA_EARLY_STORAGE_MGR__
