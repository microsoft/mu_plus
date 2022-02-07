/** @file -- MuTelemetryHelperLib.c
Helper functions to make it even easier to report telemetry events in
source code.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <MsWheaErrorStatus.h>
#include <Guid/MsWheaReportDataType.h>

#include <Library/ReportStatusCodeLib.h>

#include <Library/MuTelemetryHelperLib.h>

STATIC
VOID
FillOutMsWheaData (
  IN  CONST EFI_GUID                   *LibraryId OPTIONAL,
  IN  CONST EFI_GUID                   *IhvId OPTIONAL,
  IN  UINT64                           ExtraData1,
  IN  UINT64                           ExtraData2,
  OUT MS_WHEA_RSC_INTERNAL_ERROR_DATA  *ErrorData
  )
{
  // Zero structure.
  ZeroMem (ErrorData, sizeof (*ErrorData));

  // set the values, and report the event.
  if (LibraryId != NULL) {
    CopyGuid (&ErrorData->LibraryID, LibraryId);
  }

  if (IhvId != NULL) {
    CopyGuid (&ErrorData->IhvSharingGuid, IhvId);
  }

  ErrorData->AdditionalInfo1 = ExtraData1;
  ErrorData->AdditionalInfo2 = ExtraData2;
} // FillOutMsWheaData()

/**
  Helper function to minimize the number of parameters for a critical telemetry event
  and to clarify the purpose of each field.

  This helper constructs the most detailed report possible that can still be persisted in case
  of power loss or reset. It will log a critical fatal event through the telemetry system.

  @param[in]  ComponentId   [Optional] This identifier should uniquely identify the module that is emitting this
                            event. When this is passed in as NULL, report status code will automatically populate
                            this field with gEfiCallerIdGuid.
  @param[in]  ClassId       An EFI_STATUS_CODE_VALUE representing the event that has occurred. This
                            value will occupy the same space as EventId from LogCriticalEvent(), and
                            should be unique enough to identify a module or region of code.
  @param[in]  LibraryId     This should identify the library that is emitting this event.
  @param[in]  IhvId         This should identify the Ihv related to this event if applicable. For example,
                            this would typically be used for TPM and SOC specific events.
  @param[in]  ExtraData1    [Optional] This should be data specific to the cause. Ideally, used to contain contextual
                            or runtime data related to the event (e.g. register contents, failure codes, etc.).
                            It will be persisted.
  @param[in]  ExtraData2    [Optional] Another UINT32 similar to ExtraData1.


  @retval     EFI_SUCCESS             Event has been reported.
  @retval     EFI_ERROR               Something has gone wrong.

**/
EFI_STATUS
EFIAPI
LogTelemetry (
  IN  BOOLEAN                IsFatal,
  IN  EFI_GUID               *ComponentId OPTIONAL,
  IN  EFI_STATUS_CODE_VALUE  ClassId,
  IN  EFI_GUID               *LibraryId OPTIONAL,
  IN  EFI_GUID               *IhvId OPTIONAL,
  IN  UINT64                 ExtraData1,
  IN  UINT64                 ExtraData2
  )
{
  EFI_STATUS                       Status;
  MS_WHEA_RSC_INTERNAL_ERROR_DATA  EventHeader;
  EFI_STATUS_CODE_TYPE             Severity;

  Status = EFI_SUCCESS;

  Severity = MS_WHEA_ERROR_STATUS_TYPE_FATAL;
  if (!IsFatal) {
    Severity = MS_WHEA_ERROR_STATUS_TYPE_INFO;
  }

  FillOutMsWheaData (
    LibraryId,
    IhvId,
    ExtraData1,
    ExtraData2,
    &EventHeader
    );

  // Report the event
  Status = ReportStatusCodeEx (
             Severity,
             ClassId,
             0,
             ComponentId,
             &gMsWheaRSCDataTypeGuid,
             &EventHeader,
             sizeof (EventHeader)
             );

  return Status;
} // LogTelemetry()

/**
  Helper function to minimize the number of parameters for a critical telemetry event
  and to clarify the purpose of each field.

  This helper constructs the most detailed report possible that can still be persisted in case
  of power loss or reset. It will log a critical fatal event through the telemetry system.

  This Ex implementation allows the caller to pass an ID and an arbitrary data buffer. If provided,
  this will create an additional WHEA section with the specified GUID and containing the Data.

  @param[in]  ComponentId   [Optional] This identifier should uniquely identify the module that is emitting this
                            event. When this is passed in as NULL, report status code will automatically populate
                            this field with gEfiCallerIdGuid.
  @param[in]  ClassId       An EFI_STATUS_CODE_VALUE representing the event that has occurred. This
                            value will occupy the same space as EventId from LogCriticalEvent(), and
                            should be unique enough to identify a module or region of code.
  @param[in]  LibraryId     This should identify the library that is emitting this event.
  @param[in]  IhvId         This should identify the Ihv related to this event if applicable. For example,
                            this would typically be used for TPM and SOC specific events.
  @param[in]  ExtraData1    [Optional] This should be data specific to the cause. Ideally, used to contain contextual
                            or runtime data related to the event (e.g. register contents, failure codes, etc.).
                            It will be persisted.
  @param[in]  ExtraData2    [Optional] Another UINT32 similar to ExtraData1.
  @param[in]  ExtraExtraId    [Optional] You know those times that you just can't seem to save enough data?
  @param[in]  ExtraExtraSize  [Optional] When you feel that 384 bits just doesn't get it done?
  @param[in]  ExtraExtraData  [Optional] Well... this one's for you.

  @retval     EFI_SUCCESS             Event has been reported.
  @retval     EFI_ERROR               Something has gone wrong.

**/
EFI_STATUS
EFIAPI
LogTelemetryEx (
  IN  BOOLEAN                IsFatal,
  IN  EFI_GUID               *ComponentId OPTIONAL,
  IN  EFI_STATUS_CODE_VALUE  ClassId,
  IN  EFI_GUID               *LibraryId OPTIONAL,
  IN  EFI_GUID               *IhvId OPTIONAL,
  IN  UINT64                 ExtraData1,
  IN  UINT64                 ExtraData2,
  IN  CONST EFI_GUID         *ExtraExtraId,
  IN  UINTN                  ExtraExtraSize,
  IN  CONST VOID             *ExtraExtraData
  )
{
  EFI_STATUS            Status;
  UINTN                 FullBufferSize;
  UINT8                 *FullBuffer;
  EFI_STATUS_CODE_TYPE  Severity;

  Status     = EFI_SUCCESS;
  FullBuffer = NULL;

  // If only SOME of the parameters are provided, fail.
  if (((ExtraExtraId != NULL) && (ExtraExtraData == NULL)) ||
      ((ExtraExtraId == NULL) && (ExtraExtraData != NULL)))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Only need to figure out the buffer allocation if there's extra data.
  //
  if ((ExtraExtraSize > 0) && (ExtraExtraData != NULL)) {
    FullBufferSize = sizeof (MS_WHEA_RSC_INTERNAL_ERROR_DATA) + sizeof (EFI_GUID) + ExtraExtraSize;
    ASSERT (FullBufferSize > sizeof (MS_WHEA_RSC_INTERNAL_ERROR_DATA));
    // TODO: Consider doing this with a stack buffer and max size for maximum
    //        flexibility.
    if (FullBufferSize > (sizeof (MS_WHEA_RSC_INTERNAL_ERROR_DATA) + sizeof (EFI_GUID))) {
      FullBuffer = AllocatePool (FullBufferSize);
    }
  }

  //
  // If we don't have a buffer to work with (either because there was no data or
  // the buffer allocation failed), just alias back to the lesser call.
  //
  if (FullBuffer == NULL) {
    return LogTelemetry (
             IsFatal,
             ComponentId,
             ClassId,
             LibraryId,
             IhvId,
             ExtraData1,
             ExtraData2
             );
  }

  Severity = MS_WHEA_ERROR_STATUS_TYPE_FATAL;
  if (!IsFatal) {
    Severity = MS_WHEA_ERROR_STATUS_TYPE_INFO;
  }

  FillOutMsWheaData (
    LibraryId,
    IhvId,
    ExtraData1,
    ExtraData2,
    (MS_WHEA_RSC_INTERNAL_ERROR_DATA *)FullBuffer
    );
  CopyGuid ((EFI_GUID *)(FullBuffer + sizeof (MS_WHEA_RSC_INTERNAL_ERROR_DATA)), ExtraExtraId);
  CopyMem (
    FullBuffer + sizeof (MS_WHEA_RSC_INTERNAL_ERROR_DATA) + sizeof (EFI_GUID),
    ExtraExtraData,
    (FullBufferSize - sizeof (MS_WHEA_RSC_INTERNAL_ERROR_DATA)) - sizeof (EFI_GUID)
    );

  // Report the event
  Status = ReportStatusCodeEx (
             Severity,
             ClassId,
             0,
             ComponentId,
             &gMsWheaRSCDataTypeGuid,
             FullBuffer,
             FullBufferSize
             );

  if (FullBuffer != NULL) {
    FreePool (FullBuffer);
  }

  return Status;
} // LogTelemetryEx()
