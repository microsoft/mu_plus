/** @file
DfciVarPolicies.c

This module implements the variable policy settings for the DFCI Variables.


Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DfciManager.h"

#include <DfciVariablePolicies.h>

/**
  InitializeAndSetPolicyForAllDfciVariables

**/
EFI_STATUS
InitializeAndSetPolicyForAllDfciVariables (
  VOID
  )
{
  UINTN                           i;
  EFI_STATUS                      Status;
  EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy = NULL;

  Status = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID **)&VariablePolicy);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a %a: - Locating Variable Policy failed - Code=%r\n", _DBGMSGID_, __FUNCTION__, Status));
    goto Done;
  }

  //
  // Lock most variables at ReadyToBoot
  //
  for (i = 0; i < ARRAY_SIZE (gReadyToBootPolicies); i++) {
    Status = RegisterVarStateVariablePolicy (
               VariablePolicy,
               gReadyToBootPolicies[i].Namespace,
               gReadyToBootPolicies[i].Name,
               gReadyToBootPolicies[i].MinSize,
               gReadyToBootPolicies[i].MaxSize,
               gReadyToBootPolicies[i].AttributesMustHave,
               gReadyToBootPolicies[i].AttributesCantHave,
               &gMuVarPolicyDxePhaseGuid,
               READY_TO_BOOT_INDICATOR_VAR_NAME,
               PHASE_INDICATOR_SET
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a %a: - RegisterVarStateVariablePolicy() ReadyToBoot[%d] returned %r!\n", _DBGMSGID_, __FUNCTION__, i, Status));
      DEBUG ((DEBUG_ERROR, "%a %a: - Error registering %g:%s\n", _DBGMSGID_, __FUNCTION__, gReadyToBootPolicies[i].Namespace, gReadyToBootPolicies[i].Name));
      goto Done;
    }
  }

  //
  // The mailboxes are not locked, but set restrictions for variable sizes and attributes
  //
  for (i = 0; i < ARRAY_SIZE (gMailBoxPolicies); i++) {
    Status = RegisterBasicVariablePolicy (
               VariablePolicy,
               gMailBoxPolicies[i].Namespace,
               gMailBoxPolicies[i].Name,
               gMailBoxPolicies[i].MinSize,
               gMailBoxPolicies[i].MaxSize,
               gMailBoxPolicies[i].AttributesMustHave,
               gMailBoxPolicies[i].AttributesCantHave,
               VARIABLE_POLICY_TYPE_NO_LOCK
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a %a: - RegisterBasicVariablePolicy() ReadyToBoot[%d] returned %r!\n", _DBGMSGID_, __FUNCTION__, i, Status));
      DEBUG ((DEBUG_ERROR, "%a %a: - Error registering %g:%s\n", _DBGMSGID_, __FUNCTION__, gMailBoxPolicies[i].Namespace, gMailBoxPolicies[i].Name));
      goto Done;
    }
  }

Done:

  return Status;
}

/**
  DelateAllMailboxes

  Delete all mailboxes in the error case when DfciManager cannot process variables

**/
EFI_STATUS
DeleteAllMailboxes (
  VOID
  )
{
  UINTN       i;
  EFI_STATUS  Status;
  EFI_STATUS  ReturnStatus;

  ReturnStatus = EFI_SUCCESS;
  for (i = 0; i < ARRAY_SIZE (gMailBoxPolicies); i++) {
    Status = gRT->SetVariable (
                    (CHAR16 *)gMailBoxPolicies[i].Name,
                    (EFI_GUID *)gMailBoxPolicies[i].Namespace,
                    0,
                    0,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a %a: - Unable to delete mailbox %g:%s. Code=%r\n", _DBGMSGID_, __FUNCTION__, gMailBoxPolicies[i].Namespace, gMailBoxPolicies[i].Name, Status));
      ReturnStatus = Status;
    }
  }

  return ReturnStatus;
}
