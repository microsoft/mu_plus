/** @file -- MsWheaErrorStatus.h

This header file defines MsWheaReport expected/applied header components.

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

#ifndef __MS_WHEA_ERROR_STATUS__ 
#define __MS_WHEA_ERROR_STATUS__ 

#include <Guid/Cper.h>
#include <Pi/PiStatusCode.h>

#define MS_WHEA_ERROR_SIGNATURE       SIGNATURE_32('W', 'H', 'E', 'A')

/**

 Accepted phase values

**/
#define MS_WHEA_PHASE_PEI             0x00
#define MS_WHEA_PHASE_DXE             0x01
#define MS_WHEA_PHASE_DXE_RUNTIME     0x02

/**

 Accepted revision values

**/
#define MS_WHEA_REV_0                 0x0000
#define MS_WHEA_REV_1                 0x0001
#define MS_WHEA_REV_WILDCARD          0x7FFF

#define MS_WHEA_ERROR_EARLY_STORAGE_STORE_FULL  \
                                      0xAFAFAFAF

/**

 Microsoft WHEA accepted error status type, other types will be ignored

**/
#define MS_WHEA_ERROR_STATUS_TYPE     (EFI_ERROR_MAJOR | EFI_ERROR_CODE)

typedef UINT16                        MS_WHEA_REV;
typedef UINT16                        MS_WHEA_ERROR_PHASE;

/**

 Minimal information reported for reported status codes under fatal severity (Rev0 and above).

 Note: All the upcoming Early Storage entry should start with the following parameters

 Rev:               Revision used for parser to identify supplied payload format.
 Phase:             Phase of boot process reporting module, will be filled at the backend.
 ErrorStatusCode:   Reported Status Code Value upon calling ReportStatusCode*

**/
typedef struct MS_WHEA_EARLY_STORAGE_ENTRY_V0_T_DEF {
  MS_WHEA_REV                         Rev;
  MS_WHEA_ERROR_PHASE                 Phase;
  UINT32                              ErrorStatusCode;
} MS_WHEA_EARLY_STORAGE_ENTRY_V0, MS_WHEA_EARLY_STORAGE_ENTRY_COMMON;

/**

 Minimal information reported for reported status codes under Rev 1 and fatal severity

 Rev:               Revision used for parser to identify supplied payload format.
 Phase:             Phase of boot process reporting module, will be filled at the backend.
 ErrorStatusCode:   Reported Status Code Value upon calling ReportStatusCode*
 CriticalInfo:      Critical information to be filled by caller
 ReporterID:        ReporterID to be filled by caller to reporter identification purpose

**/
typedef struct MS_WHEA_EARLY_STORAGE_ENTRY_V1_T_DEF {
  MS_WHEA_REV                         Rev;
  MS_WHEA_ERROR_PHASE                 Phase;
  UINT32                              ErrorStatusCode;
  UINT64                              CriticalInfo;
  UINT64                              ReporterID;
} MS_WHEA_EARLY_STORAGE_ENTRY_V1;

/**

 Microsoft WHEA error list entry header, to be included as error header when using
 ReportStatusCodeWithExtendedData.

 Signature:         'WHEA', indocator of WHEA compliance
 Rev:               Revision used for parser to identify supplied payload format.
 Phase:             Phase of boot process reporting module, will be filled at the backend.
 ErrorStatusCode:   Reported Status Code Value upon calling ReportStatusCode*
 CriticalInfo:      Critical information to be filled by caller, Ignored for Rev: 0 and wildcard
 ReporterID:        ReporterID to be filled by caller to reporter identification purpose, ignored for 
                    Rev: 0 and wildcard

**/
typedef struct MS_WHEA_ERROR_HDR_T_DEF {
  UINT32                              Signature;
  MS_WHEA_REV                         Rev;
  MS_WHEA_ERROR_PHASE                 Phase;
  UINT32                              ErrorSeverity;
  UINT32                              Reserved;
  UINT64                              CriticalInfo;
  UINT64                              ReporterID;
} MS_WHEA_ERROR_HDR;

#endif // __MS_WHEA_ERROR_STATUS__ 
