/** @file -- MsWheaReportCommon.h

This header defines MsWheaReport related helper functions.

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

#ifndef __MS_WHEA_REPORT_COMMON__
#define __MS_WHEA_REPORT_COMMON__

#include <Guid/Cper.h>
#include <Protocol/AcpiTable.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <MsWheaErrorStatus.h>

/************************************ Definition Section *************************************/

/**
Definition wrapper to unify all bert/hwerrrec related specification versioning
**/
#define EFI_FIRMWARE_ERROR_REVISION   0x0002 // Set Firmware Error Record Revision to 2 as per UEFI Spec 2.7A
#define MS_WHEA_SECTION_REVISION      0x0100 // Set Section Descriptor Revision to 1.0 as per UEFI Spec 2.7A 

#define EFI_HW_ERR_REC_VAR_NAME       L"HwErrRec"
#define EFI_HW_ERR_REC_VAR_NAME_LEN   16      // Buffer length covers at least "HwErrRec####\0"

/**
MS WHEA error entry metadata:

MsWheaErrorHdr:     MS WHEA error header, either from payload or self-generated if wildcard.
PayloadSize:        Payload size of this error, when used for hob/linked list operation, including a MS WHEA
                    metadata and a well formatted payload (starting with a MS_WHEA_ERROR_HDR, followed by raw
                    payload).
ErrorStatusCode:    Reported Status Code value from ReportStatusCode*
CallerID:           Caller ID for identification purpose
**/
typedef struct MS_WHEA_ERROR_ENTRY_MD_T_DEF {
  MS_WHEA_ERROR_HDR                   MsWheaErrorHdr;
  UINT32                              PayloadSize;
  UINT32                              ErrorStatusCode;
  EFI_GUID                            CallerID;
} MS_WHEA_ERROR_ENTRY_MD;

/**
This routine accepts the Common Platform Error Record header and Section 
Descriptor and correspthen store on the flash as HwErrRec awaiting to be picked up by OS
(Refer to UEFI Spec 2.7A)

@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error metadata
@param[in]  PayloadPtr                The pointer to reported error block, the content will be copied
@param[in]  PayloadSize               The size of a well formatted payload (starting with a 
                                      MS_WHEA_ERROR_HDR, followed by raw payload)

@retval EFI_SUCCESS                   Entry addition is successful.
@retval EFI_INVALID_PARAMETER         Input has NULL pointer as input.
@retval EFI_OUT_OF_RESOURCES          Not enough spcae for the requested space.
**/
typedef
EFI_STATUS
(EFIAPI *MS_WHEA_ERR_REPORT_PS_FN) (
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD,
  IN VOID                             *PayloadPtr,
  IN UINT32                           PayloadSize
);

/************************************ Function Section *************************************/

/**
This routine will fill CPER related headers with certain preset values;

Presets:  NotificationType: gEfiEventNotificationTypeBootGuid; SectionType: gEfiFirmwareErrorSectionGuid;
Zeroed:   CPER Header: Flags, RecordID; Section Descriptor: SectionFlags, FruId, FruString;

@param[out] CperHdr                   Supplies a pointer to CPER header structure 
@param[out] CperErrSecDscp            Supplies a pointer to CPER Section Decsriptor structure
@param[out] EfiFirmwareErrorData      Supplies a pointer to Firmware Error Data header structure
@param[in]  MsWheaEntryMD             Supplies a pointer to reported MS WHEA error metadata
@param[in]  PayloadSize               Length of entire payload to be included within this entry, refer to 
                                      UEFI Spec for more information

@retval EFI_SUCCESS                   The operation completed successfully
@retval EFI_INVALID_PARAMETER         Any required input pointer is NULL
**/
EFI_STATUS 
EFIAPI
CreateHeadersDefault (
  OUT EFI_COMMON_ERROR_RECORD_HEADER  *CperHdr,
  OUT EFI_ERROR_SECTION_DESCRIPTOR    *CperErrSecDscp,
  OUT EFI_FIRMWARE_ERROR_DATA         *EfiFirmwareErrorData,
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD,
  IN UINT32                           PayloadSize
);

/**
This routine will filter status code based on status type, decode reported extended data , if any, 
generate MS WHEA metadata based on decoded information and either 1. pass generated metadata to supplied
callback function for further processing or 2. store onto CMOS if boot phase and error severtiy meets 
certain requirements

@param[in]  CodeType                  Indicates the type of status code being reported.
@param[in]  Value                     Describes the current status of a hardware or software entity. This
                                      includes information about the class and subclass that is used to 
                                      classify the entity as well as an operation.
@param[in]  Instance                  The enumeration of a hardware or software entity within the system. 
                                      Valid instance numbers start with 1.
@param[in]  CallerId                  This optional parameter may be used to identify the caller. This 
                                      parameter allows the status code driver to apply different rules to 
                                      different callers.
@param[in]  Data                      This optional parameter may be used to pass additional data.
@param[in]  CurrentPhase              This is passed by the RSC handler by indicating what to do next 
                                      (store critical information on the CMOS)
@param[in]  ReportFn                  Function pointer collect input argument and save to HwErrRec for OS 
                                      to process 

@retval EFI_SUCCESS                   The operation completed successfully
@retval EFI_INVALID_PARAMETER         Any required input pointer is NULL
@retval Others                        Any other error that rises from Variable Services, Boot Services, 
                                      Runtime Services, etc.
**/
EFI_STATUS
EFIAPI
ReportHwErrRecRouter (
  IN EFI_STATUS_CODE_TYPE             CodeType,
  IN EFI_STATUS_CODE_VALUE            Value,
  IN UINT32                           Instance,
  IN CONST EFI_GUID                   *CallerId,
  IN CONST EFI_STATUS_CODE_DATA       *Data OPTIONAL,
  IN MS_WHEA_ERROR_PHASE              CurrentPhase,
  IN MS_WHEA_ERR_REPORT_PS_FN         ReportFn
);

/**
This routine will create a few ReportStatusCode calls to test the implementation of backend service.
1. This function should be invoked after evaluating gMsWheaPkgTokenSpaceGuid.PcdMsWheaReportTestEnable;
2. Only the modules creating backend services should use this function;

@param[out] CurrentPhase              Supplies the boot phase the reporting module is in
**/
VOID
EFIAPI
MsWheaInSituTest(
  IN MS_WHEA_ERROR_PHASE              CurrentPhase
);

#endif //__MS_WHEA_REPORT_COMMON__
