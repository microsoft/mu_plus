/** @file
  Declares the interface to the MFCI Policy parsing library

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MFCI_POLICY_PARSING_LIB_H__
#define __MFCI_POLICY_PARSING_LIB_H__


EFI_STATUS
EFIAPI
ValidateBlob (
   IN CONST UINT8   *SignedPolicy,
      UINTN          SignedPolicySize,
   IN CONST UINT8   *TrustAnchorCert,
   IN UINTN          TrustAnchorCertSize,
   IN CONST CHAR8   *EKU
 );

/*
  MfciPolicyStringValue is allocated inside ExtractChar16() using AllocatePool().  It is callers responsability to free it.
  MfciPolicyStringValue is guaranteed to be CHAR16 NULL terminated.  The pool allocation may be larger than the string.
*/
EFI_STATUS
EFIAPI
ExtractChar16 (
    IN  CONST  VOID         *Policy,
               UINTN         PolicySize,
    IN  CONST  CHAR16       *MfciPolicyName,
    OUT        CHAR16      **MfciPolicyStringValue
 );


EFI_STATUS
EFIAPI
ExtractUint64 (
    IN   CONST  VOID         *Policy,
                UINTN         PolicySize,
    IN   CONST  CHAR16       *MfciPolicyName,
    OUT         UINT64       *MfciPolicyU64Value
 );

#endif //__MFCI_POLICY_PARSING_LIB_H__
