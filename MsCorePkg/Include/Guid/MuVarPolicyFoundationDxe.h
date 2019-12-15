/** @file -- MuVarPolicyFoundationDxe.h
Support definitions for working with the Variable Policies that were created with
the MuVarPolicyFoundationDxe driver.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MU_VAR_POLICY_FOUNDATION_DXE_H_
#define _MU_VAR_POLICY_FOUNDATION_DXE_H_

typedef BOOLEAN   PHASE_INDICATOR;

#define PHASE_INDICATOR_SET                     TRUE

// NOTE: Variable Attributes
// When locating one of the phase indication variables, the attributes should be compared.
// If the attributes don't match, that's a security error and the indicator cannot be trusted.
#define DXE_PHASE_INDICATOR_ATTR                (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)     // Volatile, BS and RT

// EndOfDxe Indicator
#define END_OF_DXE_INDICATOR_VAR_NAME           L"EOD"
#define END_OF_DXE_INDICATOR_VAR_ATTR           DXE_PHASE_INDICATOR_ATTR

// ReadyToBoot Indicator
#define READY_TO_BOOT_INDICATOR_VAR_NAME        L"RTB"
#define READY_TO_BOOT_INDICATOR_VAR_ATTR        DXE_PHASE_INDICATOR_ATTR

// ExitBootServices Indicator
#define EXIT_BOOT_SERVICES_INDICATOR_VAR_NAME   L"EBS"
#define EXIT_BOOT_SERVICES_INDICATOR_VAR_ATTR   DXE_PHASE_INDICATOR_ATTR

typedef BOOLEAN   POLICY_LOCK_VAR;
#define WRITE_ONCE_STATE_VAR_ATTR               (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)     // Volatile, BS and RT

#endif // _MU_VAR_POLICY_FOUNDATION_DXE_H_
