/** @file -- MsWheaReportUnitTestApp.c

Tests for MS WHEA report tests with various payloads and error severities.

Copyright (c) 2017, Microsoft Corporation

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

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <UnitTestTypes.h>
#include <Library/UnitTestLib.h>
#include <Library/UnitTestLogLib.h>
#include <Library/UnitTestAssertLib.h>
#include <Library/UnitTestBootUsbLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PcdLib.h>
#include <MsWheaErrorStatus.h>


#define UNIT_TEST_APP_NAME            L"MsWhea Report Test"
#define UNIT_TEST_APP_VERSION         L"0.1"

// Copy from MsWheaReportCommon.h, will fail test if any mismatch
#define EFI_HW_ERR_REC_VAR_NAME       L"HwErrRec"
#define EFI_HW_ERR_REC_VAR_NAME_LEN   16      // Buffer length covers at least "HwErrRec####\0"
#define CPER_HDR_SEC_CNT              0x01
#define EFI_FIRMWARE_ERROR_REVISION   0x0002  // Set Firmware Error Record Revision to 2 as per UEFI Spec 2.7A

#define UNIT_TEST_ERROR_CODE          0xA0A0A0A0
#define UNIT_TEST_ERROR_SIZE          0x100
#define UNIT_TEST_ERROR_SHORT_SIZE    (sizeof(MS_WHEA_ERROR_HDR) >> 1) // Half of the header size
#define UNIT_TEST_ERROR_PATTERN       0x30
#define UNIT_TEST_ERROR_INFO          0xC0C0C0C0
#define UNIT_TEST_ERROR_ID            0x50505050

#define MS_WHEA_REV_UNSUPPORTED       0x66

#define HW_ERR_REC_HEADERS_OFFSET     (sizeof(EFI_COMMON_ERROR_RECORD_HEADER) + \
                                       sizeof(EFI_ERROR_SECTION_DESCRIPTOR))
#define HW_ERR_REC_PAYLOAD_OVERHEAD   (HW_ERR_REC_HEADERS_OFFSET + \
                                       sizeof(EFI_FIRMWARE_ERROR_DATA))

typedef enum TEST_ID_T_DEF {
  TEST_ID_FATAL_REV_0,
  TEST_ID_FATAL_REV_1,
  TEST_ID_FATAL_REV_UNSUP,
  TEST_ID_NON_FATAL_REV_0,
  TEST_ID_NON_FATAL_REV_1,
  TEST_ID_NON_FATAL_REV_UNSUP,
  TEST_ID_WILDCARD,
  TEST_ID_SHORT,
  TEST_ID_STRESS,
  TEST_ID_BOUNDARY,
  TEST_ID_VARSEV,

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

  @param[in] Framework                Test framework applied for this test case
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
  IN UNIT_TEST_FRAMEWORK_HANDLE       Framework,
  IN UNIT_TEST_CONTEXT                Context,
  IN EFI_COMMON_ERROR_RECORD_HEADER   *CperHdr,
  IN UINT32                           ErrorSeverity,
  IN UINT32                           ErrorStatusCodeValue,
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
    UT_LOG_ERROR( "CPER Header section count mismatch: has: %d, expect: %d.", 
                  CperHdr->SectionCount, 
                  CPER_HDR_SEC_CNT);
    goto Cleanup;
  }

  if (CperHdr->ValidationBits != EFI_ERROR_RECORD_HEADER_PLATFORM_ID_VALID) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Header validation bits mismatch: has: %d, expect: %d.", 
                  CperHdr->ValidationBits, 
                  EFI_ERROR_RECORD_HEADER_PLATFORM_ID_VALID);
    goto Cleanup;
  }

  if (CperHdr->RecordLength != TotalSize) {
    Status = EFI_BAD_BUFFER_SIZE;
    UT_LOG_ERROR( "CPER Header record length incorrect: has: %08X, expect: %08X.", 
                  CperHdr->RecordLength, 
                  TotalSize);
    goto Cleanup;
  }

  if (CompareGuid(&CperHdr->PlatformID, &gMsWheaReportServiceGuid) == FALSE) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Header Platform ID mismatch: has: %g, expect: %g.", 
                  CperHdr->PlatformID,
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

  if (CperHdr->RecordID != ErrorStatusCodeValue) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Header Error Status Code mismatch: has: %16X, expect: %16X.", 
                  CperHdr->RecordID,
                  ErrorStatusCodeValue);
    goto Cleanup;
  }

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

  @param[in] Framework                Test framework applied for this test case
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
  IN UNIT_TEST_FRAMEWORK_HANDLE       Framework,
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

  if (CompareGuid(&CperSecDecs->SectionType, &gEfiFirmwareErrorSectionGuid) == FALSE) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Section Descriptor Section Type mismatch: has: %g, expect: %g.", 
                  CperSecDecs->SectionType,
                  &gEfiFirmwareErrorSectionGuid);
    goto Cleanup;
  }

  if (CperSecDecs->Severity != ErrorSeverity) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "CPER Section Descriptor Error severity mismatch: has: %08X, expect: %08X.", 
                  CperSecDecs->Severity,
                  ErrorSeverity);
    goto Cleanup;
  }

  Status = EFI_SUCCESS;

Cleanup:
  return Status;
} // MsWheaVerifyCPERSecDesc ()


/**
  Firmware Error Data verification routine, between flash entry and expect based on input payload

  @param[in] Framework                Test framework applied for this test case
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
MsWheaVerifyEfiFirmwareErrorData (
  IN UNIT_TEST_FRAMEWORK_HANDLE       Framework,
  IN UNIT_TEST_CONTEXT                Context,
  IN EFI_FIRMWARE_ERROR_DATA          *EfiFirmwareErrorData,
  IN UINT32                           ErrorStatusCodeValue
  )
{
  EFI_STATUS            Status;

  if (EfiFirmwareErrorData == NULL) {
    Status = EFI_INVALID_PARAMETER;
    UT_LOG_ERROR( "Firmware Error Data Null pointer exception.");
    goto Cleanup;
  }

  if (EfiFirmwareErrorData->ErrorType != EFI_FIRMWARE_ERROR_TYPE_SOC_TYPE2) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "Firmware Error Data Error type mismatch: %d.", 
                  EfiFirmwareErrorData->ErrorType);
    goto Cleanup;
  }

  if (EfiFirmwareErrorData->Revision != EFI_FIRMWARE_ERROR_REVISION) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "Firmware Error Revision mismatch: %04X.", 
                  EfiFirmwareErrorData->Revision);
    goto Cleanup;
  }
  
  if (EfiFirmwareErrorData->RecordId != 0) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "Firmware Error Data RecordID mismatch: has: %16X, expect 0x0.", 
                  EfiFirmwareErrorData->RecordId);
    goto Cleanup;
  }

  Status = EFI_SUCCESS;

Cleanup:
  return Status;
} // MsWheaVerifyEfiFirmwareErrorData ()


/**
  Paylaod verification routine, between flash entry and expect based on input payload

  @param[in] Framework                Test framework applied for this test case
  @param[in] Context                  Test context applied for this test case
  @param[in] Revision                 Revision number from input paylaod, Rev Wildcard if null payload
  @param[in] ErrorSeverity            Error Severity from input payload
  @param[in] HERPayload               Payload portion of a HwErrRec entry, excluding CPER Header, CPER 
                                      section descriptor and Firmware error data
  @param[in] HERPayloadSize           Size of HwErrRec entry paylaod, excluding CPER Header, CPER 
                                      section descriptor and Firmware error data
  @param[in] Payload                  Payload passed in when calling ReportStatusCodeWithExtendedData
  @param[in] PayloadSize              PayloadSize passed in when calling ReportStatusCodeWithExtendedData

  @retval EFI_SUCCESS                 The entry point executed successfully.
  @retval EFI_INVALID_PARAMETER       Null pointer detected.
  @retval EFI_PROTOCOL_ERROR          Parameter mismatches during validation.
  @retval EFI_BAD_BUFFER_SIZE         Read buffer does match the data size field in the header.

**/
STATIC
EFI_STATUS
MsWheaVerifyPayload (
  IN UNIT_TEST_FRAMEWORK_HANDLE       Framework,
  IN UNIT_TEST_CONTEXT                Context,
  IN MS_WHEA_REV                      Revision,
  IN UINT32                           ErrorSeverity,
  IN UINT8                            *HERPayload, 
  IN UINT32                           HERPayloadSize,
  IN VOID                             *Payload,
  IN UINT32                           PayloadSize
  )
{
  EFI_STATUS            Status;
  MS_WHEA_ERROR_HDR     *HERMsWheaErrorHdr = NULL;
  
  // Default value for wildcard inputs, should change if input is valid
  UINT32                ExpectedPayloadSize = PayloadSize + sizeof(MS_WHEA_ERROR_HDR);
  VOID                  *ExpectedPayloadPtr = (MS_WHEA_ERROR_HDR*)Payload;

  MS_WHEA_ERROR_HDR     ExpectedHeader = {
    .Signature = MS_WHEA_ERROR_SIGNATURE,
    .Phase = MS_WHEA_PHASE_DXE_RUNTIME,
    .CriticalInfo = 0,
    .ReporterID = 0
  };
  ExpectedHeader.Rev = Revision,
  ExpectedHeader.ErrorSeverity = ErrorSeverity;

  // Payload CAN be NULL as the caller maybe calling ReportStatusCode
  if (HERPayload == NULL) {
    Status = EFI_INVALID_PARAMETER;
    UT_LOG_ERROR( "MS WHEA payload Null pointer exception.");
    goto Cleanup;
  }

  if (HERPayloadSize < sizeof(MS_WHEA_ERROR_HDR)) {
    Status = EFI_BAD_BUFFER_SIZE;
    UT_LOG_ERROR( "MS WHEA payload length non-sensical: has %d, minimal: %d.", 
                  HERPayloadSize,
                  sizeof(MS_WHEA_ERROR_HDR));
    goto Cleanup;
  }

  HERMsWheaErrorHdr = (MS_WHEA_ERROR_HDR *)HERPayload;

  if (HERMsWheaErrorHdr->Signature != MS_WHEA_ERROR_SIGNATURE) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "MS WHEA payload signature mismatch: %08X.", 
                  HERMsWheaErrorHdr->Signature);
    goto Cleanup;
  }

  if ((Payload != NULL) && 
      (PayloadSize >= sizeof(MS_WHEA_ERROR_HDR)) && 
      (((MS_WHEA_ERROR_HDR*)Payload)->Signature == MS_WHEA_ERROR_SIGNATURE)) {
    // Valid staff from caller;
    ExpectedPayloadSize = PayloadSize;
    ExpectedPayloadPtr = (UINT8*)ExpectedPayloadPtr + sizeof(MS_WHEA_ERROR_HDR);
    CopyMem(&ExpectedHeader, Payload, sizeof(MS_WHEA_ERROR_HDR));
    // Phase should always be runtime in this case
    ExpectedHeader.Phase = MS_WHEA_PHASE_DXE_RUNTIME;
  }
  
  if (HERPayloadSize != ExpectedPayloadSize) {
    Status = EFI_BAD_BUFFER_SIZE;
    UT_LOG_ERROR( "MS WHEA payload wildcard revision mismatch: has %08X, expects %08X.", 
                  HERPayloadSize, 
                  ExpectedPayloadSize);
    goto Cleanup;
  }

  if (HERMsWheaErrorHdr->Rev != ExpectedHeader.Rev) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "MS WHEA payload wildcard revision incorrect: %d.", 
                  HERMsWheaErrorHdr->Rev);
    goto Cleanup;
  }
  
  if (HERMsWheaErrorHdr->Phase != ExpectedHeader.Phase) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "MS WHEA payload wildcard phase incorrect: %d.", 
                  HERMsWheaErrorHdr->Phase);
    goto Cleanup;
  }

  if (HERMsWheaErrorHdr->ErrorSeverity != 
      ExpectedHeader.ErrorSeverity) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "MS WHEA payload wildcard severity mismatch: has %08X, expects %08X.", 
                  HERMsWheaErrorHdr->ErrorSeverity,
                  ErrorSeverity);
    goto Cleanup;
  }

  if (HERMsWheaErrorHdr->CriticalInfo != ExpectedHeader.CriticalInfo) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "MS WHEA payload wildcard critical information non-empty %08X.", 
                  HERMsWheaErrorHdr->CriticalInfo);
    goto Cleanup;
  }

  if (HERMsWheaErrorHdr->ReporterID != ExpectedHeader.ReporterID) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "MS WHEA payload wildcard Reporter ID non-empty %08X.", 
                  HERMsWheaErrorHdr->ReporterID);
    goto Cleanup;
  }

  if (CompareMem(ExpectedPayloadPtr, 
                HERMsWheaErrorHdr + 1, 
                ExpectedPayloadSize - sizeof(MS_WHEA_ERROR_HDR)) != 0) {
    Status = EFI_PROTOCOL_ERROR;
    UT_LOG_ERROR( "MS WHEA payload wildcard content mismatch.");
    goto Cleanup;
  }

  Status = EFI_SUCCESS;

Cleanup:
  return Status;
} // MsWheaVerifyPayload ()


/**
  HwErrRec Entry verification routine, between flash entry and expect based on input payload

  @param[in] Framework                Test framework applied for this test case
  @param[in] Context                  Test context applied for this test case
  @param[in] TestIndex                This is used to locate the corresponding HwErrRec entry
  @param[in] ErrorStatusCodeValue     Error Status Code Value passed during ReportStatusCode*
  @param[in] ErrorSeverity            Error Severity from input payload, Sev Info if null payload
  @param[in] Revision                 Revision number from input paylaod, Rev Wildcard if null payload
  @param[in] Payload                  Payload passed in when calling ReportStatusCodeWithExtendedData
  @param[in] PayloadSize              PayloadSize passed in when calling ReportStatusCodeWithExtendedData

  @retval EFI_SUCCESS                 The entry point executed successfully.
  @retval EFI_INVALID_PARAMETER       Null pointer detected.
  @retval EFI_PROTOCOL_ERROR          Parameter mismatches during validation.
  @retval EFI_BAD_BUFFER_SIZE         Read buffer does match the data size field in the header.

**/
STATIC
EFI_STATUS
MsWheaVerifyFlashStorage (
  IN UNIT_TEST_FRAMEWORK_HANDLE       Framework,
  IN UNIT_TEST_CONTEXT                Context,
  IN UINT16                           TestIndex, 
  IN UINT32                           ErrorStatusCodeValue, // Whatever was passed into RSC
  IN UINT32                           ErrorSeverity, 
  IN MS_WHEA_REV                      Revision,
  IN VOID                             *Payload,             // Whatever was passed into RSC
  IN UINT32                           PayloadSize           // Whatever was passed into RSC
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
  EFI_FIRMWARE_ERROR_DATA         *EfiFwErrData;
  VOID                            *HERPayload;

  DEBUG((DEBUG_ERROR, __FUNCTION__ " enter\n"));
  
  UnicodeSPrint(VarName, sizeof(VarName), L"%s%04X", EFI_HW_ERR_REC_VAR_NAME, TestIndex);
  Status = gRT->GetVariable(VarName,
                            &gEfiHardwareErrorVariableGuid,
                            NULL,
                            &Size,
                            NULL);
  if ((Status == EFI_NOT_FOUND) && (MsWheaContext->TestId == TEST_ID_BOUNDARY)) {
    // Silently continue
    DEBUG((DEBUG_INFO, __FUNCTION__ " Boundary test has Not Found error %s %08X %08X\n", 
                                    VarName, 
                                    PcdGet32(PcdMaxHardwareErrorVariableSize), 
                                    HW_ERR_REC_HEADERS_OFFSET));
    goto Cleanup;
  }
  else if (Status != EFI_BUFFER_TOO_SMALL) {
    UT_LOG_ERROR( "Variable service read %s returns %08X at Test No. %d.", 
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
  Status = MsWheaVerifyCPERHeader(Framework, 
                                  Context, 
                                  CperHdr, 
                                  ErrorSeverity, 
                                  ErrorStatusCodeValue, 
                                  (UINT32)Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_ERROR( "CPER Header validation fails.");
    goto Cleanup;
  }

  mIndex += sizeof(EFI_COMMON_ERROR_RECORD_HEADER);
  CperSecDecs = (EFI_ERROR_SECTION_DESCRIPTOR*)&Buffer[mIndex];
  Status = MsWheaVerifyCPERSecDesc(Framework, 
                                  Context, 
                                  CperSecDecs, 
                                  ErrorSeverity, 
                                  ErrorStatusCodeValue, 
                                  (UINT32)Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_ERROR( "CPER Section Descriptor validation fails.");
    goto Cleanup;
  }
  
  mIndex += sizeof(EFI_ERROR_SECTION_DESCRIPTOR);
  EfiFwErrData = (EFI_FIRMWARE_ERROR_DATA*)&Buffer[mIndex];
  Status = MsWheaVerifyEfiFirmwareErrorData(Framework, 
                                            Context, 
                                            EfiFwErrData, 
                                            ErrorStatusCodeValue);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_ERROR( "Firmware Error Data validation fails.");
    goto Cleanup;
  }

  mIndex += sizeof(EFI_FIRMWARE_ERROR_DATA);
  HERPayload = &Buffer[mIndex];
  Status = MsWheaVerifyPayload(Framework, 
                              Context, 
                              Revision,
                              ErrorSeverity, 
                              HERPayload, 
                              (UINT32) Size - mIndex, 
                              Payload, 
                              PayloadSize);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_ERROR( "MS WHEA Payload validation fails.");
    goto Cleanup;
  }

Cleanup:
  if (Buffer != NULL) {
    FreePool(Buffer);
  }
  DEBUG((DEBUG_ERROR, __FUNCTION__ " exit %r\n", Status));
  return Status;  
} // MsWheaVerifyFlashStorage ()

/**
  Helper function to fill out MsWhea headers

  @param[in] Buffer                   Buffer for header
  @param[in] ErrorSeverity            Error Severity from input payload, Sev Info if null payload
  @param[in] Revision                 Revision number from input paylaod, Rev Wildcard if null payload

**/
VOID
InitMsWheaHeader (
  IN MS_WHEA_REV                      Revision,
  IN UINT32                           Severity,
  OUT UINT8                           *Buffer
)
{
  MS_WHEA_ERROR_HDR     *mMsWheaErrHdr;

  if (Buffer == NULL) {
    return;
  }

  SetMem(Buffer, sizeof(MS_WHEA_ERROR_HDR), 0);
  
  mMsWheaErrHdr = (MS_WHEA_ERROR_HDR*)Buffer;
  mMsWheaErrHdr->Signature = MS_WHEA_ERROR_SIGNATURE;
  mMsWheaErrHdr->CriticalInfo = UNIT_TEST_ERROR_INFO;
  mMsWheaErrHdr->ReporterID = UNIT_TEST_ERROR_ID;
  mMsWheaErrHdr->ErrorSeverity = Severity;
  mMsWheaErrHdr->Rev = Revision;
} // InitMsWheaHeader ()

///================================================================================================
///================================================================================================
///
/// PRE REQ FUNCTIONS
///
///================================================================================================
///================================================================================================

/**
  Clear all the HwErrRec entries on flash

  @param[in] Framework                Test framework applied for this test case
  @param[in] Context                  Test context applied for this test case

  @retval UNIT_TEST_PASSED            The entry point executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED Null pointer detected.

**/
UNIT_TEST_STATUS
EFIAPI
MsWheaCommonClean (
  IN UNIT_TEST_FRAMEWORK_HANDLE       Framework,
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UINT32                Index = 0;
  CHAR16                VarName[EFI_HW_ERR_REC_VAR_NAME_LEN];
  UINTN                 Size = 0;
  EFI_STATUS            Status = EFI_SUCCESS;
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  
  DEBUG((DEBUG_ERROR, __FUNCTION__ " enter\n"));

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

    Status = gRT->SetVariable(VarName,
                              &gEfiHardwareErrorVariableGuid,
                              EFI_VARIABLE_NON_VOLATILE |
                              EFI_VARIABLE_BOOTSERVICE_ACCESS |
                              EFI_VARIABLE_RUNTIME_ACCESS |
                              EFI_VARIABLE_HARDWARE_ERROR_RECORD,
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
  
  DEBUG((DEBUG_ERROR, __FUNCTION__ " exit...\n"));
  return utStatus;
} // MsWheaCommonClean ()

///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================


/**
  This rountine should store 2 Fatal Rev 0 errors on flash 
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaFatalRev0Entries (
  IN UNIT_TEST_FRAMEWORK_HANDLE       Framework,
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 Data[sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE];
  UINT32                Size = sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE;
  MS_WHEA_ERROR_HDR     *mMsWheaErrHdr;
  UINT8                 TestIndex;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));
  
  MsWheaContext->TestId = TEST_ID_FATAL_REV_0;
  InitMsWheaHeader (MS_WHEA_REV_0, EFI_GENERIC_ERROR_FATAL, Data);
  mMsWheaErrHdr = (MS_WHEA_ERROR_HDR*)Data;
  
  TestIndex = 0;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  SetMem(mMsWheaErrHdr + 1, 
        UNIT_TEST_ERROR_SIZE, 
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr->ErrorSeverity, 
                                    mMsWheaErrHdr->Rev, 
                                    Data, 
                                    Size);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Fatal Rev0 test case %d failed.", TestIndex);
    goto Cleanup;
  }

  TestIndex ++;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  SetMem(mMsWheaErrHdr + 1, 
        UNIT_TEST_ERROR_SIZE, 
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr->ErrorSeverity, 
                                    mMsWheaErrHdr->Rev, 
                                    Data, 
                                    Size);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Fatal Rev0 test case %d failed.", TestIndex);
    goto Cleanup;
  }

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Fatal Rev0 test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...\n"));
  return utStatus;
} // MsWheaFatalRev0Entries ()


/**
  This rountine should store 2 Fatal Rev 1 errors on flash 
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaFatalRev1Entries (
  IN UNIT_TEST_FRAMEWORK_HANDLE       Framework,
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 Data[sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE];
  UINT32                Size = sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE;
  MS_WHEA_ERROR_HDR     *mMsWheaErrHdr;
  UINT8                 TestIndex;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  MsWheaContext->TestId = TEST_ID_FATAL_REV_1;
  InitMsWheaHeader (MS_WHEA_REV_1, EFI_GENERIC_ERROR_FATAL, Data);
  mMsWheaErrHdr = (MS_WHEA_ERROR_HDR*)Data;

  TestIndex = 0;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  SetMem(mMsWheaErrHdr + 1, 
        UNIT_TEST_ERROR_SIZE, 
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }  
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr->ErrorSeverity, 
                                    mMsWheaErrHdr->Rev, 
                                    Data, 
                                    Size);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Fatal Rev1 test case %d failed.", TestIndex);
    goto Cleanup;
  }

  TestIndex ++;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  SetMem(mMsWheaErrHdr + 1, 
        UNIT_TEST_ERROR_SIZE, 
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }  
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr->ErrorSeverity, 
                                    mMsWheaErrHdr->Rev, 
                                    Data, 
                                    Size);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Fatal Rev1 test case %d failed.", TestIndex);
    goto Cleanup;
  }

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Fatal Rev1 test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...\n"));
  return utStatus;
} // MsWheaFatalRev1Entries ()


/**
  This rountine should store 2 Fatal Unsupported Reved errors on flash 
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaFatalRevUnsupEntries (
  IN UNIT_TEST_FRAMEWORK_HANDLE       Framework,
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 Data[sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE];
  UINT32                Size = sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE;
  MS_WHEA_ERROR_HDR     *mMsWheaErrHdr;
  UINT8                 TestIndex;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  MsWheaContext->TestId = TEST_ID_FATAL_REV_UNSUP;
  InitMsWheaHeader (MS_WHEA_REV_UNSUPPORTED, EFI_GENERIC_ERROR_FATAL, Data);
  mMsWheaErrHdr = (MS_WHEA_ERROR_HDR*)Data;

  TestIndex = 0;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  SetMem(mMsWheaErrHdr + 1, 
        UNIT_TEST_ERROR_SIZE, 
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }  
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr->ErrorSeverity, 
                                    mMsWheaErrHdr->Rev, 
                                    Data, 
                                    Size);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Fatal Rev1 test case %d failed.", TestIndex);
    goto Cleanup;
  }

  TestIndex ++;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  SetMem(mMsWheaErrHdr + 1, 
        UNIT_TEST_ERROR_SIZE, 
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }  
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr->ErrorSeverity, 
                                    mMsWheaErrHdr->Rev, 
                                    Data, 
                                    Size);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Fatal Rev1 test case %d failed.", TestIndex);
    goto Cleanup;
  }

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Fatal Rev1 test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...\n"));
  return utStatus;
} // MsWheaFatalRevUnsupEntries ()


/**
  This rountine should store 2 Non Fatal Rev 0 errors on flash 
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaNonFatalRev0Entries (
  IN UNIT_TEST_FRAMEWORK_HANDLE       Framework,
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 Data[sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE];
  UINT32                Size = sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE;
  MS_WHEA_ERROR_HDR     *mMsWheaErrHdr;
  UINT8                 TestIndex;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  MsWheaContext->TestId = TEST_ID_NON_FATAL_REV_0;
  InitMsWheaHeader (MS_WHEA_REV_0, EFI_GENERIC_ERROR_CORRECTED, Data);
  mMsWheaErrHdr = (MS_WHEA_ERROR_HDR*)Data;
  
  TestIndex = 0;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  SetMem(mMsWheaErrHdr + 1, 
        UNIT_TEST_ERROR_SIZE, 
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  mMsWheaErrHdr->ErrorSeverity = EFI_GENERIC_ERROR_CORRECTED;  
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }  
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr->ErrorSeverity, 
                                    mMsWheaErrHdr->Rev, 
                                    Data, 
                                    Size);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Non Fatal Rev0 test case %d failed.", TestIndex);
    goto Cleanup;
  }

  TestIndex ++;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  SetMem(mMsWheaErrHdr + 1, 
        UNIT_TEST_ERROR_SIZE, 
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  mMsWheaErrHdr->ErrorSeverity = EFI_GENERIC_ERROR_RECOVERABLE;
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr->ErrorSeverity, 
                                    mMsWheaErrHdr->Rev, 
                                    Data, 
                                    Size);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Non Fatal Rev0 test case %d failed.", TestIndex);
    goto Cleanup;
  }

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Non Fatal Rev0 test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...\n"));
  return utStatus;
} // MsWheaNonFatalRev0Entries ()


/**
  This rountine should store 2 Non Fatal Rev 1 errors on flash 
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaNonFatalRev1Entries (
  IN UNIT_TEST_FRAMEWORK_HANDLE       Framework,
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 Data[sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE];
  UINT32                Size = sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE;
  MS_WHEA_ERROR_HDR     *mMsWheaErrHdr;
  UINT8                 TestIndex;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  MsWheaContext->TestId = TEST_ID_NON_FATAL_REV_1;
  InitMsWheaHeader (MS_WHEA_REV_1, EFI_GENERIC_ERROR_CORRECTED, Data);
  mMsWheaErrHdr = (MS_WHEA_ERROR_HDR*)Data;
  
  TestIndex = 0;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  SetMem(mMsWheaErrHdr + 1, 
        UNIT_TEST_ERROR_SIZE, 
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  mMsWheaErrHdr->ErrorSeverity = EFI_GENERIC_ERROR_CORRECTED;  
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }  
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr->ErrorSeverity, 
                                    mMsWheaErrHdr->Rev, 
                                    Data, 
                                    Size);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Non Fatal Rev1 test case %d failed.", TestIndex);
    goto Cleanup;
  }

  TestIndex ++;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  SetMem(mMsWheaErrHdr + 1, 
        UNIT_TEST_ERROR_SIZE, 
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  mMsWheaErrHdr->ErrorSeverity = EFI_GENERIC_ERROR_RECOVERABLE;
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr->ErrorSeverity, 
                                    mMsWheaErrHdr->Rev, 
                                    Data, 
                                    Size);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Non Fatal Rev1 test case %d failed.", TestIndex);
    goto Cleanup;
  }

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Non Fatal Rev1 test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...\n"));
  return utStatus;
} // MsWheaNonFatalRev1Entries ()


/**
  This rountine should store 2 Non-Fatal Unsupported Reved errors on flash 
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaNonFatalRevUnsupEntries (
  IN UNIT_TEST_FRAMEWORK_HANDLE       Framework,
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 Data[sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE];
  UINT32                Size = sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE;
  MS_WHEA_ERROR_HDR     *mMsWheaErrHdr;
  UINT8                 TestIndex;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  MsWheaContext->TestId = TEST_ID_NON_FATAL_REV_UNSUP;
  InitMsWheaHeader (MS_WHEA_REV_UNSUPPORTED, EFI_GENERIC_ERROR_CORRECTED, Data);
  mMsWheaErrHdr = (MS_WHEA_ERROR_HDR*)Data;

  TestIndex = 0;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  SetMem(mMsWheaErrHdr + 1, 
        UNIT_TEST_ERROR_SIZE, 
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  mMsWheaErrHdr->ErrorSeverity = EFI_GENERIC_ERROR_CORRECTED;  
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }  
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr->ErrorSeverity, 
                                    mMsWheaErrHdr->Rev, 
                                    Data, 
                                    Size);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Non Fatal Rev1 test case %d failed.", TestIndex);
    goto Cleanup;
  }

  TestIndex ++;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  SetMem(mMsWheaErrHdr + 1, 
        UNIT_TEST_ERROR_SIZE, 
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  mMsWheaErrHdr->ErrorSeverity = EFI_GENERIC_ERROR_RECOVERABLE;
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr->ErrorSeverity, 
                                    mMsWheaErrHdr->Rev, 
                                    Data, 
                                    Size);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Non Fatal Rev1 test case %d failed.", TestIndex);
    goto Cleanup;
  }

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Non Fatal Unsupported rev test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...\n"));
  return utStatus;
} // MsWheaNonFatalRevUnsupEntries ()


/**
  This rountine should store 2 wildcard errors on flash 
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaWildcardEntries (
  IN UNIT_TEST_FRAMEWORK_HANDLE       Framework,
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 Data[UNIT_TEST_ERROR_SIZE];
  UINT8                 TestIndex;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));
  
  MsWheaContext->TestId = TEST_ID_WILDCARD;
  TestIndex = 0;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  SetMem(Data, 
        UNIT_TEST_ERROR_SIZE, 
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            UNIT_TEST_ERROR_SIZE);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    EFI_GENERIC_ERROR_INFO, 
                                    MS_WHEA_REV_WILDCARD, 
                                    Data, 
                                    UNIT_TEST_ERROR_SIZE);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Wildcard payload test case %d failed.", TestIndex);
    goto Cleanup;
  }

  TestIndex ++;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  SetMem(Data, 
        UNIT_TEST_ERROR_SIZE, 
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            UNIT_TEST_ERROR_SIZE);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    EFI_GENERIC_ERROR_INFO, 
                                    MS_WHEA_REV_WILDCARD, 
                                    Data, 
                                    UNIT_TEST_ERROR_SIZE);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Wildcard payload test case %d failed.", TestIndex);
    goto Cleanup;
  }

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Wildcard payload test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...\n"));
  return utStatus;
} // MsWheaWildcardEntries ()


/**
  This rountine should store 4 errors with mixed severity and revision with only the header as a payload 
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaMixedHeaderOnlyEntries (
  IN UNIT_TEST_FRAMEWORK_HANDLE       Framework,
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  MS_WHEA_ERROR_HDR     mMsWheaErrHdr;
  UINT8                 TestIndex;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  MsWheaContext->TestId = TEST_ID_NON_FATAL_REV_UNSUP;
  InitMsWheaHeader (MS_WHEA_REV_UNSUPPORTED, EFI_GENERIC_ERROR_FATAL, (UINT8 *)&mMsWheaErrHdr);
  
  TestIndex = 0;
  mMsWheaErrHdr.Rev = MS_WHEA_REV_0;
  mMsWheaErrHdr.ErrorSeverity = EFI_GENERIC_ERROR_FATAL;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d: Sev: %d Rev %d...\n", 
                                  TestIndex, 
                                  mMsWheaErrHdr.ErrorSeverity, 
                                  mMsWheaErrHdr.Rev));
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            &mMsWheaErrHdr,
                                            sizeof(MS_WHEA_ERROR_HDR));
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }  
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr.ErrorSeverity, 
                                    mMsWheaErrHdr.Rev, 
                                    &mMsWheaErrHdr, 
                                    sizeof(MS_WHEA_ERROR_HDR));
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Mixed Header Only test case %d failed.", TestIndex);
    goto Cleanup;
  }

  TestIndex ++;
  mMsWheaErrHdr.Rev = MS_WHEA_REV_1;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d: Sev: %d Rev %d...\n", 
                                  TestIndex, 
                                  mMsWheaErrHdr.ErrorSeverity, 
                                  mMsWheaErrHdr.Rev));
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            &mMsWheaErrHdr,
                                            sizeof(MS_WHEA_ERROR_HDR));
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }  
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr.ErrorSeverity, 
                                    mMsWheaErrHdr.Rev, 
                                    &mMsWheaErrHdr, 
                                    sizeof(MS_WHEA_ERROR_HDR));
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Mixed Header Only test case %d failed.", TestIndex);
    goto Cleanup;
  }

  TestIndex ++;
  mMsWheaErrHdr.Rev = MS_WHEA_REV_0;
  mMsWheaErrHdr.ErrorSeverity = EFI_GENERIC_ERROR_CORRECTED;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d: Sev: %d Rev %d...\n", 
                                  TestIndex, 
                                  mMsWheaErrHdr.ErrorSeverity, 
                                  mMsWheaErrHdr.Rev));
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            &mMsWheaErrHdr,
                                            sizeof(MS_WHEA_ERROR_HDR));
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }  
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr.ErrorSeverity, 
                                    mMsWheaErrHdr.Rev, 
                                    &mMsWheaErrHdr, 
                                    sizeof(MS_WHEA_ERROR_HDR));
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Mixed Header Only test case %d failed.", TestIndex);
    goto Cleanup;
  }

  TestIndex ++;
  mMsWheaErrHdr.Rev = MS_WHEA_REV_1;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d: Sev: %d Rev %d...\n", 
                                  TestIndex, 
                                  mMsWheaErrHdr.ErrorSeverity, 
                                  mMsWheaErrHdr.Rev));
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            &mMsWheaErrHdr,
                                            sizeof(MS_WHEA_ERROR_HDR));
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }  
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr.ErrorSeverity, 
                                    mMsWheaErrHdr.Rev, 
                                    &mMsWheaErrHdr, 
                                    sizeof(MS_WHEA_ERROR_HDR));
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Mixed Header Only test case %d failed.", TestIndex);
    goto Cleanup;
  }

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Header only test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...\n"));
  return utStatus;
} // MsWheaMixedHeaderOnlyEntries ()


/**
  This rountine should store 3 short errors on flash 
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaShortEntries (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 Data[UNIT_TEST_ERROR_SHORT_SIZE];
  UINT8                 TestIndex;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));
  
  MsWheaContext->TestId = TEST_ID_SHORT;
  TestIndex = 0;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  Status = ReportStatusCode((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                            UNIT_TEST_ERROR_CODE);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    EFI_GENERIC_ERROR_INFO, 
                                    MS_WHEA_REV_WILDCARD, 
                                    NULL, 
                                    0);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Short invocation test case %d failed.", TestIndex);
    goto Cleanup;
  }

  TestIndex ++;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  Status = ReportStatusCode((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                            UNIT_TEST_ERROR_CODE);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    EFI_GENERIC_ERROR_INFO, 
                                    MS_WHEA_REV_WILDCARD, 
                                    NULL, 
                                    0);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Short invocation test case %d failed.", TestIndex);
    goto Cleanup;
  }

  TestIndex ++;
  DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
  SetMem(Data, 
        UNIT_TEST_ERROR_SHORT_SIZE, 
        (UINT8)(UNIT_TEST_ERROR_PATTERN | TestIndex));
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            UNIT_TEST_ERROR_SHORT_SIZE);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    TestIndex, 
                                    UNIT_TEST_ERROR_CODE, 
                                    EFI_GENERIC_ERROR_INFO, 
                                    MS_WHEA_REV_WILDCARD, 
                                    Data, 
                                    UNIT_TEST_ERROR_SHORT_SIZE);
  if (EFI_ERROR(Status) != FALSE) {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Short invocation test case %d failed.", TestIndex);
    goto Cleanup;
  }

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Short invocation test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...\n"));
  return utStatus;
} // MsWheaShortEntries ()


/**
  This rountine should try to store as many errors on flash with a fixed size, test should fail with
  out of resources before the designated count is reached
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaStressEntries (
  IN UNIT_TEST_FRAMEWORK_HANDLE       Framework,
  IN UNIT_TEST_CONTEXT                Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 Data[sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE];
  UINT32                Size = sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE;
  MS_WHEA_ERROR_HDR     *mMsWheaErrHdr;
  UINT16                TestIndex;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  MsWheaContext->TestId = TEST_ID_STRESS;
  InitMsWheaHeader (MS_WHEA_REV_1, EFI_GENERIC_ERROR_FATAL, Data);
  mMsWheaErrHdr = (MS_WHEA_ERROR_HDR*)Data;

  // This should use up all the available space and return 
  for (TestIndex = 0; TestIndex < (PcdGet32(PcdHwErrStorageSize)/UNIT_TEST_ERROR_SIZE + 1); TestIndex++) {
    DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
    SetMem(mMsWheaErrHdr + 1, 
          UNIT_TEST_ERROR_SIZE, 
          (UINT8)(TestIndex & 0xFF));
    Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                              UNIT_TEST_ERROR_CODE,
                                              Data,
                                              Size);
    if (EFI_ERROR(Status) != FALSE) {
      UT_LOG_WARNING( "Report Status Code returns non success value.");
    }
    Status = MsWheaVerifyFlashStorage(Framework,
                                      Context,
                                      TestIndex, 
                                      UNIT_TEST_ERROR_CODE, 
                                      mMsWheaErrHdr->ErrorSeverity, 
                                      mMsWheaErrHdr->Rev, 
                                      Data, 
                                      Size);
    DEBUG((DEBUG_INFO,"Result: %r \n", Status));
    if (EFI_ERROR(Status) != FALSE) {
      DEBUG((DEBUG_INFO, __FUNCTION__"Stress test case ceased at No. %d.\n", TestIndex));
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
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...\n"));
  return utStatus;
} // MsWheaStressEntries ()


/**
  This rountine should try to figure out the maximal size this payload 
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaBoundaryEntries (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 *Data = NULL;
  UINT32                Size;
  UINT32                TestIndex;
  MS_WHEA_ERROR_HDR     *mMsWheaErrHdr;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  MsWheaContext->TestId = TEST_ID_BOUNDARY;
  Size = sizeof(MS_WHEA_ERROR_HDR) + PcdGet32(PcdMaxHardwareErrorVariableSize);
  Data = AllocateZeroPool(Size);
  if (Data == NULL) {
    UT_LOG_ERROR( "Failed to allocate buffer %08X", Size);
    goto Cleanup;
  }
  
  mMsWheaErrHdr = (MS_WHEA_ERROR_HDR*)Data;
  mMsWheaErrHdr->Signature = MS_WHEA_ERROR_SIGNATURE;
  mMsWheaErrHdr->CriticalInfo = UNIT_TEST_ERROR_INFO;
  mMsWheaErrHdr->ReporterID = UNIT_TEST_ERROR_ID;
  mMsWheaErrHdr->Rev = MS_WHEA_REV_1;
  mMsWheaErrHdr->ErrorSeverity = EFI_GENERIC_ERROR_FATAL;

  SetMem(mMsWheaErrHdr + 1, 
        PcdGet32(PcdMaxHardwareErrorVariableSize), 
        (UINT8)(UNIT_TEST_ERROR_PATTERN & 0xFF));

  // This would start from upper bound and reduce the size unti the first success
  for (TestIndex = Size; TestIndex >= sizeof(MS_WHEA_ERROR_HDR); TestIndex --) {
    DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", Size - TestIndex));
    
    Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE),
                                              UNIT_TEST_ERROR_CODE,
                                              Data,
                                              TestIndex);
    if (EFI_ERROR(Status) != FALSE) {
      UT_LOG_WARNING( "Report Status Code returns non success value.");
    }
    Status = MsWheaVerifyFlashStorage(Framework,
                                      Context,
                                      0, 
                                      UNIT_TEST_ERROR_CODE, 
                                      mMsWheaErrHdr->ErrorSeverity, 
                                      mMsWheaErrHdr->Rev, 
                                      Data, 
                                      TestIndex);
    DEBUG((DEBUG_INFO,"Result: %r \n", Status));
    if (EFI_ERROR(Status) == FALSE) {
      DEBUG((DEBUG_INFO, __FUNCTION__"Boundary test case ceased at payload size %d bytes.\n", TestIndex));
      break;
    }
    else if (Status != EFI_NOT_FOUND) {
      DEBUG((DEBUG_INFO, __FUNCTION__"Boundary test case errored at payload size %d bytes, status: %r.\n", 
                                      TestIndex, Status));
      break;
    }
  }

  DEBUG((DEBUG_INFO,"Result: %r \n", Status));
  if (EFI_ERROR(Status) == FALSE) {
    utStatus = UNIT_TEST_PASSED;
    UT_LOG_INFO( "Boundary found to be %d (including MS WHEA Error header), test passed!", TestIndex);
  }
  else {
    utStatus = UNIT_TEST_ERROR_TEST_FAILED;
    UT_LOG_ERROR( "Boundary test case failed as no lower boundary found.");
  }
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...\n"));
  
Cleanup:
  if (Data != NULL) {
    FreePool(Data);
  }
  return utStatus;
} // MsWheaBoundaryEntries ()


/**
  This rountine should verify changes regarding variable services:
  1.	Deleted hw error records should not count towards the hw error record space quota
  2.	HwError record quata state should not trigger variable store reclaim
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaVariableServicesTest (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UNIT_TEST_STATUS      utStatus = UNIT_TEST_RUNNING;
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT8                 Data[sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE];
  UINT32                Size = sizeof(MS_WHEA_ERROR_HDR) + UNIT_TEST_ERROR_SIZE;
  MS_WHEA_ERROR_HDR     *mMsWheaErrHdr;
  UINT16                TestIndex;
  CHAR16                VarName[EFI_HW_ERR_REC_VAR_NAME_LEN];
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = (MS_WHEA_TEST_CONTEXT*) Context;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  MsWheaContext->TestId = TEST_ID_VARSEV;
  UnicodeSPrint(VarName, sizeof(VarName), L"%s%04X", EFI_HW_ERR_REC_VAR_NAME, 0);
  InitMsWheaHeader (MS_WHEA_REV_1, EFI_GENERIC_ERROR_FATAL, Data);
  mMsWheaErrHdr = (MS_WHEA_ERROR_HDR*)Data;

  // Phase 1: Alternate write and delete HwErrRec, it should end up with out of resources
  for (TestIndex = 0; TestIndex < (PcdGet32(PcdFlashNvStorageVariableSize)/UNIT_TEST_ERROR_SIZE + 1); TestIndex++) {
    DEBUG((DEBUG_INFO, __FUNCTION__ ": Test No. %d...\n", TestIndex));
    SetMem(mMsWheaErrHdr + 1, 
          UNIT_TEST_ERROR_SIZE, 
          (UINT8)(TestIndex & 0xFF));
    Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                              UNIT_TEST_ERROR_CODE,
                                              Data,
                                              Size);
    if (EFI_ERROR(Status) != FALSE) {
      DEBUG((DEBUG_WARN, __FUNCTION__ ": Write %d failed with %r...\n", TestIndex, Status));
    }
    
    Status = gRT->SetVariable(VarName,
                              &gEfiHardwareErrorVariableGuid,
                              EFI_VARIABLE_NON_VOLATILE |
                              EFI_VARIABLE_BOOTSERVICE_ACCESS |
                              EFI_VARIABLE_RUNTIME_ACCESS |
                              EFI_VARIABLE_HARDWARE_ERROR_RECORD,
                              0,
                              NULL);
    if (Status == EFI_SUCCESS) {
      DEBUG((DEBUG_INFO, __FUNCTION__ ": Write %d result: %r...\n", TestIndex, Status));
    } else if (Status == EFI_NOT_FOUND) {
      DEBUG((DEBUG_INFO, __FUNCTION__ ": Phase 1 test ceased at %d...\n", TestIndex));
      break;
    } else {
      UT_LOG_ERROR( "Read HwErrRec failed at %d, result: %r.", TestIndex, Status);
      goto Cleanup;
    }
  }

  if (Status != EFI_NOT_FOUND) {
    UT_LOG_ERROR( "Variable service test Phase 1 expect EFI_OUT_OF_RESOURCES, has %r.", Status);
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
    UT_LOG_ERROR( "Write commont variable not succeeded at result: %r.", Status);
    goto Cleanup;
  }

  // Phase 3: Write a HwErrRec should succeed
  Status = ReportStatusCodeWithExtendedData((EFI_ERROR_MAJOR|EFI_ERROR_CODE), 
                                            UNIT_TEST_ERROR_CODE,
                                            Data,
                                            Size);
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_WARNING( "Report Status Code returns non success value.");
  }
  Status = MsWheaVerifyFlashStorage(Framework,
                                    Context,
                                    0, 
                                    UNIT_TEST_ERROR_CODE, 
                                    mMsWheaErrHdr->ErrorSeverity, 
                                    mMsWheaErrHdr->Rev, 
                                    Data, 
                                    Size);
  DEBUG((DEBUG_INFO,"Result: %r \n", Status));
  if (EFI_ERROR(Status) != FALSE) {
    UT_LOG_ERROR( "Written HwErrRec failed to pass verification.");
    goto Cleanup;
  }

  //
  // TODO: Phase 4: Verify overloading HwErrRec will not trigger Reclaim
  //

  utStatus = UNIT_TEST_PASSED;
  UT_LOG_INFO( "Variable service test passed!");
Cleanup:
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...\n"));
  return utStatus;
} // MsWheaVariableServicesTest ()


/**
  MsWheaReportUnitTestAppEntryPoint

  @param[in] ImageHandle              The firmware allocated handle for the EFI image.
  @param[in] SystemTable              A pointer to the EFI System Table.

  @retval EFI_SUCCESS                 The entry point executed successfully.
  @retval other                       Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
MsWheaReportUnitTestAppEntryPoint (
  IN EFI_HANDLE                       ImageHandle,
  IN EFI_SYSTEM_TABLE                 *SystemTable
  )
{
  EFI_STATUS            Status = EFI_ABORTED;
  UNIT_TEST_FRAMEWORK   *Fw = NULL;
  CHAR16                ShortTitle[128];
  UNIT_TEST_SUITE       *Misc = NULL;
  MS_WHEA_TEST_CONTEXT  *MsWheaContext = NULL;
  
  DEBUG((DEBUG_ERROR, __FUNCTION__ " enter\n"));

  MsWheaContext = AllocateZeroPool(sizeof(MS_WHEA_TEST_CONTEXT));
  if (MsWheaContext == NULL) {
    DEBUG(( DEBUG_ERROR, __FUNCTION__ "MS WHEA context allocation failed\n"));
    goto Cleanup;
  }

  SetMem(ShortTitle, sizeof(ShortTitle), 0);
  UnicodeSPrint(ShortTitle, sizeof(ShortTitle), L"%a", gEfiCallerBaseName);
  DEBUG(( DEBUG_ERROR, __FUNCTION__ "%s v%s\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));
  
  // Start setting up the test framework for running the tests.
  Status = InitUnitTestFramework( &Fw, UNIT_TEST_APP_NAME, ShortTitle, UNIT_TEST_APP_VERSION );
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, __FUNCTION__ "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto Cleanup;
  }

  // Misc test suite for all tests.
  CreateUnitTestSuite( &Misc, Fw, L"MS WHEA Miscellaneous Test cases", L"MsWhea.Miscellaneous", NULL, NULL);

  if (Misc == NULL) {
    DEBUG((DEBUG_ERROR, __FUNCTION__ "Failed in CreateUnitTestSuite for TestSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  AddTestCase(Misc, L"Fatal error Rev 0 report", L"MsWhea.Miscellaneous.MsWheaFatalRev0Entries", 
              MsWheaFatalRev0Entries, MsWheaCommonClean, NULL, MsWheaContext );

  AddTestCase(Misc, L"Fatal error Rev 1 report", L"MsWhea.Miscellaneous.MsWheaFatalRev1Entries", 
              MsWheaFatalRev1Entries, MsWheaCommonClean, NULL, MsWheaContext );

  AddTestCase(Misc, L"Fatal unsupported error", L"MsWhea.Miscellaneous.MsWheaFatalRevUnsupEntries", 
              MsWheaFatalRevUnsupEntries, MsWheaCommonClean, NULL, MsWheaContext );

  AddTestCase(Misc, L"Non-fatal error Rev 0 report", L"MsWhea.Miscellaneous.MsWheaNonFatalRev0Entries", 
              MsWheaNonFatalRev0Entries, MsWheaCommonClean, NULL, MsWheaContext );
              
  AddTestCase(Misc, L"Non-fatal error Rev 1 report", L"MsWhea.Miscellaneous.MsWheaNonFatalRev1Entries", 
              MsWheaNonFatalRev1Entries, MsWheaCommonClean, NULL, MsWheaContext );

  AddTestCase(Misc, L"Non-fatal unsupported error", L"MsWhea.Miscellaneous.MsWheaNonFatalRevUnsupEntries", 
              MsWheaNonFatalRevUnsupEntries, MsWheaCommonClean, NULL, MsWheaContext );

  AddTestCase(Misc, L"Wildcard error report", L"MsWhea.Miscellaneous.MsWheaWildcardEntries", 
              MsWheaWildcardEntries, MsWheaCommonClean, NULL, MsWheaContext );

  AddTestCase(Misc, L"Headers only error report", L"MsWhea.Miscellaneous.MsWheaMixedHeaderOnlyEntries", 
              MsWheaMixedHeaderOnlyEntries, MsWheaCommonClean, NULL, MsWheaContext );

  AddTestCase(Misc, L"Short error report", L"MsWhea.Miscellaneous.MsWheaShortEntries", 
              MsWheaShortEntries, MsWheaCommonClean, NULL, MsWheaContext );

  AddTestCase(Misc, L"Stress test should fill up reserved variable space", L"MsWhea.Miscellaneous.MsWheaStressEntries", 
              MsWheaStressEntries, MsWheaCommonClean, NULL, MsWheaContext );

  AddTestCase(Misc, L"Boundary test should probe maximal payload accepted", L"MsWhea.Miscellaneous.MsWheaBoundaryEntries", 
              MsWheaBoundaryEntries, MsWheaCommonClean, MsWheaCommonClean, MsWheaContext );

  AddTestCase(Misc, L"Variable service test should verify Reclaim and quota manipulation", L"MsWhea.Miscellaneous.MsWheaVariableServicesTest", 
              MsWheaVariableServicesTest, MsWheaCommonClean, MsWheaCommonClean, MsWheaContext );

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
  DEBUG((DEBUG_ERROR, __FUNCTION__ " exit\n"));
  return Status;
} // MsWheaReportUnitTestAppEntryPoint ()
