/** @file -- MsWheaErrorStatus.h

This header file defines MsWheaReport expected data structure.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MS_WHEA_REPORT_DATA_TYPE__ 
#define __MS_WHEA_REPORT_DATA_TYPE__ 

#define MS_WHEA_RSC_DATA_TYPE \
  { \
    0x91deea05, 0x8c0a, 0x4dcd, { 0xb9, 0x1e, 0xf2, 0x1c, 0xa0, 0xc6, 0x84, 0x5 } \
  }

extern EFI_GUID gMsWheaRSCDataTypeGuid;

#pragma pack(1)

/**
 Internal RSC Extended Data Buffer format used by Project Mu firmware WHEA infrastructure.

 A Buffer of this format should be passed to ReportStatusCodeWithExtendedData

 LibraryID:         GUID of the library reporting the error. If not from a library use zero guid
 IhvSharingGuid:    GUID of the partner to share this with. If none use zero guid
 AdditionalInfo1:   64 bit value used for caller to include necessary interrogative information
 AdditionalInfo2:   64 bit value used for caller to include necessary interrogative information
**/
typedef struct {
 EFI_GUID           LibraryID;
 EFI_GUID           IhvSharingGuid;
 UINT64             AdditionalInfo1;
 UINT64             AdditionalInfo2;
} MS_WHEA_RSC_INTERNAL_ERROR_DATA;

#pragma pack()

#endif // __MS_WHEA_REPORT_DATA_TYPE__ 
