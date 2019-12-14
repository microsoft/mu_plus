/** @file -- MsWheaReportCommon.c

This source implements MsWheaReport helpers shared by both PEI and DXE phases.

The APIs include populating fields in Common Platform Error (CPER) related headers,
fill out timestamp based on CMOS, CMOS information storage, etc. The populated
headers can be used to report Microsoft WHEA to OS via Hardware Error Records (HwErrRec).

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ReportStatusCodeLib.h>
#include "MsWheaEarlyStorageMgr.h"
#include "MsWheaReportCommon.h"

#define CPER_HDR_SEC_CNT                      0x01

/************************************ Header Section *************************************/

/**
This routine will fill out the CPER header for caller.

Zeroed: Flags, PersistenceInfo;

@param[out] CperHdr                   Supplies a pointer to CPER header structure
@param[in]  PayloadSize               Length of entire payload to be included within this entry, refer to
                                      UEFI Spec for more information
@param[in]  ErrorSeverity             Error severity of this entry
@param[in]  ErrorStatusCode           Reported Status Code from ReportStatusCode* function
@param[in]  PartitionIdGuid           PartitionID for error source identification, default to
                                      zero guid if not supplied by caller

@retval EFI_SUCCESS                   The operation completed successfully
@retval EFI_INVALID_PARAMETER         Any required input pointer is NULL
**/
STATIC
EFI_STATUS
CreateCperHdrDefaultMin (
  OUT EFI_COMMON_ERROR_RECORD_HEADER  *CperHdr,
  IN UINT32                           PayloadSize,
  IN UINT32                           ErrorSeverity,
  IN UINT32                           ErrorStatusCode,
  IN EFI_GUID                         *PartitionIdGuid,
  IN UINT8                            ErrorPhase
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  EFI_TIME            CurrentTime;
  UINT64              RecordID;

  if (CperHdr == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  SetMem(CperHdr, sizeof(EFI_COMMON_ERROR_RECORD_HEADER), 0);

  CperHdr->SignatureStart = EFI_ERROR_RECORD_SIGNATURE_START;
  CperHdr->Revision = EFI_ERROR_RECORD_REVISION;
  CperHdr->SignatureEnd = EFI_ERROR_RECORD_SIGNATURE_END;
  // TODO: This will be updated accordingly when report ex is supported.
  CperHdr->SectionCount = CPER_HDR_SEC_CNT;
  CperHdr->ErrorSeverity = ErrorSeverity;
  CperHdr->ValidationBits = EFI_ERROR_RECORD_HEADER_PLATFORM_ID_VALID;
  CperHdr->RecordLength = (UINT32)(sizeof(EFI_COMMON_ERROR_RECORD_HEADER) + sizeof(EFI_ERROR_SECTION_DESCRIPTOR) + PayloadSize);

  if(PopulateTime(&CurrentTime)){
    CperHdr->ValidationBits    |= (EFI_ERROR_RECORD_HEADER_TIME_STAMP_VALID);
    CperHdr->TimeStamp.Seconds  = DecimalToBcd8(CurrentTime.Second);
    CperHdr->TimeStamp.Minutes  = DecimalToBcd8(CurrentTime.Minute);
    CperHdr->TimeStamp.Hours    = DecimalToBcd8(CurrentTime.Hour);
    CperHdr->TimeStamp.Day      = DecimalToBcd8(CurrentTime.Day);
    CperHdr->TimeStamp.Month    = DecimalToBcd8(CurrentTime.Month);
    CperHdr->TimeStamp.Year     = DecimalToBcd8(CurrentTime.Year % 100);
    CperHdr->TimeStamp.Century  = DecimalToBcd8((CurrentTime.Year / 100 + 1) % 100); // should not lose data
    if (ErrorPhase == MS_WHEA_PHASE_DXE_VAR) {
      CperHdr->TimeStamp.Flag   = BIT0;
    }
  } else{
    CperHdr->ValidationBits &= (~EFI_ERROR_RECORD_HEADER_TIME_STAMP_VALID);
  }

  CopyMem(&CperHdr->PlatformID, (EFI_GUID*)PcdGetPtr(PcdDeviceIdentifierGuid), sizeof(EFI_GUID));
  if (!IsZeroBuffer(PartitionIdGuid, sizeof(EFI_GUID))) {
    CperHdr->ValidationBits |= EFI_ERROR_RECORD_HEADER_PARTITION_ID_VALID;
  }
  CopyMem(&CperHdr->PartitionID, PartitionIdGuid, sizeof(EFI_GUID));

  // Default to MS WHEA Service guid
  CopyMem(&CperHdr->CreatorID, &gMsWheaReportServiceGuid, sizeof(EFI_GUID));

  // Default to Boot Error
  CopyMem(&CperHdr->NotificationType, &gEfiEventNotificationTypeBootGuid, sizeof(EFI_GUID));

  if(EFI_ERROR(GetRecordID(&RecordID, &gMsWheaReportRecordIDGuid))) {
    DEBUG ((DEBUG_INFO, "%a - RECORD ID NOT UPDATED\n", __FUNCTION__));
  }

  //Even if the record id was not updated, the value is either 0 or the previously incremented value
  CperHdr->RecordID = RecordID;
  CperHdr->Flags |= EFI_HW_ERROR_FLAGS_PREVERR;
  //CperHdr->PersistenceInfo = 0;// Untouched.
  //SetMem(&CperHdr->Resv1, sizeof(CperHdr->Resv1), 0); // Reserved field, should be 0.

Cleanup:
  return Status;
}

/**
This routine will fill out the CPER Section Descriptor for caller.

Zeroed: SectionFlags, FruId, FruString;

@param[out] CperErrSecDscp            Supplies a pointer to CPER header structure
@param[in]  PayloadSize               Length of entire payload to be included within this entry, refer to
                                      UEFI Spec for more information
@param[in]  ErrorSeverity             Error severity of this section

@retval EFI_SUCCESS                   The operation completed successfully
@retval EFI_INVALID_PARAMETER         Any required input pointer is NULL
**/
STATIC
EFI_STATUS
CreateCperErrSecDscpDefaultMin (
  OUT EFI_ERROR_SECTION_DESCRIPTOR    *CperErrSecDscp,
  IN UINT32                           PayloadSize,
  IN UINT32                           ErrorSeverity,
  IN EFI_GUID                         *SectionType  OPTIONAL
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  if (CperErrSecDscp == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  SetMem(CperErrSecDscp, sizeof(EFI_ERROR_SECTION_DESCRIPTOR), 0);

  CperErrSecDscp->SectionOffset = sizeof(EFI_COMMON_ERROR_RECORD_HEADER) + sizeof(EFI_ERROR_SECTION_DESCRIPTOR);
  CperErrSecDscp->SectionLength = (UINT32) PayloadSize;
  CperErrSecDscp->Revision = MS_WHEA_SECTION_REVISION;
  //CperErrSecDscp->SecValidMask = 0;

  //CperErrSecDscp->Resv1 = 0; // Reserved field, should be 0.
  //CperErrSecDscp->SectionFlags = 0; // Untouched.

  // Default to Mu telemetry error data
  CopyMem(&CperErrSecDscp->SectionType, &gMuTelemetrySectionTypeGuid, sizeof(EFI_GUID));

  //SetMem(&CperErrSecDscp->FruId, sizeof(CperErrSecDscp->FruId), 0); // Untouched.
  CperErrSecDscp->Severity = ErrorSeverity;
  //SetMem(CperErrSecDscp->FruString, sizeof(CperErrSecDscp->FruString), 0); // Untouched.

Cleanup:
  return Status;
}

/**
This routine will fill out the Mu Telemetry Error Data structure for caller.

@param[out] MuTelemetryData           Supplies a pointer to Mu Telemetry Error Data structure
@param[in]  MsWheaEntryMD             Internal telemetry metadata collected during error report

@retval EFI_SUCCESS                   The operation completed successfully
@retval EFI_INVALID_PARAMETER         Any required input pointer is NULL
**/
STATIC
EFI_STATUS
CreateMuTelemetryData (
  OUT MU_TELEMETRY_CPER_SECTION_DATA                 *MuTelemetryData,
  IN MS_WHEA_ERROR_ENTRY_MD                           *MsWheaEntryMD
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  if ((MuTelemetryData == NULL) ||
      (MsWheaEntryMD == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  SetMem(MuTelemetryData, sizeof(MU_TELEMETRY_CPER_SECTION_DATA), 0);

  MuTelemetryData->ComponentID = MsWheaEntryMD->ModuleID;
  MuTelemetryData->SubComponentID = MsWheaEntryMD->LibraryID;
  MuTelemetryData->ErrorStatusValue = MsWheaEntryMD->ErrorStatusValue;
  MuTelemetryData->AdditionalInfo1 = MsWheaEntryMD->AdditionalInfo1;
  MuTelemetryData->AdditionalInfo2 = MsWheaEntryMD->AdditionalInfo2;

Cleanup:
  return Status;
}

/**
This routine will populate certain MS WHEA metadata fields based on supplied information

@param[in]  Value                     Reported Status Code from ReportStatusCode* function
@param[in]  CurrentPhase              Supplies the boot phase the reporting module is in
@param[in]  ReportHdr                 Supplies the pointer to reported MS WHEA header, will be treated as
                                      wildcard if cannot decode signature
@param[in]  DataType                  Data type guid used during telemetry logging call. Will not be
                                      used if both ReportHdr and DataType are NULL
@param[in]  CallerId                  Supplies the CallerId, will be populated into CPER table CreatorID
                                      field if supplied, else default to gMsWheaReportServiceGuid
@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error metadata
**/
STATIC
EFI_STATUS
GenMsWheaEntryHeaderHelper (
  IN EFI_STATUS_CODE_VALUE            Value,
  IN UINT32                           ErrorSeverity,
  IN UINT8                            CurrentPhase,
  IN CONST VOID                       *ReportHdr OPTIONAL,
  IN CONST EFI_GUID                   *DataType OPTIONAL,
  IN CONST EFI_GUID                   *CallerId OPTIONAL,
  OUT MS_WHEA_ERROR_ENTRY_MD          *MsWheaEntryMD
  )
{
  EFI_STATUS            Status;

  if ((MsWheaEntryMD == NULL) ||
      ((ReportHdr != NULL) && (DataType == NULL))) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  SetMem(MsWheaEntryMD, sizeof(MS_WHEA_ERROR_ENTRY_MD), 0);

  // Sanity check first
  if (ReportHdr == NULL) {
    // Nothing to populate here
  }
  else if (!CompareMem(DataType, &gMsWheaRSCDataTypeGuid, sizeof(EFI_GUID))) {
    MS_WHEA_RSC_INTERNAL_ERROR_DATA *DataHeader;
    DataHeader = (MS_WHEA_RSC_INTERNAL_ERROR_DATA *) ReportHdr;
    CopyMem(&MsWheaEntryMD->LibraryID, &DataHeader->LibraryID, sizeof(EFI_GUID));
    CopyMem(&MsWheaEntryMD->IhvSharingGuid, &DataHeader->IhvSharingGuid, sizeof(EFI_GUID));
    MsWheaEntryMD->AdditionalInfo1 = DataHeader->AdditionalInfo1;
    MsWheaEntryMD->AdditionalInfo2 = DataHeader->AdditionalInfo2;
  }
  else {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  MsWheaEntryMD->ErrorSeverity = ErrorSeverity;
  MsWheaEntryMD->Rev = MS_WHEA_REV_0;
  MsWheaEntryMD->Phase = CurrentPhase;
  MsWheaEntryMD->ErrorStatusValue = Value;

  if (CallerId != NULL) {
    CopyMem(&MsWheaEntryMD->ModuleID, CallerId, sizeof(EFI_GUID));
  }

  Status = EFI_SUCCESS;

Cleanup:
  return Status;
}

/**
This routine will fill CPER related headers with certain preset values;

Presets:  NotificationType: gEfiEventNotificationTypeBootGuid; SectionType: gMuTelemetrySectionTypeGuid;
Zeroed:   CPER Header: Flags, RecordID; Section Descriptor: SectionFlags, FruId, FruString;

@param[out] CperHdr                   Supplies a pointer to CPER header structure
@param[out] CperErrSecDscp            Supplies a pointer to CPER Section Decsriptor structure
@param[out] MuTelemetryData           Supplies a pointer to Mu Telemetry Error Data structure
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
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  if ((CperHdr == NULL) ||
      (CperErrSecDscp == NULL) ||
      (MuTelemetryData == NULL) ||
      (MsWheaEntryMD == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  Status = CreateCperHdrDefaultMin(CperHdr,
                                  PayloadSize,
                                  MsWheaEntryMD->ErrorSeverity,
                                  MsWheaEntryMD->ErrorStatusValue,
                                  &MsWheaEntryMD->IhvSharingGuid,
                                  MsWheaEntryMD->Phase);
  if (EFI_ERROR(Status) != FALSE) {
    goto Cleanup;
  }

  Status = CreateCperErrSecDscpDefaultMin(CperErrSecDscp,
                                          PayloadSize,
                                          MsWheaEntryMD->ErrorSeverity,
                                          NULL);
  if (EFI_ERROR(Status) != FALSE) {
    goto Cleanup;
  }

  Status = CreateMuTelemetryData(MuTelemetryData, MsWheaEntryMD);
  if (EFI_ERROR(Status) != FALSE) {
    goto Cleanup;
  }

Cleanup:
  return Status;
}

/************************************ Reporter Section *************************************/

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
  )
{
  EFI_STATUS                          Status = EFI_SUCCESS;
  UINT32                              ErrorSeverity;
  MS_WHEA_ERROR_ENTRY_MD              MsWheaEntryMD;

  if (ReportFn == NULL) {
    DEBUG((DEBUG_ERROR, "%a: Input function pointer cannot be null!\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  if ((CodeType & MS_WHEA_ERROR_STATUS_TYPE_INFO) == MS_WHEA_ERROR_STATUS_TYPE_INFO) {
    ErrorSeverity = EFI_GENERIC_ERROR_INFO;
  } else if ((CodeType & MS_WHEA_ERROR_STATUS_TYPE_FATAL) == MS_WHEA_ERROR_STATUS_TYPE_FATAL) {
    ErrorSeverity = EFI_GENERIC_ERROR_FATAL;
  } else {
    Status = EFI_SUCCESS;
    goto Cleanup;
  }

  if ((Data != NULL) &&
      (Data->HeaderSize >= sizeof(EFI_STATUS_CODE_DATA)) &&
      (Data->Size == sizeof(MS_WHEA_RSC_INTERNAL_ERROR_DATA)) &&
      (!CompareMem(&Data->Type, &gMsWheaRSCDataTypeGuid, sizeof(EFI_GUID)))) {
    // Skip the EFI_STATUS_CODE_DATA in the beginning
    Status = GenMsWheaEntryHeaderHelper(Value,
                                        ErrorSeverity,
                                        CurrentPhase,
                                        (VOID*)(((UINT8*)Data) + Data->HeaderSize),
                                        &Data->Type,
                                        CallerId,
                                        &MsWheaEntryMD);
  }
  else if ((Data == NULL) || (Data->Size == 0)) {
    // No extra data reported
    Status = GenMsWheaEntryHeaderHelper(Value,
                                        ErrorSeverity,
                                        CurrentPhase,
                                        NULL,
                                        NULL,
                                        CallerId,
                                        &MsWheaEntryMD);
  }
  else {
    Status = EFI_UNSUPPORTED;
    goto Cleanup;
  }

  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: Failed to convert reported data to metadata blob - %r\n", __FUNCTION__, Status));
    goto Cleanup;
  }

  if ((ErrorSeverity == EFI_GENERIC_ERROR_FATAL) &&
      ((CurrentPhase == MS_WHEA_PHASE_DXE) || (CurrentPhase == MS_WHEA_PHASE_PEI))) {
    Status = MsWheaESStoreEntry(&MsWheaEntryMD);
    goto Cleanup;
  }

  Status = ReportFn(&MsWheaEntryMD);

Cleanup:
  return Status;
}
