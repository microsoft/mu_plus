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

#endif // __MS_WHEA_REPORT_HER__
