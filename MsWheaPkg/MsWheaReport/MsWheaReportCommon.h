/** @file -- MsWheaReportCommon.h

This header defines MsWheaReport related helper functions.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MS_WHEA_REPORT_COMMON__
#define __MS_WHEA_REPORT_COMMON__

#include <Guid/Cper.h>
#include <Guid/MsWheaReportDataType.h>
#include <Guid/MuTelemetryCperSection.h>
#include <Protocol/AcpiTable.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <MsWheaErrorStatus.h>

/************************************ Definition Section *************************************/

#define MS_WHEA_ERROR_SIGNATURE       SIGNATURE_32('W', 'H', 'E', 'A')

/**
Definition wrapper to unify all bert/hwerrrec related specification versioning
**/
#define MS_WHEA_SECTION_REVISION      0x0100 // Set Section Descriptor Revision to 1.0 as per UEFI Spec 2.7A

#define EFI_HW_ERR_REC_VAR_NAME       L"HwErrRec"
#define EFI_HW_ERR_REC_VAR_NAME_LEN   16      // Buffer length covers at least "HwErrRec####\0"

/**

 Accepted phase values

**/
#define MS_WHEA_PHASE_PEI             0x00
#define MS_WHEA_PHASE_DXE             0x01
#define MS_WHEA_PHASE_DXE_VAR         0x02
#define MS_WHEA_PHASE_SMM             0x03

#pragma pack(1)

/**
MS WHEA error entry metadata, used for intermediate data storage and preliminarily processed
raw data. All fields usage is the same as in their own header unless listed otherwise.
**/
typedef struct MS_WHEA_ERROR_ENTRY_MD_T_DEF {
  UINT8                               Rev;
  UINT8                               Phase;
  UINT16                              Reserved;
  UINT32                              ErrorSeverity;
  UINT32                              PayloadSize;
  EFI_STATUS_CODE_VALUE               ErrorStatusValue;
  UINT64                              AdditionalInfo1;
  UINT64                              AdditionalInfo2;
  EFI_GUID                            ModuleID;
  EFI_GUID                            LibraryID;
  EFI_GUID                            IhvSharingGuid;
} MS_WHEA_ERROR_ENTRY_MD;

#pragma pack()

/**
This routine accepts the Common Platform Error Record header and Section
Descriptor and correspthen store on the flash as HwErrRec awaiting to be picked up by OS
(Refer to UEFI Spec 2.7A)

@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error metadata

@retval EFI_SUCCESS                   Entry addition is successful.
@retval EFI_INVALID_PARAMETER         Input has NULL pointer as input.
@retval EFI_OUT_OF_RESOURCES          Not enough space for the requested space.
**/
typedef
EFI_STATUS
(EFIAPI *MS_WHEA_ERR_REPORT_PS_FN) (
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
);

/************************************ Function Section *************************************/

/**
This routine will fill CPER related headers with certain preset values;

Presets:  NotificationType: gEfiEventNotificationTypeBootGuid; SectionType: gMuTelemetrySectionTypeGuid;
Zeroed:   CPER Header: Flags, RecordID; Section Descriptor: SectionFlags, FruId, FruString;

@param[out] CperHdr                   Supplies a pointer to CPER header structure
@param[out] CperErrSecDscp            Supplies a pointer to CPER Section Decsriptor structure
@param[out] MuTelemetryData           Supplies a pointer to Mu telemetry data structure
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
  OUT MU_TELEMETRY_CPER_SECTION_DATA  *MuTelemetryData,
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD,
  IN UINT32                           PayloadSize
);

/**
This routine will filter status code based on status type, decode reported extended data , if any,
generate MS WHEA metadata based on decoded information and either 1. pass generated metadata to supplied
callback function for further processing or 2. store onto CMOS if boot phase and error severity meets
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
  IN UINT8                            CurrentPhase,
  IN MS_WHEA_ERR_REPORT_PS_FN         ReportFn
);

/**
Populates the current time for WHEA records

@param[in,out]  *CurrentTime              A pointer to an EFI_TIME variable which will contain the curren time after
                                          this function executes

@retval          BOOLEAN                  True if *CurrentTime was populated.
                                          False otherwise.
**/
BOOLEAN
PopulateTime(EFI_TIME* CurrentTime);

/**
Gets the Record ID variable and increments it for WHEA records

@param[in,out]  *RecordID                   Pointer to a UINT64 which will contain the record ID to be put on the next WHEA Record
@param[in]      *RecordIDGuid               Pointer to guid used to get the record ID variable

@retval          EFI_SUCCESS                The firmware has successfully stored the variable and its data as
                                            defined by the Attributes.
@retval          EFI_INVALID_PARAMETER      An invalid combination of attribute bits, name, and GUID was supplied, or the
                                            DataSize exceeds the maximum allowed.
@retval          EFI_INVALID_PARAMETER      VariableName is an empty string.
@retval          EFI_OUT_OF_RESOURCES       Not enough storage is available to hold the variable and its data.
@retval          EFI_DEVICE_ERROR           The variable could not be retrieved due to a hardware error.
@retval          EFI_WRITE_PROTECTED        The variable in question is read-only.
@retval          EFI_WRITE_PROTECTED        The variable in question cannot be deleted.
@retval          EFI_SECURITY_VIOLATION     The variable could not be written due to EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS being set,
                                            but the AuthInfo does NOT pass the validation check carried out by the firmware.
@retval          EFI_NOT_FOUND              The variable trying to be updated or deleted was not found.
**/
EFI_STATUS
GetRecordID(UINT64* RecordID,
            EFI_GUID *RecordIDGuid
);

#endif //__MS_WHEA_REPORT_COMMON__
