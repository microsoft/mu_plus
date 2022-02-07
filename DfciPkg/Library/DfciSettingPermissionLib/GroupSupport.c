/**@file
SettingsManager.c

Implements the SettingAccess Provider

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DfciSettingPermission.h"
#include <Library/DfciGroupLib.h>

LIST_ENTRY  mGroupList = INITIALIZE_LIST_HEAD_VARIABLE (mGroupList);       // linked list for the groups

/**
 * Register a Group.
 *
 * @param GroupId     - Group Id to register
 *
 * @return EFI_ALREADY_STARTED  -- GroupId is already in a group
 *                                or Group name exists as a setting
 * @return EFI_SUCCESS         -- Id Registered to group.
 *                                If this is the first registered setting
 *                                the group is created
 */
EFI_STATUS
RegisterGroup (
  IN DFCI_SETTING_ID_STRING  GroupId
  )
{
  DFCI_GROUP_LIST_ENTRY  *Group;
  EFI_STATUS             Status;

  Group = FindGroup (GroupId);
  if (NULL == Group) {
    // Create New Group entry.
    Group = (DFCI_GROUP_LIST_ENTRY *)AllocatePool (sizeof (DFCI_GROUP_LIST_ENTRY));
    if (NULL == Group) {
      return EFI_OUT_OF_RESOURCES;
    }

    Group->Signature = DFCI_GROUP_LIST_ENTRY_SIGNATURE;
    Group->GroupId   = GroupId;
    InsertTailList (&mGroupList, &Group->GroupLink);
    InitializeListHead (&Group->MemberHead);
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_ALREADY_STARTED;
  }

  return Status;
}

/**
 * Register a setting as a member of a group.
 *
 * 1. A GroupId cannot be a Setting Id.
 *
 * @param PList       - The setting Id name
 *
 * @return EFI_NOT_FOUND       -- Id not available on this system
 * @return EFI_SUCCESS         -- Id Registered to group.
 *                                If this is the first registered setting in a group,
 *                                the group is created
 */
EFI_STATUS
EFIAPI
RegisterSettingToGroup (
  IN DFCI_SETTING_ID_STRING  Id
  )
{
  DFCI_GROUP_ENTRY        *GroupEntry;
  DFCI_GROUP_LIST_ENTRY   *Group;
  DFCI_MEMBER_LIST_ENTRY  *Member;
  DFCI_SETTING_ID_STRING  *Setting;
  EFI_STATUS              Status;

  Group = FindGroup (Id);
  if (NULL != Group) {
    // Don't allow a setting Id to exist as a group Id
    ASSERT (NULL == Group);
    return EFI_UNSUPPORTED;
  }

  //
  // Check if the setting is to be related to a group
  //
  Status     = EFI_NOT_FOUND;
  GroupEntry = DfciGetGroupEntries ();
  if (NULL != GroupEntry) {
    while (NULL != GroupEntry->GroupId) {
      // Get NULL terminated list of settings that are part of this group.  Each group has
      // a NULL terminated list of settings that are part of the group.
      Setting = GroupEntry->GroupMembers;
      while (*Setting != NULL) {
        if (0 == AsciiStrnCmp (Id, *Setting, DFCI_MAX_ID_LEN)) {
          // See if this group exists.
          Group = FindGroup (GroupEntry->GroupId);
          if (NULL == Group) {
            // Register the first instance of this group
            Status = RegisterGroup (GroupEntry->GroupId);
            if (!EFI_ERROR (Status)) {
              Group = FindGroup (GroupEntry->GroupId);
            }

            if (NULL == Group) {
              DEBUG ((DEBUG_ERROR, "Unable to create group for setting %a, group %a\n", Id, Group->GroupId));
              return EFI_OUT_OF_RESOURCES;
            }
          }

          // Allocate a member entry for this setting
          Member = (DFCI_MEMBER_LIST_ENTRY *)AllocatePool (sizeof (DFCI_MEMBER_LIST_ENTRY));
          if (NULL == Member) {
            ASSERT (NULL != Member);
            return EFI_OUT_OF_RESOURCES;
          }

          Member->Signature = DFCI_MEMBER_ENTRY_SIGNATURE;
          Member->Id        = Id;
          InsertTailList (&Group->MemberHead, &Member->MemberLink);
          DEBUG ((DEBUG_INFO, "Setting %a added to group %a\n", Id, Group->GroupId));
          Status = EFI_SUCCESS;
          break;
        }

        Setting++;
      }

      GroupEntry++;
    }
  }

  return Status;
}

/*
Helper function to print out all Groups currently registered
*/
VOID
EFIAPI
DebugPrintGroups (
  )
{
  LIST_ENTRY              *Link  = NULL;
  LIST_ENTRY              *Link2 = NULL;
  DFCI_GROUP_LIST_ENTRY   *Group;
  DFCI_MEMBER_LIST_ENTRY  *Member;

  DEBUG ((DEBUG_INFO, "-----------------------------------------------------\n"));
  DEBUG ((DEBUG_INFO, "START PRINTING ALL REGISTERED GROUPS\n"));
  DEBUG ((DEBUG_INFO, "-----------------------------------------------------\n"));

  EFI_LIST_FOR_EACH (Link, &mGroupList) {
    Group = GROUP_LIST_ENTRY_FROM_GROUP_LINK (Link);
    DEBUG ((DEBUG_INFO, "Group %a members:\n", Group->GroupId));
    EFI_LIST_FOR_EACH (Link2, &Group->MemberHead) {
      Member = MEMBER_LIST_ENTRY_FROM_MEMBER_LINK (Link2);
      DEBUG ((DEBUG_INFO, "      %a\n", Member->Id));
    }
  }

  DEBUG ((DEBUG_INFO, "-----------------------------------------------------\n"));
  DEBUG ((DEBUG_INFO, " END PRINTING ALL REGISTERED GROUPS\n"));
  DEBUG ((DEBUG_INFO, "-----------------------------------------------------\n"));
}

/*
Find a group.
*/
DFCI_GROUP_LIST_ENTRY *
EFIAPI
FindGroup (
  DFCI_SETTING_ID_STRING  Id
  )
{
  LIST_ENTRY             *Link;
  DFCI_GROUP_LIST_ENTRY  *Group = NULL;

  EFI_LIST_FOR_EACH (Link, &mGroupList) {
    Group = GROUP_LIST_ENTRY_FROM_GROUP_LINK (Link);
    if (0 == AsciiStrnCmp (Group->GroupId, Id, DFCI_MAX_ID_LEN)) {
      DEBUG ((DEBUG_INFO, "FindGroup - Found (%a)\n", Id));
      return Group;
    }
  }

  DEBUG ((DEBUG_INFO, "FindGroup - Failed to find (%a)\n", Id));
  return NULL;
}

/*
Find a group by setting - return the GroupId of a setting if it is a member of a group.

@param      Id        The setting Id
@param      Key       Set to NULL to obtain the first GroupId the settings is within
                      or the previous value to find the next group

@return     Group Id  if a member of a group with explicit permissions
                      NULL of not (Key == NULL)
                      NULL of no more groups (Key != NULL)
*/
DFCI_SETTING_ID_STRING
EFIAPI
FindGroupIdBySetting (
  DFCI_SETTING_ID_STRING  Id,
  VOID                    **Key OPTIONAL
  )
{
  LIST_ENTRY              *Link;
  LIST_ENTRY              *Link2;
  DFCI_GROUP_LIST_ENTRY   *Group;
  DFCI_MEMBER_LIST_ENTRY  *Member;
  BOOLEAN                 PreviousFound;

  if (Key == NULL) {
    // Key is a required parameter
    return NULL;
  }

  PreviousFound = (*Key == NULL) ? TRUE : FALSE;
  EFI_LIST_FOR_EACH (Link, &mGroupList) {
    Group = GROUP_LIST_ENTRY_FROM_GROUP_LINK (Link);
    if (*Key == (VOID *)Group) {
      PreviousFound = TRUE;
      continue;
    }

    if (!PreviousFound) {
      continue;
    }

    EFI_LIST_FOR_EACH (Link2, &Group->MemberHead) {
      Member = MEMBER_LIST_ENTRY_FROM_MEMBER_LINK (Link2);
      if (0 == AsciiStrnCmp (Id, Member->Id, DFCI_MAX_ID_LEN)) {
        DEBUG ((DEBUG_INFO, "FindGroup Setting - %a is a member of a group %a\n", Id, Group->GroupId));
        *Key = (VOID *)Group;
        return Group->GroupId;
      }
    }
  }

  return NULL;
}
