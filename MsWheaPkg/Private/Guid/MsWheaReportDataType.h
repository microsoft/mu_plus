/** @file -- MsWheaErrorStatus.h

This header file defines MsWheaReport expected data structure.

Copyright (C) Microsoft Corporation. All rights reserved.

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
