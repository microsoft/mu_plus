/** @file
DfciVarPolicies.c

This module implements the variable policy settings for the DFCI Variables.


Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DfciManager.h"

typedef struct {
  EFI_GUID    *Namespace;
  CHAR16      *Name;
  UINT32      MinSize;
  UINT32      MaxSize;
  UINT32      AttributesMustHave;
  UINT32      AttributesCantHave;
} VARIABLE_POLICY_ELEMENT;

VARIABLE_POLICY_ELEMENT  gReadyToBootPolicies[] =

{
  {
    .Namespace          = &gDfciInternalVariableGuid, .Name = NULL,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = VARIABLE_POLICY_NO_MAX_SIZE,
    .AttributesMustHave = DFCI_INTERNAL_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_INTERNAL_VAR_ATTRIBUTES,
  },
  {
    .Namespace          = &gDfciSettingsGuid, .Name = NULL,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = VARIABLE_POLICY_NO_MAX_SIZE,
    .AttributesMustHave = DFCI_SETTINGS_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_SETTINGS_ATTRIBUTES,
  },

  {
    .Namespace          = &gDfciDeviceIdVarNamespace, .Name = NULL,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_DEVICE_ID_VARIABLE_SIZE,
    .AttributesMustHave = DFCI_DEVICE_ID_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_DEVICE_ID_VAR_ATTRIBUTES,
  },

  {
    .Namespace          = &gZeroTouchVariableGuid, .Name = NULL,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_ZERO_TOUCH_VAR_SIZE,
    .AttributesMustHave = ZERO_TOUCH_VARIABLE_ATTRIBUTES, .AttributesCantHave = (UINT32) ~ZERO_TOUCH_VARIABLE_ATTRIBUTES,
  },

  {
    .Namespace          = &gDfciAuthProvisionVarNamespace, .Name = DFCI_IDENTITY_CURRENT_VAR_NAME,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_CURRENT_VAR_SIZE,
    .AttributesMustHave = DFCI_IDENTITY_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_IDENTITY_VAR_ATTRIBUTES,
  },
  {
    .Namespace          = &gDfciAuthProvisionVarNamespace, .Name = DFCI_IDENTITY_RESULT_VAR_NAME,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_RESULT_VAR_SIZE,
    .AttributesMustHave = DFCI_IDENTITY_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_IDENTITY_VAR_ATTRIBUTES,
  },
  {
    .Namespace          = &gDfciAuthProvisionVarNamespace, .Name = DFCI_IDENTITY2_RESULT_VAR_NAME,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_RESULT_VAR_SIZE,
    .AttributesMustHave = DFCI_IDENTITY_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_IDENTITY_VAR_ATTRIBUTES,
  },

  {
    .Namespace          = &gDfciPermissionManagerVarNamespace, .Name = DFCI_PERMISSION_POLICY_CURRENT_VAR_NAME,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_CURRENT_VAR_SIZE,
    .AttributesMustHave = DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES,
  },
  {
    .Namespace          = &gDfciPermissionManagerVarNamespace, .Name = DFCI_PERMISSION_POLICY_RESULT_VAR_NAME,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_RESULT_VAR_SIZE,
    .AttributesMustHave = DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES,
  },
  {
    .Namespace          = &gDfciPermissionManagerVarNamespace, .Name = DFCI_PERMISSION2_POLICY_RESULT_VAR_NAME,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_RESULT_VAR_SIZE,
    .AttributesMustHave = DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES,
  },

  {
    .Namespace          = &gDfciSettingsManagerVarNamespace, .Name = DFCI_SETTINGS_CURRENT_OUTPUT_VAR_NAME,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_CURRENT_VAR_SIZE,
    .AttributesMustHave = DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES,
  },
  {
    .Namespace          = &gDfciSettingsManagerVarNamespace, .Name = DFCI_SETTINGS_APPLY_OUTPUT_VAR_NAME,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_RESULT_VAR_SIZE,
    .AttributesMustHave = DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES,
  },
  {
    .Namespace          = &gDfciSettingsManagerVarNamespace, .Name = DFCI_SETTINGS2_APPLY_OUTPUT_VAR_NAME,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_RESULT_VAR_SIZE,
    .AttributesMustHave = DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES,
  },
};

// Set the policy for all of the public mail boxes.
VARIABLE_POLICY_ELEMENT  gMailBoxPolicies[] =
{
  {
    .Namespace          = &gDfciAuthProvisionVarNamespace, .Name = DFCI_IDENTITY_APPLY_VAR_NAME,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_APPLY_VAR_SIZE,
    .AttributesMustHave = DFCI_IDENTITY_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_IDENTITY_VAR_ATTRIBUTES,
  },
  {
    .Namespace          = &gDfciAuthProvisionVarNamespace, .Name = DFCI_IDENTITY2_APPLY_VAR_NAME,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_APPLY_VAR_SIZE,
    .AttributesMustHave = DFCI_IDENTITY_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_IDENTITY_VAR_ATTRIBUTES,
  },

  {
    .Namespace          = &gDfciPermissionManagerVarNamespace, .Name = DFCI_PERMISSION_POLICY_APPLY_VAR_NAME,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_APPLY_VAR_SIZE,
    .AttributesMustHave = DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES,
  },
  {
    .Namespace          = &gDfciPermissionManagerVarNamespace, .Name = DFCI_PERMISSION2_POLICY_APPLY_VAR_NAME,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_APPLY_VAR_SIZE,
    .AttributesMustHave = DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES,
  },

  {
    .Namespace          = &gDfciSettingsManagerVarNamespace, .Name = DFCI_SETTINGS_APPLY_INPUT_VAR_NAME,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_APPLY_VAR_SIZE,
    .AttributesMustHave = DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES,
  },
  {
    .Namespace          = &gDfciSettingsManagerVarNamespace, .Name = DFCI_SETTINGS2_APPLY_INPUT_VAR_NAME,
    .MinSize            = VARIABLE_POLICY_NO_MIN_SIZE, .MaxSize = MAX_ALLOWABLE_DFCI_APPLY_VAR_SIZE,
    .AttributesMustHave = DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES, .AttributesCantHave = (UINT32) ~DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES,
  },
};

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
                    gMailBoxPolicies[i].Name,
                    gMailBoxPolicies[i].Namespace,
                    0,
                    0,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a %a: - Unable to delete mailbox %g:%s\n", _DBGMSGID_, __FUNCTION__, gMailBoxPolicies[i].Namespace, gMailBoxPolicies[i].Name));
      ReturnStatus = Status;
    }
  }

  return ReturnStatus;
}
