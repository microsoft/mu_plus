/** @file -- MuVarPolicyFoundationDxe.h
Support definitions for working with the Variable Policies that were created with
the MuVarPolicyFoundationDxe driver.

Copyright (C) Microsoft Corporation

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

#ifndef _MU_VAR_POLICY_FOUNDATION_DXE_H_
#define _MU_VAR_POLICY_FOUNDATION_DXE_H_

typedef BOOLEAN   PHASE_INDICATOR;

// NOTE: Variable Attributes
// When locating one of the phase indication variables, the attributes should be compared.
// If the attributes don't match, that's a security error and the indicator cannot be trusted.
#define DXE_PHASE_INDICATOR_ATTR                (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)     // Volatile, BS and RT

// EndOfDxe Indicator
#define END_OF_DXE_INDICATOR_VAR_NAME           L"EndOfDxeSignalled"
#define END_OF_DXE_INDICATOR_VAR_ATTR           DXE_PHASE_INDICATOR_ATTR

// ReadyToBoot Indicator
#define READY_TO_BOOT_INDICATOR_VAR_NAME        L"ReadyToBootSignalled"
#define READY_TO_BOOT_INDICATOR_VAR_ATTR        DXE_PHASE_INDICATOR_ATTR

// ExitBootServices Indicator
#define EXIT_BOOT_SERVICES_INDICATOR_VAR_NAME   L"ExitBootServicesSignalled"
#define EXIT_BOOT_SERVICES_INDICATOR_VAR_ATTR   DXE_PHASE_INDICATOR_ATTR

typedef BOOLEAN   POLICY_LOCK_VAR;
#define WRTIE_ONCE_STATE_VAR_ATTR               (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)     // Volatile, BS and RT

#endif // _MU_VAR_POLICY_FOUNDATION_DXE_H_
