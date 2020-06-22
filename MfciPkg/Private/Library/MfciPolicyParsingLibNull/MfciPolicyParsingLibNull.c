/** @file
  Implements the NULL library for the MFCI Policy parser

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/MfciPolicyParsingLib.h>

EFI_STATUS
EFIAPI
ValidateBlob (
   IN CONST UINT8   *SignedPolicy,
      UINTN          SignedPolicySize,
   IN CONST UINT8   *TrustAnchorCert,
   IN UINTN          TrustAnchorCertSize,
   IN CONST CHAR8   *EKU
 )
{
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
ExtractChar16 (
    IN  CONST  VOID         *blob,
               UINTN         blobSize,
    IN  CONST  CHAR16       *MfciPolicyName,
    OUT        CHAR16      **MfciPolicyStringValue
 )
{
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
ExtractUint64 (
    IN   CONST  VOID         *blob,
                UINTN         blobSize,
    IN   CONST  CHAR16       *MfciPolicyName,
    OUT         UINT64       *MfciPolicyU64Value
 )
{
  return EFI_UNSUPPORTED;
}
