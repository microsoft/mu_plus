/** @file
DfciIdentityAndAuthManagerVariables.h

This file defines the GUID for NVRAM variables used by DFCI features
internally. This GUID should not be reused for anything outside of
private/internal DFCI Modules

The only reason a public header file is created is so that the namespace
can be declared and locked by the core var locking lib.  Individual
variable structure and def are private.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_INTERNAL_VARIABLE_GUID_H__
#define __DFCI_INTERNAL_VARIABLE_GUID_H__

#define DFCI_INTERNAL_VAR_ATTRIBUTES  (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS) 

extern EFI_GUID gDfciInternalVariableGuid;

#endif // __DFCI_INTERNAL_VARIABLE_GUID_H__
