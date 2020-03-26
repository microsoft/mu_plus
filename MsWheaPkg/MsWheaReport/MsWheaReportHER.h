/** @file -- MsWheaReportHER.h

This header defines API that will save supplied payload via HwErrRec.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MS_WHEA_REPORT_HER__
#define __MS_WHEA_REPORT_HER__

#include "MsWheaReportCommon.h"

/**

Clear all the HwErrRec entries on flash.

@retval EFI_SUCCESS                     Entry addition is successful.
@retval Others                          See GetVariable/SetVariable for more details

**/
EFI_STATUS
EFIAPI
MsWheaClearAllEntries (
  VOID
);

/**

This routine accepts the pointer to the MS WHEA entry metadata, error specific data payload and its size
then store on the flash as HwErrRec awaiting to be picked up by OS (Refer to UEFI Spec 2.7A)

@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error metadata

@retval EFI_SUCCESS                   Entry addition is successful.
@retval EFI_INVALID_PARAMETER         Input has NULL pointer as input.
@retval EFI_OUT_OF_RESOURCES          Not enough space for the requested space.

**/
EFI_STATUS
EFIAPI
MsWheaReportHERAdd (
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
);

/********************** HELPER FUNCTIONS **********************/

/**

Returns the value of a variable. See definition of EFI_GET_VARIABLE in
Include/Uefi/UefiSpec.h.

@retval EFI_NOT_READY                 If requested service is not yet available
@retval Others                        See EFI_GET_VARIABLE for more details

**/
EFI_STATUS
EFIAPI
WheaGetVariable (
  IN     CHAR16                      *VariableName,
  IN     EFI_GUID                    *VendorGuid,
  OUT    UINT32                      *Attributes,    OPTIONAL
  IN OUT UINTN                       *DataSize,
  OUT    VOID                        *Data           OPTIONAL
);

/**

Enumerates the current variable names. See definition of EFI_GET_NEXT_VARIABLE_NAME in
Include/Uefi/UefiSpec.h.

@retval EFI_NOT_READY                 If requested service is not yet available
@retval Others                        See EFI_GET_NEXT_VARIABLE_NAME for more details

**/
EFI_STATUS
EFIAPI
WheaGetNextVariableName (
  IN OUT UINTN                    *VariableNameSize,
  IN OUT CHAR16                   *VariableName,
  IN OUT EFI_GUID                 *VendorGuid
);

/**
Sets the value of a variable. See definition of EFI_SET_VARIABLE in
Include/Uefi/UefiSpec.h.

@retval EFI_NOT_READY                 If requested service is not yet available
@retval Others                        See EFI_SET_VARIABLE for more details

**/
EFI_STATUS
EFIAPI
WheaSetVariable (
  IN  CHAR16                       *VariableName,
  IN  EFI_GUID                     *VendorGuid,
  IN  UINT32                       Attributes,
  IN  UINTN                        DataSize,
  IN  VOID                         *Data
);

#endif // __MS_WHEA_REPORT_HER__
