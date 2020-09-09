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

//
// This library was designed with advanced unit-test features.
// This define handles the configuration.
#ifdef INTERNAL_UNIT_TEST
#undef STATIC
#define STATIC    // Nothing...
#endif

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
  EFI_STATUS                          Status;
  UINT32                              ErrorSeverity;
  MS_WHEA_ERROR_ENTRY_MD              MsWheaEntryMD;
  MS_WHEA_RSC_INTERNAL_ERROR_DATA     *DataHeader;
  MS_WHEA_ERROR_EXTRA_SECTION_DATA    *ExtraData;
  UINT32                              ExtraDataSize;

  Status    = EFI_SUCCESS;
  ExtraData = NULL;

  // If we don't have a ReportFn, this is a problem with the caller.
  if (ReportFn == NULL) {
    DEBUG((DEBUG_ERROR, "%a - Input function pointer cannot be null!\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    ASSERT_EFI_ERROR(Status);
    goto Cleanup;
  }

  // If this isn't a code type we recognize, we can bail early.
  if ((CodeType & MS_WHEA_ERROR_STATUS_TYPE_INFO) == MS_WHEA_ERROR_STATUS_TYPE_INFO) {
    ErrorSeverity = EFI_GENERIC_ERROR_INFO;
  } else if ((CodeType & MS_WHEA_ERROR_STATUS_TYPE_FATAL) == MS_WHEA_ERROR_STATUS_TYPE_FATAL) {
    ErrorSeverity = EFI_GENERIC_ERROR_FATAL;
  } else {
    Status = EFI_SUCCESS;
    goto Cleanup;
  }

  // If Data is provided, we need to validate it.
  if (Data != NULL) {
    // If the Data doesn't look right, bail.
    if (Data->HeaderSize != sizeof(EFI_STATUS_CODE_DATA) ||
        Data->Size < sizeof(MS_WHEA_RSC_INTERNAL_ERROR_DATA) ||
        CompareMem(&Data->Type, &gMsWheaRSCDataTypeGuid, sizeof(EFI_GUID)) != 0) {
      // Bail logging telemetry, this is not for us...
      DEBUG ((DEBUG_ERROR, "%a - Unrecognized data provided! Bailing!\n", __FUNCTION__));
      Status = EFI_UNSUPPORTED;
      goto Cleanup;
    }
  }

  // 
  // Alright, so if we're still here, we should have somewhat sanitized data to process.
  // 
  ZeroMem(&MsWheaEntryMD, sizeof(MsWheaEntryMD));

  // Populate the things we KNOW we have.
  MsWheaEntryMD.Rev               = MS_WHEA_REV_0;
  MsWheaEntryMD.Phase             = CurrentPhase;
  MsWheaEntryMD.ErrorSeverity     = ErrorSeverity;
  MsWheaEntryMD.ErrorStatusValue  = Value;
  if (CallerId != NULL) {
    CopyGuid(&MsWheaEntryMD.ModuleID, CallerId);
  }

  // Now, if we've still got some data, we know if must at least have room
  // for the MS_WHEA_RSC_INTERNAL_ERROR_DATA.
  if (Data != NULL) {
    DataHeader = (MS_WHEA_RSC_INTERNAL_ERROR_DATA*)((UINT8*)Data + Data->HeaderSize);
    MsWheaEntryMD.AdditionalInfo1 = DataHeader->AdditionalInfo1;
    MsWheaEntryMD.AdditionalInfo2 = DataHeader->AdditionalInfo2;
    CopyGuid(&MsWheaEntryMD.LibraryID, &DataHeader->LibraryID);
    CopyGuid(&MsWheaEntryMD.IhvSharingGuid, &DataHeader->IhvSharingGuid);

    // If the data size is larger, we must have extra data. Let's
    // try to store that, too.
    // NOTE: Don't allocate space for ExtraData if not in at least DXE.
    if (Data->Size > (sizeof(MS_WHEA_RSC_INTERNAL_ERROR_DATA) + sizeof(EFI_GUID)) && CurrentPhase != MS_WHEA_PHASE_PEI) {
      ExtraDataSize = Data->Size - sizeof(MS_WHEA_RSC_INTERNAL_ERROR_DATA) - sizeof(EFI_GUID);
      ExtraData = AllocatePool(sizeof(*ExtraData) + ExtraDataSize);
      if (ExtraData != NULL) {
        ExtraData->DataSize = ExtraDataSize;
        CopyGuid(&ExtraData->SectionGuid, (EFI_GUID*)((UINT8*)DataHeader + sizeof(MS_WHEA_RSC_INTERNAL_ERROR_DATA)));
        CopyMem(ExtraData->Data, (UINT8*)DataHeader + sizeof(MS_WHEA_RSC_INTERNAL_ERROR_DATA) + sizeof(EFI_GUID), ExtraDataSize);
      }
      MsWheaEntryMD.ExtraSection = ExtraData;
    }
  }

  // Now we need to make a decision about how to report, either to EarlyStorage, or full reporting.
  if ((ErrorSeverity == EFI_GENERIC_ERROR_FATAL) &&
      ((CurrentPhase == MS_WHEA_PHASE_DXE) || (CurrentPhase == MS_WHEA_PHASE_PEI))) {
    Status = MsWheaESStoreEntry(&MsWheaEntryMD);
  } else {
    Status = ReportFn(&MsWheaEntryMD);
  }

Cleanup:
  if (ExtraData != NULL) {
    FreePool(ExtraData);
  }
  return Status;
}
