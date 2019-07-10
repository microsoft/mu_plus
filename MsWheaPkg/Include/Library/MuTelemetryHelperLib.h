/** @file -- MuTelemetryHelperLib.h
Helper functions to make it even easier to report telemetry events in
source code.

Copyright (c) 2019, Microsoft Corporation

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
