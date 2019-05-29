/** @file -- MuVariablePolicyHelperLib.c
This library contains helper functions for marshalling and registering
new policies with the VariablePolicy infrastructure.

This library is currently written against VariablePolicy revision 0x00010000.

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

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/VariablePolicy.h>

// TODO: UnitTests for this would be nice.


/**
  This helper function will allocate and populate a new VariablePolicy
  structure for a policy that does not contain any sub-structures (such as
  VARIABLE_LOCK_ON_VAR_STATE_POLICY).

  Caller will need to free structure once finished.

  @param[in]  Namespace   Pointer to an EFI_GUID for the namespace that this policy will protect.
  @param[in]  Namespace   Pointer to an EFI_GUID for the namespace that this policy will protect.

  @retval     EFI_SUCCESS             Operation completed successfully and structure is populated.
  @retval     EFI_OUT_OF_RESOURCES    Could not allocate sufficient space for structure.
  @retval     EFI_INVALID_PARAMETER   Namespace is NULL.
  @retval     EFI_INVALID_PARAMETER   LockPolicyType is invalid for a basic structure.

**/
EFI_STATUS
EFIAPI
CreateBasicVariablePolicy (
  IN  EFI_GUID                  *Namespace,
  IN  CHAR16                    *Name OPTIONAL,
  IN  UINT32                    MinSize,
  IN  UINT32                    MaxSize,
  IN  UINT32                    AttributesMustHave,
  IN  UINT32                    AttributesCantHave,
  IN  UINT8                     LockPolicyType,
  OUT VARIABLE_POLICY_ENTRY     **NewEntry
  )
{
    
} // CreateBasicVariablePolicy()
