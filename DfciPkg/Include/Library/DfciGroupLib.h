/** @file
DfciGroupsLib.h

Registers the platform groups and settings within the group.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

typedef struct {
    DFCI_SETTING_ID_STRING    GroupId;             // Pointer to Group Id
    DFCI_SETTING_ID_STRING   *GroupMembers;        // Pointer to array of Setting Id's
} DFCI_GROUP_ENTRY;

/**
 * Register a setting as a member of a group.
 *
 * 1. Settings can only be member of one group
 * 2. A GroupId cannot be a Setting Id.
 *
 * @param GroupId     - Group
 * @param Id          - Setting to add to group
 *
 * @return EFI_NOT_FOUND       -- Id not available on this system
 * @return EFI_ALREADY_STARTD  -- Id is already in a group
 *                                or Group name exists as a setting
 * @return EFI_SUCCESS         -- Id Registered to group.
 *                                If this is the first registered setting
 *                                the group is created
 */
DFCI_GROUP_ENTRY *
EFIAPI
DfciGetGroupEntries (VOID);
