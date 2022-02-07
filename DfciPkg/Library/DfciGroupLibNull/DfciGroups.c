/** @file
DfciGroups.c

Library Instance for Dfci to establish platform settings that are part of Dfci Group settings.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <DfciSystemSettingTypes.h>

#include <Library/DfciGroupLib.h>

/**
 * Return a pointer to the Group Array to DFCI.  This NULL
 * library does not return any group settings.
 */
DFCI_GROUP_ENTRY *
EFIAPI
DfciGetGroupEntries (
  VOID
  )
{
  return NULL;
}
