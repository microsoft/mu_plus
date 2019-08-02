/** @file
DfciEventGroup.h

Contains definitions for Dfci remote conficuration started and ended. This event
group it meant to inform a platform when a long running operation is being performed.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_EVENT_GROUP_GUID__
#define __DFCI_EVENT_GROUP_GUID__

//gDfciConfigStartEventGroupGuid is used to signal the start of a DFCI configuration update
extern EFI_GUID gDfciConfigStartEventGroupGuid;

//gDfciConfigCompleteEventGroupGuid is used to signal that the DFCI configuration update finished
extern EFI_GUID gDfciConfigCompleteEventGroupGuid;

#endif