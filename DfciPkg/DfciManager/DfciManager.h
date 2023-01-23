/** @file
DfciManager.h

Header file for Dfci Manager implementation

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_MANAGER_H__
#define __DFCI_MANAGER_H__

#include <Uefi.h>

#include <DfciSystemSettingTypes.h>

#include <Guid/DfciPacketHeader.h>
#include <Guid/DfciDeviceIdVariables.h>
#include <Guid/DfciIdentityAndAuthManagerVariables.h>
#include <Guid/DfciInternalVariableGuid.h>
#include <Guid/DfciPacketHeader.h>
#include <Guid/DfciPermissionManagerVariables.h>
#include <Guid/DfciSettingsGuid.h>
#include <Guid/DfciSettingsManagerVariables.h>
#include <Guid/ZeroTouchVariables.h>

#include <Protocol/DfciApplyPacket.h>
#include <Protocol/VariablePolicy.h>

#include <Private/DfciGlobalPrivate.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DfciDeviceIdSupportLib.h>
#include <Library/DfciSettingChangedNotificationLib.h>
#include <Library/DfciUiSupportLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PerformanceLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/VariablePolicyHelperLib.h>

#define _DBGMSGID_  "[DM]"

/**
  InitializeAndSetPolicyForAllDfciVariables

**/
EFI_STATUS
InitializeAndSetPolicyForAllDfciVariables (
  VOID
  );

/**
  DelateAllMailboxes

  Delete all mailboxes in the error case when DfciManager cannot process variables

**/
EFI_STATUS
DeleteAllMailboxes (
  VOID
  );

#endif // __DFCI_MANAGER_H__
