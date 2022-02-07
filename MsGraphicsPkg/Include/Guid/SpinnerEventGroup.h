/** @file
SpinnerEventGroup.h

Contains definitions for the Timeout Spinners 1 - 4 start and stop. This event
group it meant to inform a platform user when a long running operation is being performed.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SPINNER_EVENT_GROUP_GUID__
#define __SPINNER_EVENT_GROUP_GUID__

// Event groups for spinner1
extern  EFI_GUID  gGeneralSpinner1StartEventGroupGuid;
extern  EFI_GUID  gGeneralSpinner1CompleteEventGroupGuid;

// Event groups for spinner2
extern  EFI_GUID  gGeneralSpinner2StartEventGroupGuid;
extern  EFI_GUID  gGeneralSpinner2CompleteEventGroupGuid;

// Event groups for spinner3
extern  EFI_GUID  gGeneralSpinner3tartEventGroupGuid;
extern  EFI_GUID  gGeneralSpinner3CompleteEventGroupGuid;

// Event groups for spinner4
extern  EFI_GUID  gGeneralSpinner4StartEventGroupGuid;
extern  EFI_GUID  gGeneralSpinner4CompleteEventGroupGuid;

#endif
