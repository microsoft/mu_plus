/** @file -- MuTelemetryHelperLib.h
Helper functions to make it even easier to report telemetry events in
source code.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MU_TELEMETRY_HELPER_LIB_H_
#define _MU_TELEMETRY_HELPER_LIB_H_

// TODO: This is against common Mu practice.
#include <Library/ReportStatusCodeLib.h>

// TODO: Make this a MACRO so it can be disabled.

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
  IN  BOOLEAN                   IsFatal,
  IN  EFI_GUID*                 ComponentId OPTIONAL,
  IN  EFI_STATUS_CODE_VALUE     ClassId,
  IN  EFI_GUID*                 LibraryId OPTIONAL,
  IN  EFI_GUID*                 IhvId OPTIONAL,
  IN  UINT64                    ExtraData1,
  IN  UINT64                    ExtraData2
  );


#endif // _MU_TELEMETRY_HELPER_LIB_H_
