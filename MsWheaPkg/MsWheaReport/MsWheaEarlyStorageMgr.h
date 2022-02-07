/** @file -- MsWheaEarlyStorageMgr.h

This header defines APIs to manipulate early storage for MsWheaReport usage.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
  IN MS_WHEA_ERROR_ENTRY_MD  *MsWheaEntryMD
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
  IN MS_WHEA_ERR_REPORT_PS_FN  ReportFn
  );

#endif // __MS_WHEA_EARLY_STORAGE_MGR__
