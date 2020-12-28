/** @file
DfciGroupLib.h

Registers the platform groups and settings within the group.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

typedef struct {
    DFCI_SETTING_ID_STRING    GroupId;             // Pointer to Group Id
    DFCI_SETTING_ID_STRING   *GroupMembers;        // Pointer to array of Setting Id's
} DFCI_GROUP_ENTRY;

/**
 * Acquire the platforms group settings.
 *
 *
 * @return DFCI_GROUP_ENTRY     Array of group settings. NULL if there are no
 *                              group settings.
 *
 */
DFCI_GROUP_ENTRY *
EFIAPI
DfciGetGroupEntries (VOID);
