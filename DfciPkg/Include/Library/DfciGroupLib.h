/** @file
DfciGroupsLib.h

Registers the platform groups and settings within the group.

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
