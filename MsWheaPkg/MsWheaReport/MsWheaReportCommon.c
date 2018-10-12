/** @file -- MsWheaReportCommon.c

This source implements MsWheaReport helpers shared by both PEI and DXE phases.

The APIs include populating fields in Common Platform Error (CPER) related headers, 
fill out timestamp based on CMOS, CMOS information storage, etc. The populated 
headers can be used to report Microsoft WHEA to OS via Hardware Error Records (HwErrRec).

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

#include <Library/ReportStatusCodeLib.h>
#include "MsWheaEarlyStorageMgr.h"
#include "MsWheaReportCommon.h"

#define CPER_HDR_SEC_CNT                      0x01

#define MS_WHEA_IN_SITU_TEST_ERROR_CODE       0xA0A0A0A0
#define MS_WHEA_IN_SITU_TEST_ERROR_SIZE       0x40
#define MS_WHEA_IN_SITU_TEST_ERROR_SHORT_SIZE 0x08
#define MS_WHEA_IN_SITU_TEST_ERROR_PATTERN    0xC0
#define MS_WHEA_IN_SITU_TEST_ERROR_PHASE_BSFT 0x03
#define MS_WHEA_IN_SITU_TEST_ERROR_PI_BMSK    0x07
#define MS_WHEA_IN_SITU_TEST_ERROR_PATTERN    0xC0
#define MS_WHEA_IN_SITU_TEST_ERROR_REV_UNSUP  0x66
#define MS_WHEA_IN_SITU_TEST_ERROR_INFO       0x30303030
#define MS_WHEA_IN_SITU_TEST_ERROR_ID         0x50505050

/************************************ Header Section *************************************/

/**
This routine will fill out the CPER header for caller.

Zeroed: Flags, PersistenceInfo;

@param[out] CperHdr                   Supplies a pointer to CPER header structure 
@param[in]  PayloadSize               Length of entire payload to be included within this entry, refer to 
                                      UEFI Spec for more information
@param[in]  ErrorSeverity             Error severity of this entry
@param[in]  ErrorStatusCode           Reported Status Code from ReportStatucCode* function
@param[in]  RecordIDGuid              RecordID for error source identification, default to 
                                      gMsWheaReportServiceGuid if NULL
@param[in]  NotificationType          Notification type GUID of this entry, default to 
                                      gEfiEventNotificationTypeBootGuid if NULL

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
  IN EFI_GUID                         *RecordIDGuid       OPTIONAL,
  IN EFI_GUID                         *NotificationType   OPTIONAL
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  if (CperHdr == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  SetMem(CperHdr, sizeof(EFI_COMMON_ERROR_RECORD_HEADER), 0);

  CperHdr->SignatureStart = EFI_ERROR_RECORD_SIGNATURE_START;
  CperHdr->Revision = EFI_ERROR_RECORD_REVISION;
  CperHdr->SignatureEnd = EFI_ERROR_RECORD_SIGNATURE_END;
  CperHdr->SectionCount = CPER_HDR_SEC_CNT;
  CperHdr->ErrorSeverity = ErrorSeverity;
  CperHdr->ValidationBits = EFI_ERROR_RECORD_HEADER_PLATFORM_ID_VALID;
  CperHdr->RecordLength = (UINT32)(sizeof(EFI_COMMON_ERROR_RECORD_HEADER) + sizeof(EFI_ERROR_SECTION_DESCRIPTOR) + PayloadSize);

  CperHdr->ValidationBits &= (~EFI_ERROR_RECORD_HEADER_TIME_STAMP_VALID);
  CopyGuid(&CperHdr->PlatformID, (EFI_GUID*)PcdGetPtr(PcdDeviceIdentifierGuid));
  //SetMem(&CperHdr->PartitionID, sizeof(EFI_GUID), 0); // Not used, left zero.
  if (RecordIDGuid != NULL) {
    CopyGuid(&CperHdr->CreatorID, RecordIDGuid);
  } 
  else {
    // Default to MS WHEA Service guid
    CopyGuid(&CperHdr->CreatorID, &gMsWheaReportServiceGuid);
  }

  if (NotificationType) {
    CopyGuid(&CperHdr->NotificationType, NotificationType);
  } 
  else {
    // Default to Boot Error
    CopyGuid(&CperHdr->NotificationType, &gEfiEventNotificationTypeBootGuid);
  }
  
  // This is can be modified further
  CperHdr->RecordID = ErrorStatusCode;
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
@param[in]  ErrorSeverity             Error severity of this entry
@param[in]  SectionType               Section type GUID of this entry, default to 
                                      gEfiFirmwareErrorSectionGuid if NULL

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

  if (SectionType) {
    CopyGuid(&CperErrSecDscp->SectionType, SectionType);
  } 
  else {
    // Default to Firmware Error
    CopyGuid(&CperErrSecDscp->SectionType, &gEfiFirmwareErrorSectionGuid);
  }

  //SetMem(&CperErrSecDscp->FruId, sizeof(CperErrSecDscp->FruId), 0); // Untouched.
  CperErrSecDscp->Severity = ErrorSeverity;
  //SetMem(CperErrSecDscp->FruString, sizeof(CperErrSecDscp->FruString), 0); // Untouched.

Cleanup:
  return Status;
}

/**
This routine will fill out the Firmware Error Data header structure for caller.

@param[out] EfiFirmwareErrorData      Supplies a pointer to Firmware Error Data header structure
@param[in]  ErrorStatusCode           Reported Status Code from ReportStatucCode* function
@param[in]  RecordIDGuid              RecordID for error source identification, default to 
                                      gMsWheaReportServiceGuid if NULL

@retval EFI_SUCCESS                   The operation completed successfully
@retval EFI_INVALID_PARAMETER         Any required input pointer is NULL
**/
STATIC
EFI_STATUS
CreateFwErrDataDefaultMin (
  OUT EFI_FIRMWARE_ERROR_DATA                         *EfiFirmwareErrorData,
  IN UINT32                                           ErrorStatusCode,
  IN EFI_GUID                                         *RecordIDGuid       OPTIONAL
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  if (EfiFirmwareErrorData == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  SetMem(EfiFirmwareErrorData, sizeof(EFI_FIRMWARE_ERROR_DATA), 0);

  EfiFirmwareErrorData->ErrorType = EFI_FIRMWARE_ERROR_TYPE_SOC_TYPE2;
  EfiFirmwareErrorData->Revision = EFI_FIRMWARE_ERROR_REVISION;
  //SetMem(&EfiFirmwareErrorData->Resv1, sizeof(EfiFirmwareErrorData->Resv1), 0); // Reserved field.
  //EfiFirmwareErrorData->RecordId = 0; // Must be set to NULL

  if (RecordIDGuid != NULL) {
    CopyGuid(&EfiFirmwareErrorData->RecordIdGuid, RecordIDGuid);
  } 
  else {
    // Default to MS WHEA Service guid
    CopyGuid(&EfiFirmwareErrorData->RecordIdGuid, &gMsWheaReportServiceGuid);
  }

Cleanup:
  return Status;
}

/**
This routine will populate certain MS WHEA metadata fields based on supplied information

@param[in]  Value                     Reported Status Code from ReportStatucCode* function
@param[in]  CurrentPhase              Supplies the boot phase the reporting module is in
@param[in]  ReportHdr                 Supplies the pointer to reported MS WHEA header, will be treated as 
                                      wildcard if cannot decode signature
@param[in]  CallerId                  Supplies the CallerId, will be populated into CPER table CreatorID 
                                      field if supplied, else default to gMsWheaReportServiceGuid
@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error metadata
**/
STATIC
VOID
GenMsWheaEntryHeaderHelper (
  IN EFI_STATUS_CODE_VALUE            Value,
  IN MS_WHEA_ERROR_PHASE              CurrentPhase,
  IN CONST MS_WHEA_ERROR_HDR          *ReportHdr OPTIONAL,
  IN CONST EFI_GUID                   *CallerId OPTIONAL,
  OUT MS_WHEA_ERROR_ENTRY_MD          *MsWheaEntryMD
  )
{
  MS_WHEA_REV           Rev = 0;

  if (MsWheaEntryMD == NULL) {
    goto Cleanup;
  }

  SetMem(MsWheaEntryMD, sizeof(MS_WHEA_ERROR_ENTRY_MD), 0);
  
  // Sanity check first
  if ((ReportHdr == NULL) || (ReportHdr->Signature != MS_WHEA_ERROR_SIGNATURE)) {
    // Could not understand this payload, wild card and severity set to informational
    Rev = MS_WHEA_REV_WILDCARD;
    MsWheaEntryMD->MsWheaErrorHdr.ErrorSeverity = EFI_GENERIC_ERROR_INFO;
  }
  else {
    Rev = ReportHdr->Rev;
  }
  
  MsWheaEntryMD->ErrorStatusCode = Value;

  if (CallerId != NULL) {
    CopyGuid(&MsWheaEntryMD->CallerID, CallerId);
  }

  if (Rev == MS_WHEA_REV_WILDCARD) {
    MsWheaEntryMD->MsWheaErrorHdr.Signature = MS_WHEA_ERROR_SIGNATURE;
    MsWheaEntryMD->MsWheaErrorHdr.Rev = Rev;
  } 
  else {
    CopyMem(&MsWheaEntryMD->MsWheaErrorHdr, ReportHdr, sizeof(MS_WHEA_ERROR_HDR));
  }
  MsWheaEntryMD->MsWheaErrorHdr.Phase = CurrentPhase;

Cleanup:
  return;
}

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
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  
  if ((CperHdr == NULL) || 
      (CperErrSecDscp == NULL) || 
      (EfiFirmwareErrorData == NULL) || 
      (MsWheaEntryMD == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  Status = CreateCperHdrDefaultMin(CperHdr, 
                                  PayloadSize, 
                                  MsWheaEntryMD->MsWheaErrorHdr.ErrorSeverity, 
                                  MsWheaEntryMD->ErrorStatusCode, 
                                  &MsWheaEntryMD->CallerID, NULL);
  if (EFI_ERROR(Status) != FALSE) {
    goto Cleanup;
  }

  Status = CreateCperErrSecDscpDefaultMin(CperErrSecDscp, 
                                          PayloadSize, 
                                          MsWheaEntryMD->MsWheaErrorHdr.ErrorSeverity, 
                                          NULL);
  if (EFI_ERROR(Status) != FALSE) {
    goto Cleanup;
  }

  Status = CreateFwErrDataDefaultMin(EfiFirmwareErrorData, 
                                    MsWheaEntryMD->ErrorStatusCode, 
                                    &MsWheaEntryMD->CallerID);
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
  )
{
  EFI_STATUS                          Status = EFI_SUCCESS;
  UINT32                              DataSize;
  MS_WHEA_ERROR_ENTRY_MD              MsWheaEntryMD;
  VOID                                *TempPtr = NULL;

  if (ReportFn == NULL) {
    DEBUG((DEBUG_ERROR, __FUNCTION__ ": Input fucntion pointer cannot be null!\n"));
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }
  
  if ((CodeType & MS_WHEA_ERROR_STATUS_TYPE) != MS_WHEA_ERROR_STATUS_TYPE) {
    // Do Nothing for minor errors
    Status = EFI_SUCCESS;
    goto Cleanup;
  }
  
  if ((Data != NULL) && (Data->Size >= sizeof(MS_WHEA_ERROR_HDR))) {
    // Skip the EFI_STATUS_CODE_DATA in the beginning
    GenMsWheaEntryHeaderHelper(Value, 
                              CurrentPhase, 
                              (MS_WHEA_ERROR_HDR*)(Data + 1), 
                              CallerId, 
                              &MsWheaEntryMD);
  }
  else {
    // This is invoked by ReportStatusCode(), generated error header will be payload
    GenMsWheaEntryHeaderHelper(Value, 
                              CurrentPhase, 
                              NULL, 
                              CallerId, 
                              &MsWheaEntryMD);
  }

  if ((MsWheaEntryMD.MsWheaErrorHdr.ErrorSeverity == EFI_GENERIC_ERROR_FATAL) && 
      ((CurrentPhase == MS_WHEA_PHASE_DXE) || (CurrentPhase == MS_WHEA_PHASE_PEI))) {
    Status = MsWheaESStoreEntry(&MsWheaEntryMD);
    goto Cleanup;
  }
  
  if (Data == NULL) {
    // Wildcard report without extended data
    DataSize = sizeof(MS_WHEA_ERROR_HDR);
    TempPtr = AllocatePool(DataSize);
    if (TempPtr == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Cleanup;
    }
    CopyMem(TempPtr, &MsWheaEntryMD.MsWheaErrorHdr, DataSize);
  }
  else if ((Data->Size < sizeof(MS_WHEA_ERROR_HDR)) ||
          (((MS_WHEA_ERROR_HDR*)(Data + 1))->Signature != MS_WHEA_ERROR_SIGNATURE)) {
    // Wildcard report without formatted header
    DataSize = sizeof(MS_WHEA_ERROR_HDR) + Data->Size;
    TempPtr = AllocatePool(DataSize);
    if (TempPtr == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Cleanup;
    }
    CopyMem(TempPtr, &MsWheaEntryMD.MsWheaErrorHdr, sizeof(MS_WHEA_ERROR_HDR));
    CopyMem(&((UINT8*)TempPtr)[sizeof(MS_WHEA_ERROR_HDR)], Data + 1, Data->Size);
  }
  else {
    // Well-formatted data
    DataSize = Data->Size;
    TempPtr = AllocatePool(DataSize);
    if (TempPtr == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Cleanup;
    }
    CopyMem(TempPtr, Data + 1, Data->Size);
    // Overwrite the header since reporter might alter the data fields such as Phase
    CopyMem(TempPtr, &MsWheaEntryMD.MsWheaErrorHdr, sizeof(MS_WHEA_ERROR_HDR));
  }
  Status = ReportFn(&MsWheaEntryMD, TempPtr, DataSize);

Cleanup:
  if (TempPtr != NULL) {
    FreePool(TempPtr);
  }
  return Status;
}

/************************************ Test Section *************************************/

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
  )
{
  UINT8               Data[sizeof(MS_WHEA_ERROR_HDR) + MS_WHEA_IN_SITU_TEST_ERROR_SIZE];
  UINT32              Size = sizeof(MS_WHEA_ERROR_HDR) + MS_WHEA_IN_SITU_TEST_ERROR_SIZE;
  MS_WHEA_ERROR_HDR   *mMsWheaErrHdr;
  UINT8               TestIndex;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  SetMem(Data, sizeof(MS_WHEA_ERROR_HDR), 0);
  
  mMsWheaErrHdr = (MS_WHEA_ERROR_HDR*)Data;
  mMsWheaErrHdr->Signature = MS_WHEA_ERROR_SIGNATURE;
  mMsWheaErrHdr->CriticalInfo = MS_WHEA_IN_SITU_TEST_ERROR_INFO;
  mMsWheaErrHdr->ReporterID = MS_WHEA_IN_SITU_TEST_ERROR_ID;
  mMsWheaErrHdr->ErrorSeverity = EFI_GENERIC_ERROR_FATAL;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": Fatal error report rev 0...\n"));
  TestIndex = 0;
  SetMem((mMsWheaErrHdr + 1), 
        MS_WHEA_IN_SITU_TEST_ERROR_SIZE, 
        (UINT8)(MS_WHEA_IN_SITU_TEST_ERROR_PATTERN | 
                ((CurrentPhase<<MS_WHEA_IN_SITU_TEST_ERROR_PHASE_BSFT) + TestIndex)));
  mMsWheaErrHdr->Rev = MS_WHEA_REV_0;
  ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                  MS_WHEA_IN_SITU_TEST_ERROR_CODE,
                                  Data,
                                  Size);

  DEBUG((DEBUG_INFO, __FUNCTION__ ": Fatal error report rev 1...\n"));
  TestIndex ++;
  SetMem((mMsWheaErrHdr + 1), 
        MS_WHEA_IN_SITU_TEST_ERROR_SIZE, 
        (UINT8)(MS_WHEA_IN_SITU_TEST_ERROR_PATTERN | 
                ((CurrentPhase<<MS_WHEA_IN_SITU_TEST_ERROR_PHASE_BSFT) + TestIndex)));
  mMsWheaErrHdr->Rev = MS_WHEA_REV_1;
  ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                  MS_WHEA_IN_SITU_TEST_ERROR_CODE,
                                  Data,
                                  Size);

  DEBUG((DEBUG_INFO, __FUNCTION__ ": Fatal error report unsupported rev...\n"));
  TestIndex ++;
  SetMem((mMsWheaErrHdr + 1), 
        MS_WHEA_IN_SITU_TEST_ERROR_SIZE, 
        (UINT8)(MS_WHEA_IN_SITU_TEST_ERROR_PATTERN | 
                ((CurrentPhase<<MS_WHEA_IN_SITU_TEST_ERROR_PHASE_BSFT) + TestIndex)));
  mMsWheaErrHdr->Rev = MS_WHEA_IN_SITU_TEST_ERROR_REV_UNSUP;
  ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                  MS_WHEA_IN_SITU_TEST_ERROR_CODE,
                                  Data,
                                  Size);

  DEBUG((DEBUG_INFO, __FUNCTION__ ": Non-fatal error report unsupported rev...\n"));
  TestIndex ++;
  SetMem((mMsWheaErrHdr + 1), 
        MS_WHEA_IN_SITU_TEST_ERROR_SIZE, 
        (UINT8)(MS_WHEA_IN_SITU_TEST_ERROR_PATTERN | 
                ((CurrentPhase<<MS_WHEA_IN_SITU_TEST_ERROR_PHASE_BSFT) + TestIndex)));
  mMsWheaErrHdr->ErrorSeverity = EFI_GENERIC_ERROR_RECOVERABLE;
  ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                  MS_WHEA_IN_SITU_TEST_ERROR_CODE,
                                  Data,
                                  Size);

  DEBUG((DEBUG_INFO, __FUNCTION__ ": Non-fatal error report rev wildcard...\n"));
  TestIndex ++;
  SetMem((mMsWheaErrHdr + 1), 
        MS_WHEA_IN_SITU_TEST_ERROR_SIZE, 
        (UINT8)(MS_WHEA_IN_SITU_TEST_ERROR_PATTERN | 
                ((CurrentPhase<<MS_WHEA_IN_SITU_TEST_ERROR_PHASE_BSFT) + TestIndex)));
  ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                  MS_WHEA_IN_SITU_TEST_ERROR_CODE,
                                  (mMsWheaErrHdr + 1),
                                  MS_WHEA_IN_SITU_TEST_ERROR_SIZE);

  DEBUG((DEBUG_INFO, __FUNCTION__ ": Non-fatal error report rev short payload...\n"));
  TestIndex ++;
  SetMem((mMsWheaErrHdr + 1), 
        MS_WHEA_IN_SITU_TEST_ERROR_SHORT_SIZE, 
        (UINT8)(MS_WHEA_IN_SITU_TEST_ERROR_PATTERN | 
                ((CurrentPhase<<MS_WHEA_IN_SITU_TEST_ERROR_PHASE_BSFT) + TestIndex)));
  ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                  MS_WHEA_IN_SITU_TEST_ERROR_CODE,
                                  (mMsWheaErrHdr + 1),
                                  MS_WHEA_IN_SITU_TEST_ERROR_SHORT_SIZE);

  DEBUG((DEBUG_INFO, __FUNCTION__ ": Non-fatal error report rev short...\n"));
  TestIndex ++;
  ReportStatusCode((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                  MS_WHEA_IN_SITU_TEST_ERROR_CODE);

  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...\n"));
}
