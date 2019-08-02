/** @file -- MsWheaErrorStatus.h

This header file defines the Guid and data structure of Mu Telemetry section type
that will be used in Common Platform Error Record (CPER) for WHEA events.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MU_TELEMETRY_CPER_SECTION_DATA__ 
#define __MU_TELEMETRY_CPER_SECTION_DATA__ 

#define MU_TELEMETRY_SECTION_TYPE_GUID  \
  { \
    0x85183a8b, 0x9c41, 0x429c, { 0x93, 0x9c, 0x5c, 0x3c, 0x08, 0x7c, 0xa2, 0x80 } \
  }

extern EFI_GUID gMuTelemetrySectionTypeGuid;

#pragma pack(1)

/**

 Telemetry report structure. This structure corresponds to the gMuTelemetrySectionTypeGuid.
 This will contain the most essential information regarding the error of interest. Any
 other arbitrary data will not be reported/collected through telemetry pipeline

 Note: Please be compliant with privacy policy. Microsoft is not responsible for
       sanitizing the data being transported to cloud.

 ComponentID:         Component Guid which invoked telemetry report, will be gCallerId if not supplied.
 SubComponentID:      Subcomponent Guid which invoked telemetry report, will be NULL if not supplied.
 Reserved:            Not used
 ErrorStatusValue:    Reported Status Code Value upon calling ReportStatusCode
 AdditionalInfo1:     64 bit value used for caller to include necessary interrogative information
 AdditionalInfo2:     Secondary 64 bit value, usage same as AdditionalInfo1

**/
typedef struct {
  EFI_GUID                            ComponentID;
  EFI_GUID                            SubComponentID;
  UINT32                              Reserved;
  EFI_STATUS_CODE_VALUE               ErrorStatusValue;
  UINT64                              AdditionalInfo1;
  UINT64                              AdditionalInfo2;
} MU_TELEMETRY_CPER_SECTION_DATA;

#pragma pack()

#endif // __MU_TELEMETRY_CPER_SECTION_DATA__ 
