/** @file -- MsWheaReportUnitTestApp.c

Tests for MS WHEA report tests with various payloads and error severities.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Pi/PiStatusCode.h>
#include <Guid/Cper.h>
#include <Guid/MsWheaReportDataType.h>
#include <Guid/MuTelemetryCperSection.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UnitTestLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PcdLib.h>
#include <MsWheaErrorStatus.h>
#include <Library/MuTelemetryHelperLib.h>


#define UNIT_TEST_APP_NAME            "MsWhea Report Test"
#define UNIT_TEST_APP_VERSION         "0.2"

// Copy from MsWheaReportCommon.h, will fail test if any mismatch
#define EFI_HW_ERR_REC_VAR_NAME       L"HwErrRec"
#define EFI_HW_ERR_REC_VAR_NAME_LEN   16      // Buffer length covers at least "HwErrRec####\0"
#define CPER_HDR_SEC_CNT              0x01
#define EFI_FIRMWARE_ERROR_REVISION   0x0002  // Set Firmware Error Record Revision to 2 as per UEFI Spec 2.7A

#define UNIT_TEST_SINGLE_ROUND        2
#define UNIT_TEST_ERROR_CODE          0xA0A0A0A0
#define UNIT_TEST_ERROR_SIZE          0x100
#define UNIT_TEST_ERROR_SHORT_SIZE    (sizeof(MS_WHEA_RSC_INTERNAL_ERROR_DATA) >> 1) // Half of the header size
#define UNIT_TEST_ERROR_PATTERN       0x30
#define UNIT_TEST_ERROR_INFO1         0xC0C0C0C0
#define UNIT_TEST_ERROR_INFO2         0x50505050

#define MS_WHEA_REV_UNSUPPORTED       0x66

#define HW_ERR_REC_HEADERS_OFFSET     (sizeof(EFI_COMMON_ERROR_RECORD_HEADER) + \
                                       sizeof(EFI_ERROR_SECTION_DESCRIPTOR))
#define HW_ERR_REC_PAYLOAD_OVERHEAD   (HW_ERR_REC_HEADERS_OFFSET + \
                                       sizeof(MU_TELEMETRY_CPER_SECTION_DATA))

typedef enum TEST_ID_T_DEF {
  TEST_ID_FATAL_EX,
  TEST_ID_NON_FATAL_EX,
  TEST_ID_WILDCARD,
  TEST_ID_SHORT,
  TEST_ID_STRESS,
  TEST_ID_BOUNDARY,
  TEST_ID_VARSEV,
  TEST_ID_TPL,

  TEST_ID_COUNT
} TEST_ID;

typedef struct MS_WHEA_TEST_CONTEXT_T_DEF {
  UINT32                              TestId;
  UINT32                              Reserved;
} MS_WHEA_TEST_CONTEXT;

///================================================================================================
///================================================================================================
///
/// HELPER FUNCTIONS
///
///================================================================================================
///================================================================================================

/**
  CPER Header verification routine, between flash entry and expect based on input payload

  @param[in] Context                  Test context applied for this test case
  @param[in] CperHdr                  CPER Header read from a specific HwErrRec entry
  @param[in] ErrorSeverity            Error Severity from input payload
  @param[in] ErrorStatusCodeValue     Error Status Code Value passed during ReportStatusCode*
  @param[in] TotalSize                Total size of this specific HwErrRec entry

  @retval EFI_SUCCESS                 The entry point executed successfully.
  @retval EFI_INVALID_PARAMETER       Null pointer detected.
  @retval EFI_PROTOCOL_ERROR          Parameter mismatches during validation.
  @retval EFI_BAD_BUFFER_SIZE         Read buffer does match the data size field in the header.

**/
STATIC
EFI_STATUS
MsWheaVerifyCPERHeader (
  IN UNIT_TEST_CONTEXT                Context,
  IN EFI_COMMON_ERROR_RECORD_HEADER   *CperHdr,
  IN UINT32                           ErrorSeverity,
  IN EFI_GUID                         *PartitionId,
  IN UINT32                           TotalSize
  )
{
  EFI_STATUS            Status;

  if (CperHdr == NULL) {
    Status = EFI_INVALID_PARAMETER;
    UT_LOG_ERROR( "CPER Header Null pointer exception.");
    goto Cleanup;
  }

  if (CperHdr->SignatureStart != EFI_ERROR_RECORD_SIGNATURE_START) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Header Singnature Start mismatch: %08X.", CperHdr->SignatureStart);
    goto Cleanup;
  }

  if (CperHdr->Revision != EFI_ERROR_RECORD_REVISION) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Header Revision mismatch: %04X.", CperHdr->Revision);
    goto Cleanup;
  }

  if (CperHdr->SignatureEnd != EFI_ERROR_RECORD_SIGNATURE_END) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Header Singnature End mismatch: %08X.", CperHdr->SignatureEnd);
    goto Cleanup;
  }
  // If it breaks here, means count has changed for some reason, which is the point of this test
  if (CperHdr->SectionCount != CPER_HDR_SEC_CNT) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Header section count mismatch: has: %d, expect: %d.",
                  CperHdr->SectionCount,
                  CPER_HDR_SEC_CNT);
    goto Cleanup;
  }

  if (CperHdr->ErrorSeverity != ErrorSeverity) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Header error severity mismatch: has: %d, expect: %d.",
                  CperHdr->ErrorSeverity,
                  ErrorSeverity);
    goto Cleanup;
  }

  if (((CperHdr->ValidationBits & EFI_ERROR_RECORD_HEADER_PLATFORM_ID_VALID) == 0) ||
      (CompareMem(&CperHdr->PlatformID, PcdGetPtr(PcdDeviceIdentifierGuid), sizeof(EFI_GUID)))) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Header Creator Id incorrect: has: %g, validation bits: %X.",
                  CperHdr->ValidationBits,
                  CperHdr->PlatformID);
    goto Cleanup;
  }

  if ((PartitionId != NULL) &&
      (CompareMem(PartitionId, &CperHdr->PartitionID, sizeof(EFI_GUID)) ||
       ((CperHdr->ValidationBits & EFI_ERROR_RECORD_HEADER_PARTITION_ID_VALID) == 0))) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Header Partition Id incorrect: has Guid: %g, validation bits: %X.",
                  CperHdr->PartitionID,
                  CperHdr->ValidationBits);
    goto Cleanup;
  }

  if (CperHdr->RecordLength != TotalSize) {
    Status = EFI_BAD_BUFFER_SIZE;
    UT_LOG_ERROR( "CPER Header record length incorrect: has: %08X, expect: %08X.",
                  CperHdr->RecordLength,
                  TotalSize);
    goto Cleanup;
  }

  if (CompareGuid(&CperHdr->CreatorID, &gMsWheaReportServiceGuid) == FALSE) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Header Creator ID mismatch: has: %g, expect: %g.",
                  CperHdr->CreatorID,
                  &gMsWheaReportServiceGuid);
    goto Cleanup;
  }

  if (CompareGuid(&CperHdr->NotificationType, &gEfiEventNotificationTypeBootGuid) == FALSE) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Header Notification Type mismatch: has: %g, expect: %g.",
                  CperHdr->NotificationType,
                  &gEfiEventNotificationTypeBootGuid);
    goto Cleanup;
  }

  // TODO: Needs to be checked when CperHdr->RecordID is implemented

  if (CperHdr->Flags != EFI_HW_ERROR_FLAGS_PREVERR) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Header Error Flags mismatch: has: %08X, expect: %08X.",
                  CperHdr->Flags,
                  EFI_HW_ERROR_FLAGS_PREVERR);
    goto Cleanup;
  }

  Status = EFI_SUCCESS;

Cleanup:
  return Status;
} // MsWheaVerifyCPERHeader ()


/**
  CPER Section Descriptor verification routine, between flash entry and expect based on input payload

  @param[in] Context                  Test context applied for this test case
  @param[in] CperSecDecs              CPER Section Descriptor read from a specific HwErrRec entry
  @param[in] ErrorSeverity            Error Severity from input payload
  @param[in] ErrorStatusCodeValue     Error Status Code Value passed during ReportStatusCode*
  @param[in] TotalSize                Total size of this specific HwErrRec entry

  @retval EFI_SUCCESS                 The entry point executed successfully.
  @retval EFI_INVALID_PARAMETER       Null pointer detected.
  @retval EFI_PROTOCOL_ERROR          Parameter mismatches during validation.
  @retval EFI_BAD_BUFFER_SIZE         Read buffer does match the data size field in the header.

**/
STATIC
EFI_STATUS
MsWheaVerifyCPERSecDesc (
  IN UNIT_TEST_CONTEXT                Context,
  IN EFI_ERROR_SECTION_DESCRIPTOR     *CperSecDecs,
  IN UINT32                           ErrorSeverity,
  IN UINT32                           ErrorStatusCodeValue,
  IN UINT32                           TotalSize
  )
{
  EFI_STATUS            Status;

  if (CperSecDecs == NULL) {
    Status = EFI_INVALID_PARAMETER;
    UT_LOG_ERROR( "CPER Section Descriptor Null pointer exception.");
    goto Cleanup;
  }

  if (CperSecDecs->SectionOffset != HW_ERR_REC_HEADERS_OFFSET) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Section Descriptor Singnature Start mismatch: %08X.", CperSecDecs->SectionOffset);
    goto Cleanup;
  }

  if (CperSecDecs->SectionLength != (TotalSize - HW_ERR_REC_HEADERS_OFFSET)) {
    Status = EFI_BAD_BUFFER_SIZE;
    UT_LOG_ERROR( "CPER Section Descriptor length mismatch: has %08X, expects %08X.",
                  CperSecDecs->SectionLength,
                  TotalSize - HW_ERR_REC_HEADERS_OFFSET);
    goto Cleanup;
  }

  if (CperSecDecs->Revision != EFI_ERROR_SECTION_REVISION) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Section Descriptor revision End mismatch: %04X.", CperSecDecs->Revision);
    goto Cleanup;
  }

  if (CperSecDecs->SecValidMask != 0) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Section Descriptor SecValidMask incorrect: %02X.", CperSecDecs->SecValidMask);
    goto Cleanup;
  }

  if (CperSecDecs->SectionFlags != 0) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Section Descriptor SectionFlags incorrect: %08X.", CperSecDecs->SectionFlags);
    goto Cleanup;
  }

  if (CompareGuid(&CperSecDecs->SectionType, &gMuTelemetrySectionTypeGuid) == FALSE) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Section Descriptor Section Type mismatch: has: %g, expect: %g.",
                  CperSecDecs->SectionType,
                  &gMuTelemetrySectionTypeGuid);
    goto Cleanup;
  }

  if (IsZeroBuffer(&CperSecDecs->FruId, sizeof(EFI_GUID)) == FALSE) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Section Descriptor Fru ID not emptry. Has: %g.",
                  CperSecDecs->FruId);
    goto Cleanup;
  }

  if (CperSecDecs->Severity != ErrorSeverity) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Section Descriptor Error severity mismatch: has: %08X, expect: %08X.",
                  CperSecDecs->Severity,
                  ErrorSeverity);
    goto Cleanup;
  }

  if (IsZeroBuffer(&CperSecDecs->FruString, sizeof(CperSecDecs->FruString)) == FALSE) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Section Descriptor FruString not emptry.");
    goto Cleanup;
  }

  Status = EFI_SUCCESS;

Cleanup:
  return Status;
} // MsWheaVerifyCPERSecDesc ()


/**
  Firmware Error Data verification routine, between flash entry and expect based on input payload

  @param[in] Context                  Test context applied for this test case
  @param[in] EfiFirmwareErrorData     Firmware Error Data read from a specific HwErrRec entry
  @param[in] ErrorStatusCodeValue     Error Status Code Value passed during ReportStatusCode*

  @retval EFI_SUCCESS                 The entry point executed successfully.
  @retval EFI_INVALID_PARAMETER       Null pointer detected.
  @retval EFI_PROTOCOL_ERROR          Parameter mismatches during validation.
  @retval EFI_BAD_BUFFER_SIZE         Read buffer does match the data size field in the header.

**/
STATIC
EFI_STATUS
MsWheaVerifyMuTelemetryErrorData (
  IN UNIT_TEST_CONTEXT                Context,
  IN MU_TELEMETRY_CPER_SECTION_DATA   *MuTelemetrySectionData,
  IN EFI_GUID                         *LibraryId,
  IN UINT32                           ErrorStatusCodeValue,
  IN UINT64                           AdditionalInfo1,
  IN UINT64                           AdditionalInfo2
  )
{
  EFI_STATUS            Status;

  if (MuTelemetrySectionData == NULL) {
    Status = EFI_INVALID_PARAMETER;
    UT_LOG_ERROR( "Mu Telemetry Section Data Null pointer exception.");
    goto Cleanup;
  }

  if (CompareMem(&MuTelemetrySectionData->ComponentID, &gEfiCallerIdGuid, sizeof(EFI_GUID))) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "Mu Telemetry system Id mismatch: %d.",
                  MuTelemetrySectionData->ComponentID);
    goto Cleanup;
  }

  if ((LibraryId != NULL) &&
      CompareMem(&MuTelemetrySectionData->SubComponentID, LibraryId, sizeof(EFI_GUID))) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "Mu Telemetry subsystem Id mismatch: %g.",
                  MuTelemetrySectionData->SubComponentID);
    goto Cleanup;
  }

  if (MuTelemetrySectionData->ErrorStatusValue != ErrorStatusCodeValue) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "Mu Telemetry ErrorStatusValue mismatch: has: %08X, expect %08X.",
                  MuTelemetrySectionData->ErrorStatusValue, ErrorStatusCodeValue);
    goto Cleanup;
  }

  if (MuTelemetrySectionData->AdditionalInfo1 != AdditionalInfo1) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "Mu Telemetry AdditionalInfo1 mismatch: has: %16X, expect %16X.",
                  MuTelemetrySectionData->AdditionalInfo1, AdditionalInfo1);
    goto Cleanup;
  }

  if (MuTelemetrySectionData->AdditionalInfo2 != AdditionalInfo2) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "Mu Telemetry AdditionalInfo2 mismatch: has: %16X, expect %16X.",
                  MuTelemetrySectionData->AdditionalInfo2, AdditionalInfo2);
    goto Cleanup;
  }

  Status = EFI_SUCCESS;

Cleanup:
  return Status;
} // MsWheaVerifyMuTelemetryErrorData ()


/**
  HwErrRec Entry verification routine, between flash entry and expect based on input payload

  @param[in] Context                  Test context applied for this test case
  @param[in] TestIndex                This is used to locate the corresponding HwErrRec entry
  @param[in] ErrorStatusCodeValue     Error Status Code Value passed during ReportStatusCode*
  @param[in] ErrorSeverity            Error Severity from input payload, Sev Info if null payload
  @param[in] PartitionId              Ihv shared Guid from caller
  @param[in] LibraryId                Library Id Guid from caller, used to populated SubComponentID of section data
  @param[in] AdditionalInfo1          AdditionalInfo1 reported when calling ReportStatusCodeWithExtendedData
  @param[in] AdditionalInfo2          AdditionalInfo2 reported when calling ReportStatusCodeWithExtendedData

  @retval EFI_SUCCESS                 The entry point executed successfully.
  @retval EFI_INVALID_PARAMETER       Null pointer detected.
  @retval EFI_PROTOCOL_ERROR          Parameter mismatches during validation.
  @retval EFI_BAD_BUFFER_SIZE         Read buffer does match the data size field in the header.

**/
STATIC
EFI_STATUS
MsWheaVerifyFlashStorage (
  IN UNIT_TEST_CONTEXT                Context,
  IN UINT16                           TestIndex,
  IN UINT32                           ErrorStatusCodeValue, // Whatever was passed into RSC
  IN UINT32                           ErrorSeverity,
  IN EFI_GUID                         *PartitionId,
  IN EFI_GUID                         *LibraryId,
  IN UINT64                           AdditionalInfo1,
  IN UINT64                           AdditionalInfo2
  )
{
  EFI_STATUS            Status;
  UINTN                 Size = 0;
  UINT32                mIndex = 0;
  UINT8                 *Buffer = NULL;
  CHAR16                VarName[EFI_HW_ERR_REC_VAR_NAME_LEN];
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  EFI_COMMON_ERROR_RECORD_HEADER  *CperHdr;
  EFI_ERROR_SECTION_DESCRIPTOR    *CperSecDecs;
  MU_TELEMETRY_CPER_SECTION_DATA  *MuTelSecData;

  DEBUG((DEBUG_ERROR, "%a enter\n", __FUNCTION__));

  UnicodeSPrint(VarName, sizeof(VarName), L"%s%04X", EFI_HW_ERR_REC_VAR_NAME, TestIndex);
  Status = gRT->GetVariable(VarName,
                            &gEfiHardwareErrorVariableGuid,
                            NULL,
                            &Size,
                            NULL);
  if ((Status == EFI_NOT_FOUND) && (MsWheaContext->TestId == TEST_ID_BOUNDARY)) {
    // Silently continue
    DEBUG((DEBUG_INFO, "%a Boundary test has Not Found error %s %08X %08X\n", __FUNCTION__,
                                    VarName,
                                    PcdGet32(PcdMaxHardwareErrorVariableSize),
                                    HW_ERR_REC_HEADERS_OFFSET));
    goto Cleanup;
  }
  else if (Status != EFI_BUFFER_TOO_SMALL) {
    UT_LOG_WARNING( "Variable service read %s returns %08X at Test No. %d.",
                    VarName,
                    Status,
                    TestIndex);
    goto Cleanup;
  }

  Buffer = AllocatePool(Size);
  if (Buffer == NULL) {
    UT_LOG_ERROR( "Failed to allocate buffer %08X", Size);
    goto Cleanup;
  }

  Status = gRT->GetVariable(VarName,
                            &gEfiHardwareErrorVariableGuid,
                            NULL,
                            &Size,
                            Buffer);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_ERROR( "Variable service read %s returns %08X, expecting %08X.",
                  VarName,
                  Status,
                  EFI_BUFFER_TOO_SMALL);
    goto Cleanup;
  }

  mIndex = 0;
  CperHdr = (EFI_COMMON_ERROR_RECORD_HEADER*)&Buffer[mIndex];
  Status = MsWheaVerifyCPERHeader(Context,
                                  CperHdr,
                                  ErrorSeverity,
                                  PartitionId,
                                  (UINT32)Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_ERROR( "CPER Header validation fails.");
    goto Cleanup;
  }

  mIndex += sizeof(EFI_COMMON_ERROR_RECORD_HEADER);
  CperSecDecs = (EFI_ERROR_SECTION_DESCRIPTOR*)&Buffer[mIndex];
  Status = MsWheaVerifyCPERSecDesc(Context,
                                  CperSecDecs,
                                  ErrorSeverity,
                                  ErrorStatusCodeValue,
                                  (UINT32)Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_ERROR( "CPER Section Descriptor validation fails.");
    goto Cleanup;
  }

  mIndex += sizeof(EFI_ERROR_SECTION_DESCRIPTOR);
  MuTelSecData = (MU_TELEMETRY_CPER_SECTION_DATA*)&Buffer[mIndex];
  Status = MsWheaVerifyMuTelemetryErrorData(Context,
                                            MuTelSecData,
                                            LibraryId,
                                            ErrorStatusCodeValue,
                                            AdditionalInfo1,
                                            AdditionalInfo2);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_ERROR( "Firmware Error Data validation fails.");
    goto Cleanup;
  }

  mIndex += sizeof(MU_TELEMETRY_CPER_SECTION_DATA);
  if (Size != mIndex) {
    UT_LOG_ERROR( "MS WHEA Payload validation fails on size: has %d, expecting %d.", Size, mIndex);
    goto Cleanup;
  }

Cleanup:
  if (Buffer != NULL) {
    FreePool(Buffer);
  }
  DEBUG((DEBUG_ERROR, "%a exit %r\n", __FUNCTION__, Status));
  return Status;
} // MsWheaVerifyFlashStorage ()

///================================================================================================
///================================================================================================
///
/// PRE REQ FUNCTIONS
///
///================================================================================================
///================================================================================================

/**
  Clear all the HwErrRec entries on flash

  @param[in] Context                  Test context applied for this test case

  @retval UNIT_TEST_PASSED            The entry point executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED Null pointer detected.

**/
UNIT_TEST_STATUS
EFIAPI
MsWheaCommonClean (
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UINT32                Index = 0;
  CHAR16                VarName[EFI_HW_ERR_REC_VAR_NAME_LEN];
  UINTN                 Size = 0;
  EFI_STATUS            Status = EFI_SUCCESS;
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  UINT32                Attributes;

  DEBUG((DEBUG_ERROR, "%a enter\n", __FUNCTION__));

  for (Index = 0; Index <= MAX_UINT16; Index++) {
    Size = 0;
    UnicodeSPrint(VarName, sizeof(VarName), L"%s%04X", EFI_HW_ERR_REC_VAR_NAME, Index);
    Status = gRT->GetVariable(VarName,
                              &gEfiHardwareErrorVariableGuid,
                              NULL,
                              &Size,
                              NULL);
    if (Status == EFI_NOT_FOUND) {
      // Do nothing
      continue;
    }
    else if (Status != EFI_BUFFER_TOO_SMALL) {
      // We have other problems here..
      break;
    }

    Attributes = EFI_VARIABLE_NON_VOLATILE |
                 EFI_VARIABLE_BOOTSERVICE_ACCESS |
                 EFI_VARIABLE_RUNTIME_ACCESS;

    if (PcdGetBool(PcdVariableHardwareErrorRecordAttributeSupported)) {
      Attributes |= EFI_VARIABLE_HARDWARE_ERROR_RECORD;
    }

    Status = gRT->SetVariable(VarName,
                              &gEfiHardwareErrorVariableGuid,
                              Attributes,
                              0,
                              NULL);
    if (Status != EFI_SUCCESS) {
      UT_LOG_ERROR( "MS WHEA Clean variables failed: SetVar: Name: %s, Status: %08X, Size: %d\n",
                    VarName,
                    Status,
                    Size);
      break;
    }
  }

  if ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND)) {
    utStatus = UNIT_TEST_PASSED;
  }
  else {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
  }

  DEBUG((DEBUG_ERROR, "%a exit...\n", __FUNCTION__));
  return utStatus;
} // MsWheaCommonClean ()

///================================================================================================
///================================================================================================
///
/// CLEANUP FUNCTIONS
///
///================================================================================================
///================================================================================================

/**
  Clear all the HwErrRec entries on flash

  @param[in] Context                  Test context applied for this test case

  @retval UNIT_TEST_PASSED            The entry point executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED Null pointer detected.

**/
VOID
EFIAPI
MsWheaCommonCleanUp (
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UINT32                Index = 0;
  CHAR16                VarName[EFI_HW_ERR_REC_VAR_NAME_LEN];
  UINTN                 Size = 0;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT32                Attributes;

  DEBUG((DEBUG_ERROR, "%a enter\n", __FUNCTION__));

  for (Index = 0; Index <= MAX_UINT16; Index++) {
    Size = 0;
    UnicodeSPrint(VarName, sizeof(VarName), L"%s%04X", EFI_HW_ERR_REC_VAR_NAME, Index);
    Status = gRT->GetVariable(VarName,
                              &gEfiHardwareErrorVariableGuid,
                              NULL,
                              &Size,
                              NULL);
    if (Status == EFI_NOT_FOUND) {
      // Do nothing
      continue;
    }
    else if (Status != EFI_BUFFER_TOO_SMALL) {
      // We have other problems here..
      break;
    }

    Attributes = EFI_VARIABLE_NON_VOLATILE |
                 EFI_VARIABLE_BOOTSERVICE_ACCESS |
                 EFI_VARIABLE_RUNTIME_ACCESS;

    if (PcdGetBool(PcdVariableHardwareErrorRecordAttributeSupported)) {
      Attributes |= EFI_VARIABLE_HARDWARE_ERROR_RECORD;
    }

    Status = gRT->SetVariable(VarName,
                              &gEfiHardwareErrorVariableGuid,
                              Attributes,
                              0,
                              NULL);
    if (Status != EFI_SUCCESS) {
      UT_LOG_ERROR( "MS WHEA Clean variables failed: SetVar: Name: %s, Status: %08X, Size: %d\n",
                    VarName,
                    Status,
                    Size);
      break;
    }
  }


  DEBUG((DEBUG_ERROR, "%a exit...\n", __FUNCTION__));
} // MsWheaCommonClean ()
///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================


/**
  This routine should store 2 Fatal Ex errors on flash
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaFatalExEntries (
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 TestIndex;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  MsWheaContext->TestId = TEST_ID_FATAL_EX;

  for (TestIndex =0; TestIndex < UNIT_TEST_SINGLE_ROUND; TestIndex++) {
    DEBUG((DEBUG_INFO, "%a: Test No. %d...\n", __FUNCTION__, TestIndex));

    LogTelemetry (TRUE,
                  NULL,
                  UNIT_TEST_ERROR_CODE | TestIndex,
                  &gMuTestLibraryGuid,
                  &gMuTestIhvSharedGuid,
                  UNIT_TEST_ERROR_INFO1,
                  UNIT_TEST_ERROR_INFO2
                  );
    Status = MsWheaVerifyFlashStorage(Context,
                                      TestIndex,
                                      UNIT_TEST_ERROR_CODE | TestIndex,
                                      EFI_GENERIC_ERROR_FATAL,
                                      &gMuTestIhvSharedGuid,
                                      &gMuTestLibraryGuid,
                                      UNIT_TEST_ERROR_INFO1,
                                      UNIT_TEST_ERROR_INFO2);
    if (EFI_ERROR(Status) != FALSE) {
      utStatus = UNIT_TEST_ERROR_TEST_FAILED;
      UT_LOG_ERROR( "Fatal Ex test case %d failed.", TestIndex);
      goto Cleanup;
    }
  }

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Fatal Ex test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, "%a: exit...\n", __FUNCTION__));
  return utStatus;
} // MsWheaFatalExEntries ()


/**
  This routine should store 2 Non Fatal Ex errors on flash
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaNonFatalExEntries (
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 TestIndex;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  MsWheaContext->TestId = TEST_ID_NON_FATAL_EX;

  for (TestIndex =0; TestIndex < UNIT_TEST_SINGLE_ROUND; TestIndex++) {
    DEBUG((DEBUG_INFO, "%a: Test No. %d...\n", __FUNCTION__, TestIndex));
    LogTelemetry (FALSE,
                  NULL,
                  UNIT_TEST_ERROR_CODE | TestIndex,
                  &gMuTestLibraryGuid,
                  &gMuTestIhvSharedGuid,
                  UNIT_TEST_ERROR_INFO1,
                  UNIT_TEST_ERROR_INFO2
                  );
    Status = MsWheaVerifyFlashStorage(Context,
                                      TestIndex,
                                      UNIT_TEST_ERROR_CODE | TestIndex,
                                      EFI_GENERIC_ERROR_INFO,
                                      &gMuTestIhvSharedGuid,
                                      &gMuTestLibraryGuid,
                                      UNIT_TEST_ERROR_INFO1,
                                      UNIT_TEST_ERROR_INFO2);
    if (EFI_ERROR(Status) != FALSE) {
      utStatus = UNIT_TEST_ERROR_TEST_FAILED;
      UT_LOG_ERROR( "Non Fatal Ex test case %d failed.", TestIndex);
      goto Cleanup;
    }
  }

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Non Fatal Ex test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, "%a: exit...\n", __FUNCTION__));
  return utStatus;
} // MsWheaNonFatalExEntries ()


/**
  This routine should not store any errors on flash
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaWildcardEntries (
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 Data[UNIT_TEST_ERROR_SIZE];
  UINT8                 TestIndex;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  MsWheaContext->TestId = TEST_ID_WILDCARD;
  TestIndex = 0;
  DEBUG((DEBUG_INFO, "%a: Test No. %d...\n", __FUNCTION__, TestIndex));
  SetMem(Data,
        UNIT_TEST_ERROR_SIZE,
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  Status = ReportStatusCodeWithExtendedData(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            UNIT_TEST_ERROR_SIZE);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Context,
                                    TestIndex,
                                    UNIT_TEST_ERROR_CODE,
                                    EFI_GENERIC_ERROR_FATAL,
                                    &gMuTestIhvSharedGuid,
                                    &gMuTestLibraryGuid,
                                    0,
                                    0);
  if (!EFI_ERROR(Status)) {
    // Should not store anything if malformatted data
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Wildcard payload test case %d failed.", TestIndex);
    goto Cleanup;
  }

  TestIndex ++;
  DEBUG((DEBUG_INFO, "%a: Test No. %d...\n", __FUNCTION__, TestIndex));
  SetMem(Data,
        UNIT_TEST_ERROR_SIZE,
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  Status = ReportStatusCodeWithExtendedData(MS_WHEA_ERROR_STATUS_TYPE_INFO,
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            UNIT_TEST_ERROR_SIZE);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Context,
                                    TestIndex,
                                    UNIT_TEST_ERROR_CODE,
                                    EFI_GENERIC_ERROR_INFO,
                                    &gMuTestIhvSharedGuid,
                                    &gMuTestLibraryGuid,
                                    0,
                                    0);
  if (!EFI_ERROR(Status)) {
    // Should not store anything if malformatted data
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Wildcard payload test case %d failed.", TestIndex);
    goto Cleanup;
  }

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Wildcard payload test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, "%a: exit...\n", __FUNCTION__));
  return utStatus;
} // MsWheaWildcardEntries ()


/**
  This routine should store one short error for each supported severity on flash
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaShortEntries (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 TestIndex;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  MsWheaContext->TestId = TEST_ID_SHORT;
  TestIndex = 0;
  DEBUG((DEBUG_INFO, "%a: Test No. %d...\n", __FUNCTION__, TestIndex));
  Status = ReportStatusCode(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                            UNIT_TEST_ERROR_CODE);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Context,
                                    TestIndex,
                                    UNIT_TEST_ERROR_CODE,
                                    EFI_GENERIC_ERROR_FATAL,
                                    NULL,
                                    NULL,
                                    0,
                                    0);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Short invocation test case %d failed.", TestIndex);
    goto Cleanup;
  }

  TestIndex ++;
  DEBUG((DEBUG_INFO, "%a: Test No. %d...\n", __FUNCTION__, TestIndex));
  Status = ReportStatusCode(MS_WHEA_ERROR_STATUS_TYPE_INFO,
                            UNIT_TEST_ERROR_CODE);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Context,
                                    TestIndex,
                                    UNIT_TEST_ERROR_CODE,
                                    EFI_GENERIC_ERROR_INFO,
                                    NULL,
                                    NULL,
                                    0,
                                    0);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Short invocation test case %d failed.", TestIndex);
    goto Cleanup;
  }

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Short invocation test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, "%a: exit...\n", __FUNCTION__));
  return utStatus;
} // MsWheaShortEntries ()


/**
  This routine should try to store as many errors on flash with a fixed size, test should fail with
  out of resources before the designated count is reached
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaStressEntries (
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT16                TestIndex;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  MsWheaContext->TestId = TEST_ID_STRESS;

  // This should use up all the available space and return
  for (TestIndex = 0; TestIndex < (PcdGet32(PcdHwErrStorageSize)/UNIT_TEST_ERROR_SIZE + 1); TestIndex++) {
    DEBUG((DEBUG_INFO, "%a: Test No. %d...\n", __FUNCTION__, TestIndex));
    Status = ReportStatusCode(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                              UNIT_TEST_ERROR_CODE);
    if (EFI_ERROR(Status) != FALSE) {
      UT_LOG_WARNING( "Report Status Code returns non success value.");
    }
    Status = MsWheaVerifyFlashStorage(Context,
                                      TestIndex,
                                      UNIT_TEST_ERROR_CODE,
                                      EFI_GENERIC_ERROR_FATAL,
                                      NULL,
                                      NULL,
                                      0,
                                      0);
    DEBUG((DEBUG_INFO,"Result: %r \n", Status));
    if (EFI_ERROR(Status) != FALSE) {
      DEBUG((DEBUG_INFO,"%a Stress test case ceased at No. %d.\n", __FUNCTION__, TestIndex));
      break;
    }
  }

  if (Status != EFI_NOT_FOUND) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Stress test case failed as payload returns %08X, expecting %08X.",
                  Status,
                  EFI_NOT_FOUND);
    goto Cleanup;
  }

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Stress test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, "%a: exit...\n", __FUNCTION__));
  return utStatus;
} // MsWheaStressEntries ()


/**
  This routine should verify changes regarding variable services:
  1.	Deleted hw error records should not count towards the hw error record space quota
  2.	HwError record quota state should not trigger variable store reclaim
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaVariableServicesTest (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 Data[sizeof(MS_WHEA_RSC_INTERNAL_ERROR_DATA)];
  UINT32                Size = sizeof(MS_WHEA_RSC_INTERNAL_ERROR_DATA);
  UINT16                TestIndex;
  CHAR16                VarName[EFI_HW_ERR_REC_VAR_NAME_LEN];
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;
  UINT32                Attributes;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  MsWheaContext->TestId = TEST_ID_VARSEV;
  UnicodeSPrint(VarName, sizeof(VarName), L"%s%04X", EFI_HW_ERR_REC_VAR_NAME, 0);

  // Phase 1: Alternate write and delete HwErrRec, it should end up with out of resources
  for (TestIndex = 0; TestIndex < (PcdGet32(PcdFlashNvStorageVariableSize)/UNIT_TEST_ERROR_SIZE + 1); TestIndex++) {
    DEBUG((DEBUG_INFO, "%a: Test No. %d...\n", __FUNCTION__, TestIndex));
    Status = ReportStatusCode(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                              UNIT_TEST_ERROR_CODE);
    if (EFI_ERROR(Status) != FALSE) {
      DEBUG((DEBUG_WARN, "%a: Write %d failed with %r...\n", __FUNCTION__, TestIndex, Status));
    }

    Attributes = EFI_VARIABLE_NON_VOLATILE |
                 EFI_VARIABLE_BOOTSERVICE_ACCESS |
                 EFI_VARIABLE_RUNTIME_ACCESS;

    if (PcdGetBool(PcdVariableHardwareErrorRecordAttributeSupported)) {
      Attributes |= EFI_VARIABLE_HARDWARE_ERROR_RECORD;
    }

    Status = gRT->SetVariable(VarName,
                              &gEfiHardwareErrorVariableGuid,
                              Attributes,
                              0,
                              NULL);
    if (Status == EFI_SUCCESS) {
      DEBUG((DEBUG_INFO, "%a: Write %d result: %r...\n", __FUNCTION__, TestIndex, Status));
    } else if (Status == EFI_NOT_FOUND) {
      DEBUG((DEBUG_INFO, "%a: Phase 1 test ceased at %d...\n", __FUNCTION__, TestIndex));
      break;
    } else {
      utStatus = UNIT_TEST_ERROR_TEST_FAILED;
      UT_LOG_ERROR( "Read HwErrRec failed at %d, result: %r.", TestIndex, Status);
      goto Cleanup;
    }
  }

  if (Status != EFI_NOT_FOUND) {
    UT_LOG_ERROR( "Variable service test Phase 1 expect EFI_NOT_FOUND, has %r.", Status);
    goto Cleanup;
  }

  // Phase 2: Write a common variable should trigger Reclaim
  Status = gRT->SetVariable(L"CommonVar",
                            &gMsWheaReportServiceGuid,
                            EFI_VARIABLE_NON_VOLATILE |
                            EFI_VARIABLE_BOOTSERVICE_ACCESS |
                            EFI_VARIABLE_RUNTIME_ACCESS,
                            Size,
                            Data);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Write common variable not succeeded at result: %r.", Status);
    goto Cleanup;
  }

  // Turn off the light when leaving the room
  Status = gRT->SetVariable(L"CommonVar",
                            &gMsWheaReportServiceGuid,
                            EFI_VARIABLE_NON_VOLATILE |
                            EFI_VARIABLE_BOOTSERVICE_ACCESS |
                            EFI_VARIABLE_RUNTIME_ACCESS,
                            0,
                            NULL);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Delete common variable not succeeded at result: %r.", Status);
    goto Cleanup;
  }

  // Phase 3: Write a HwErrRec should succeed
  Status = ReportStatusCode(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                            UNIT_TEST_ERROR_CODE);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Context,
                                    0,
                                    UNIT_TEST_ERROR_CODE,
                                    EFI_GENERIC_ERROR_FATAL,
                                    NULL,
                                    NULL,
                                    0,
                                    0);
  DEBUG((DEBUG_INFO,"Result: %r \n", Status));
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Written HwErrRec failed to pass verification.");
    goto Cleanup;
  }

  //
  // TODO: Phase 4: Verify overloading HwErrRec will not trigger Reclaim
  //

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Variable service test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, "%a: exit...\n", __FUNCTION__));
  return utStatus;
} // MsWheaVariableServicesTest ()


/**
  This routine should verify report works at all the supported TPLs
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaReportTplTest (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT16                TestIndex;
  UINT32                Attributes;
  EFI_TPL               TplPrevious;
  EFI_TPL               TplCap;
  CHAR16                VarName[EFI_HW_ERR_REC_VAR_NAME_LEN];
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  MsWheaContext->TestId = TEST_ID_TPL;
  UnicodeSPrint(VarName, sizeof(VarName), L"%s%04X", EFI_HW_ERR_REC_VAR_NAME, 0);
  TplCap = (FixedPcdGet32(PcdMsWheaRSCHandlerTpl) > TPL_NOTIFY) ? (TPL_NOTIFY) : (FixedPcdGet32(PcdMsWheaRSCHandlerTpl));

  // This would just make sure the report status code handle will not fail on all supported TPLs
  for (TestIndex = TPL_APPLICATION; TestIndex <= TplCap; TestIndex ++)
  {
    DEBUG((DEBUG_INFO, "%a Callback level: %x\n", __FUNCTION__, TestIndex));
    TplPrevious = gBS->RaiseTPL(TestIndex);
    ReportStatusCode(MS_WHEA_ERROR_STATUS_TYPE_FATAL, (EFI_STATUS_CODE_VALUE) (UNIT_TEST_ERROR_CODE | TestIndex));
    gBS->RestoreTPL(TplPrevious) ;
    Status = MsWheaVerifyFlashStorage(Context,
                                    0,
                                    (UNIT_TEST_ERROR_CODE | TestIndex),
                                    EFI_GENERIC_ERROR_FATAL,
                                    NULL,
                                    NULL,
                                    0,
                                    0);
    DEBUG((DEBUG_INFO,"Result: %r \n", Status));
    if (EFI_ERROR(Status) != FALSE) {
      utStatus = UNIT_TEST_ERROR_TEST_FAILED;
      UT_LOG_WARNING( "Written HwErrRec failed to pass verification.");
      goto Cleanup;
    }
    Attributes = EFI_VARIABLE_NON_VOLATILE |
                 EFI_VARIABLE_BOOTSERVICE_ACCESS |
                 EFI_VARIABLE_RUNTIME_ACCESS;

    if (PcdGetBool(PcdVariableHardwareErrorRecordAttributeSupported)) {
      Attributes |= EFI_VARIABLE_HARDWARE_ERROR_RECORD;
    }

    Status = gRT->SetVariable(VarName,
                              &gEfiHardwareErrorVariableGuid,
                              Attributes,
                              0,
                              NULL);
  }

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "TPL report test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, "%a: exit...\n", __FUNCTION__));
  return utStatus;
} // MsWheaReportTplTest ()


/**
  MsWheaReportUnitTestAppEntryPoint

  @param[in] ImageHandle              The firmware allocated handle for the EFI image.
  @param[in] SystemTable              A pointer to the EFI System Table.

  @retval EFI_SUCCESS                 The entry point executed successfully.
  @retval other                       Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
MsWheaReportUnitTestAppEntryPoint (
  IN EFI_HANDLE                       ImageHandle,
  IN EFI_SYSTEM_TABLE                 *SystemTable
  )
{
  EFI_STATUS                  Status = EFI_ABORTED;
  UNIT_TEST_FRAMEWORK_HANDLE  Fw = NULL;
  UNIT_TEST_SUITE_HANDLE      Misc = NULL;
  MS_WHEA_TEST_CONTEXT        *MsWheaContext = NULL;

  DEBUG((DEBUG_ERROR, "%a enter\n", __FUNCTION__));

  MsWheaContext = AllocateZeroPool(sizeof(MS_WHEA_TEST_CONTEXT));
  if (MsWheaContext == NULL) {
    DEBUG(( DEBUG_ERROR, "%a MS WHEA context allocation failed\n", __FUNCTION__));
    goto Cleanup;
  }

  DEBUG(( DEBUG_ERROR, "%a %a v%a\n", __FUNCTION__, UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

  // Start setting up the test framework for running the tests.
  Status = InitUnitTestFramework( &Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION );
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "%a Failed in InitUnitTestFramework. Status = %r\n", __FUNCTION__, Status));
    goto Cleanup;
  }

  // Misc test suite for all tests.
  CreateUnitTestSuite( &Misc, Fw, "MS WHEA Miscellaneous Test cases", "MsWhea.Miscellaneous", NULL, NULL);

  if (Misc == NULL) {
    DEBUG((DEBUG_ERROR, "%a Failed in CreateUnitTestSuite for TestSuite\n", __FUNCTION__));
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  AddTestCase(Misc, "Fatal error Ex report", "MsWhea.Miscellaneous.MsWheaFatalExEntries",
              MsWheaFatalExEntries, MsWheaCommonClean, NULL, MsWheaContext );

  AddTestCase(Misc, "Non-fatal error Ex report", "MsWhea.Miscellaneous.MsWheaNonFatalExEntries",
              MsWheaNonFatalExEntries, MsWheaCommonClean, NULL, MsWheaContext );

  AddTestCase(Misc, "Wildcard error report", "MsWhea.Miscellaneous.MsWheaWildcardEntries",
              MsWheaWildcardEntries, MsWheaCommonClean, NULL, MsWheaContext );

  AddTestCase(Misc, "Short error report", "MsWhea.Miscellaneous.MsWheaShortEntries",
              MsWheaShortEntries, MsWheaCommonClean, NULL, MsWheaContext );

  AddTestCase(Misc, "Stress test should fill up reserved variable space", "MsWhea.Miscellaneous.MsWheaStressEntries",
              MsWheaStressEntries, MsWheaCommonClean, NULL, MsWheaContext );

  AddTestCase(Misc, "Variable service test should verify Reclaim and quota manipulation", "MsWhea.Miscellaneous.MsWheaVariableServicesTest",
              MsWheaVariableServicesTest, MsWheaCommonClean, MsWheaCommonCleanUp, MsWheaContext );

  AddTestCase(Misc, "TPL test for all supported TPLs", "MsWhea.Miscellaneous.MsWheaReportTplTest",
              MsWheaReportTplTest, MsWheaCommonClean, MsWheaCommonCleanUp, MsWheaContext );

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites(Fw);

Cleanup:
  if (Fw != NULL) {
    FreeUnitTestFramework(Fw);
  }
  if (MsWheaContext != NULL) {
    FreePool(MsWheaContext);
  }
  DEBUG((DEBUG_ERROR, "%a exit\n", __FUNCTION__));
  return Status;
} // MsWheaReportUnitTestAppEntryPoint ()
