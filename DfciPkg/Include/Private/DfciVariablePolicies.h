/** @file
DfciVarPolicies.h

This header file defines the Variable Policies for DFCI.


Copyright (C) Microsoft Corporation. All rights reserved.

**/

#ifndef __DFCI_VAR_POLICIES_H__
#define __DFCI_VAR_POLICIES_H__

typedef struct {
  CONST EFI_GUID    *Namespace;
  CONST CHAR16      *Name;
  UINT32            MinSize;
  UINT32            MaxSize;
  UINT32            AttributesMustHave;
  UINT32            AttributesCantHave;
} VARIABLE_POLICY_ELEMENT;

STATIC VARIABLE_POLICY_ELEMENT  gReadyToBootPolicies[] =

{
  // Identity and Auth variables
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

  // Permission variables
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

  // Settings variables
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

  // Wild card policies at the end of the list so specific policies are found first
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

#endif // __DFCI_VAR_POLICIES_H__
