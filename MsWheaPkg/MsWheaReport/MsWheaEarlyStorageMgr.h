/** @file -- MsWheaEarlyStorageMgr.h

This header defines APIs to manipulate early storage for MsWheaReport usage.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MS_WHEA_EARLY_STORAGE_MGR__
#define __MS_WHEA_EARLY_STORAGE_MGR__

#include "MsWheaReportCommon.h"

/**

 Accepted revision values

**/
#define MS_WHEA_REV_0                 0x00

/**

 Indicator for early storage header to be valid

**/
#define MS_WHEA_EARLY_STORAGE_SIGNATURE       SIGNATURE_32('M', 'E', 'S', '1')

#pragma pack(1)

/**

 Header of early storage, reserved for area sanity check, storage full check, etc.

 Signature:         Needs to be MS_WHEA_EARLY_STORAGE_SIGNATURE to indicate this is valid.
 IsStorageFull:     Indicator whether this early storage is full or not, if full, a error
                    report will be generated.
 FullPhase:         Phase of boot process when early storage is full.

**/
typedef struct _MS_WHEA_EARLY_STORAGE_HEADER {
  UINT32                              Signature;
  UINT32                              ActiveRange;
  UINT8                               IsStorageFull;
  UINT8                               FullPhase;
  UINT16                              Checksum;
  UINT32                              Reserved;
} MS_WHEA_EARLY_STORAGE_HEADER;

/**

 Minimal information reported for reported status codes under Rev 0 and fatal severity

 Rev:               Revision used for parser to identify supplied payload format.
 Phase:             Phase of boot process reporting module, will be filled at the backend.
 ErrorStatusValue:  Reported Status Code Value upon calling ReportStatusCode*
 AdditionalInfo1:   Critical information to be filled by caller
 AdditionalInfo2:   AdditionalInfo2 to be filled by caller, same usage as AdditionalInfo1
 PartitionID:       IHV Guid for the party reporting this error
 ModuleID:          Driver Guid that reports this error

**/
typedef struct MS_WHEA_EARLY_STORAGE_ENTRY_V0_T_DEF {
  UINT8                               Rev;
  UINT8                               Phase;
  UINT16                              Reserved;
  UINT32                              ErrorStatusValue;
  UINT64                              AdditionalInfo1;
  UINT64                              AdditionalInfo2;
  EFI_GUID                            ModuleID;
  EFI_GUID                            PartitionID;
} MS_WHEA_EARLY_STORAGE_ENTRY_V0, MS_WHEA_EARLY_STORAGE_ENTRY_COMMON;

#pragma pack()

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
