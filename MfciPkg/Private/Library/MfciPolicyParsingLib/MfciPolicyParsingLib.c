/** @file
  Implements the MFCI policy signature verification and field extraction

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>                       // CopyGuid()
#include <Library/MemoryAllocationLib.h>                 // Memory allocation and freeing
#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>                            // DEBUG tracing
#include <Library/BaseLib.h>                             // Safe String

#include "Library/MfciPolicyParsingLib.h"
#include "MfciPolicyParsingLibInternal.h"

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
  EFI_STATUS Status;

  DEBUG(( DEBUG_INFO, "%a()\n", __FUNCTION__ ));

  Status = ValidateSignature (SignedPolicy, SignedPolicySize, TrustAnchorCert, TrustAnchorCertSize, EKU);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "ValidateSignature() returned EFI_ERROR: %r\n", Status));
    goto _Exit;
  }

  // NOTE: the following extracts and validates the embedded policy payload without validating the signature
  Status = SanityCheckSignedPolicy (SignedPolicy, SignedPolicySize);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "SanityCheckSignedPolicy() returned EFI_ERROR: %r\n", Status));
    goto _Exit;
  }

_Exit:

  return Status;
}


EFI_STATUS
EFIAPI
ValidateSignature (
   IN CONST UINT8   *SignedPolicy,
      UINTN          SignedPolicySize,
   IN CONST UINT8   *TrustAnchorCert,
   IN UINTN          TrustAnchorCertSize,
   IN CONST CHAR8   *EKU
 )
{
  EFI_STATUS Status;
  UINT8 *Content = NULL;
  UINTN ContentSize = 0;

  DEBUG(( DEBUG_INFO, "%a()\n", __FUNCTION__ ));

  // Parameters Checking
  //

  if ( SignedPolicy == NULL || SignedPolicySize == 0 || TrustAnchorCert == NULL || TrustAnchorCertSize == 0 || EKU == NULL) {
    DEBUG ((DEBUG_ERROR, "SignedPolicy NULL or Size == 0, or TrustAnchorCert NULL or Size 0, or EKU NULL\n"));
    Status = EFI_INVALID_PARAMETER;
    goto _Exit;
  }

  DEBUG ((DEBUG_VERBOSE, "SignedPolicy: %p\n", SignedPolicy));
  DEBUG ((DEBUG_VERBOSE, "SignedPolicySize: %p\n", SignedPolicySize));

  if (TRUE != Pkcs7GetAttachedContent (SignedPolicy, SignedPolicySize, &Content, &ContentSize)) {
    DEBUG ((DEBUG_ERROR, "Pkcs7GetAttachedContent() returns FALSE\n"));
    Status = EFI_COMPROMISED_DATA;
    goto _Exit;
  }

  if ( ContentSize == 0 ) {
    DEBUG ((DEBUG_ERROR, "Pkcs7GetAttachedContent() returns ContentSize 0, no embedded content?\n"));
    Status = EFI_COMPROMISED_DATA;
    goto _Exit;
  }

  DEBUG ((DEBUG_VERBOSE, "Pkcs7GetAttachedContent() returns TRUE\n"));
  DEBUG ((DEBUG_VERBOSE, "Content:     0x%p\n", Content));
  DEBUG ((DEBUG_VERBOSE, "ContentSize: 0x%p\n", ContentSize));

  if (TRUE != Pkcs7Verify (SignedPolicy, SignedPolicySize, TrustAnchorCert, TrustAnchorCertSize, Content, ContentSize)) {
    DEBUG ((DEBUG_ERROR, "Pkcs7Verify() returns FALSE\n"));
    Status = EFI_SECURITY_VIOLATION;
    goto _Exit;
  }
  DEBUG ((DEBUG_VERBOSE, "Pkcs7Verify() returns TRUE\n"));

  Status = VerifyEKUsInPkcs7Signature(SignedPolicy,
                                      (UINT32)SignedPolicySize,
                                      &EKU,
                                      1,     // size of EKU[]
                                      TRUE);

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "VerifyEKUsInPkcs7Signature() returns error status: %r\n", Status));
    goto _Exit;
  }
  DEBUG ((DEBUG_VERBOSE, "VerifyEKUsInPkcs7Signature() returns SUCCESS\n"));

  Status = EFI_SUCCESS;

_Exit:
  if (Content!=NULL) {
    FreePool(Content);
    Content = NULL;
  }
  return Status;
}


EFI_STATUS
EFIAPI
SanityCheckSignedPolicy (
   IN  CONST VOID   *SignedPolicy,
       UINTN         SignedPolicySize
 )
{
  EFI_STATUS Status;
  MfciPolicyBlob *Policy = NULL;
  UINTN PolicySize = 0;

  DEBUG(( DEBUG_INFO, "%a()\n", __FUNCTION__ ));

  if ( SignedPolicy == NULL || SignedPolicySize == 0 ) {
    DEBUG ((DEBUG_ERROR, "SignedPolicy NULL or SignedPolicySize 0"));
    return EFI_INVALID_PARAMETER;
  }

  if (TRUE != Pkcs7GetAttachedContent (SignedPolicy, SignedPolicySize, &Policy, &PolicySize)) {
    DEBUG ((DEBUG_ERROR, "Pkcs7GetAttachedContent() returns FALSE\n"));
    Status = EFI_COMPROMISED_DATA;
    goto _Exit;
  }

  Status = SanityCheckPolicy(Policy, PolicySize);

_Exit:
  if (Policy!=NULL) {
    FreePool(Policy);
    Policy = NULL;
  }
  return Status;
}


EFI_STATUS
EFIAPI
SanityCheckPolicy (
   IN  CONST MfciPolicyBlob *Policy,
       UINTN               PolicySize
 )
{

  DEBUG(( DEBUG_INFO, "%a()\n", __FUNCTION__ ));

  if ( Policy == NULL || PolicySize == 0 ) {
    DEBUG ((DEBUG_ERROR, "Policy is NULL or size 0\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (PolicySize < POLICY_BLOB_MIN_SIZE || PolicySize > POLICY_BLOB_MAX_SIZE) {
    DEBUG ((DEBUG_ERROR, "Policy size is too small\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  if ( Policy->FormatVersion != POLICY_FORMAT_VERSION || Policy->PolicyVersion != POLICY_VERSION ) {
    DEBUG ((DEBUG_ERROR, "Format or Policy version are unexpected\n"));
    return EFI_COMPROMISED_DATA;
  }

  GUID PolicyPublisher;
  CopyMem(&PolicyPublisher, &Policy->PolicyPublisher, sizeof(GUID));
  if (!CompareGuid(&PolicyPublisher, &gPolicyPublisherGuid)) {
    DEBUG ((DEBUG_ERROR, "Policy Publisher GUID does NOT match\n"));
    DEBUG ((DEBUG_ERROR, "PolicyPublisher:      %g\n", &PolicyPublisher));
    DEBUG ((DEBUG_ERROR, "gPolicyPublisherGuid: %g\n", &gPolicyPublisherGuid));
    return EFI_COMPROMISED_DATA;
  }

  if (Policy->Reserved1Count != 0) {
    DEBUG ((DEBUG_ERROR, "Reserved1Count not 0\n"));
    return EFI_COMPROMISED_DATA;
  }

  if (Policy->OptionFlags != 0) {
    DEBUG ((DEBUG_ERROR, "OptionFlags not 0\n"));
    return EFI_COMPROMISED_DATA;
  }

  if (Policy->Reserved2Count != 0) {
    DEBUG ((DEBUG_ERROR, "Reserved2Count not 0\n"));
    return EFI_COMPROMISED_DATA;
  }

  UINT16 RulesCount = Policy->RulesCount;
  UINTN  RulesSize = RulesCount * sizeof(RULE);
  UINTN  ValueTableOffset = sizeof(MfciPolicyBlob) + RulesSize;

  if (ValueTableOffset > PolicySize) {
    DEBUG ((DEBUG_ERROR, "ValueTableOffset > PolicySize: %x > %x\n", ValueTableOffset, PolicySize));
    return EFI_COMPROMISED_DATA;
  }

  RULE  *Rules = (RULE*)((UINT8*)Policy + sizeof(MfciPolicyBlob));
  UINT8 *ValueTable = (UINT8*)Policy + ValueTableOffset;
  UINTN  ValueTableSize = PolicySize - ValueTableOffset;

  DEBUG ((DEBUG_VERBOSE, "Processing %d Rules\n", RulesCount));
  RULE *Rule;
  POLICY_STRING* PolicyString;
  for (UINT16 i=0; i < RulesCount; i++) {
    Rule = &Rules[i];
    DEBUG ((DEBUG_VERBOSE, "Rule #: %d  Rule* 0x%p\n", i, Rule));

    if (Rule->RootKey != UEFI_POLICIES_ROOT_KEY) {
      DEBUG ((DEBUG_ERROR, "Unsupported Root Key: 0x%04x\n", Rule->RootKey));
      return EFI_COMPROMISED_DATA;
    }

    if (Rule->OffsetToSubKeyName + sizeof(POLICY_STRING) > ValueTableSize) {
      DEBUG ((DEBUG_ERROR, "Offset to SubKey too large: 0x%04x\n", Rule->OffsetToSubKeyName));
      return EFI_COMPROMISED_DATA;
    }

    PolicyString = (POLICY_STRING*) (ValueTable + Rule->OffsetToSubKeyName);
    if (ValueTableOffset + Rule->OffsetToSubKeyName + PolicyString->StringLength > PolicySize ) {
      DEBUG ((DEBUG_ERROR, "SubKeyName string too long: 0x%04x\n", PolicyString->StringLength));
      return EFI_COMPROMISED_DATA;
    }

    if (Rule->OffsetToValueName + sizeof(POLICY_STRING) > ValueTableSize) {
      DEBUG ((DEBUG_ERROR, "Offset to ValueName too large: 0x%04x\n", Rule->OffsetToValueName));
      return EFI_COMPROMISED_DATA;
    }

    PolicyString = (POLICY_STRING*) (ValueTable + Rule->OffsetToValueName);
    if (ValueTableOffset + Rule->OffsetToValueName + PolicyString->StringLength > PolicySize ) {
      DEBUG ((DEBUG_ERROR, "ValueName string too long: 0x%04x\n", PolicyString->StringLength ));
      return EFI_COMPROMISED_DATA;
    }

    if (Rule->OffsetToValue + sizeof(POLICY_VALUE_HEADER) > ValueTableSize) {
      DEBUG ((DEBUG_ERROR, "Offset to Value too large: 0x%04x\n", Rule->OffsetToValue));
      return EFI_COMPROMISED_DATA;
    }

    POLICY_VALUE_HEADER* ValueHeader = (POLICY_VALUE_HEADER*) (ValueTable + Rule->OffsetToValue);
    POLICY_VALUE_TYPE ValueType = ValueHeader->Type;

    UINT32 ValueSize = CalculateSizeOfValueTableEntry(ValueHeader);
    if (ValueSize == 0) {
      DEBUG ((DEBUG_ERROR, "Policy Value Type 0x%04x not supported\n", ValueType));
      return EFI_COMPROMISED_DATA;
    }
    if (Rule->OffsetToValue + ValueSize > ValueTableSize) {
      DEBUG ((DEBUG_ERROR, "Value too large: %d\n", ValueSize));
      return EFI_COMPROMISED_DATA;
    }
  }

  return EFI_SUCCESS;
}


UINT32
CalculateSizeOfValueTableEntry(
    IN CONST POLICY_VALUE_HEADER *ValueHeader
  )
{
  UINT32 Size;
  CONST POLICY_VALUE_STRING *StringValue;

  switch (ValueHeader->Type)
  {
  case POLICY_VALUE_TYPE_STRING:
      StringValue = (CONST POLICY_VALUE_STRING*)ValueHeader;

      //
      // Length is in bytes but does not include the null-terminator.
      //
      Size = sizeof(POLICY_VALUE_STRING) + StringValue->String.StringLength + sizeof(CHAR16);
      break;

  case POLICY_VALUE_TYPE_QWORD:
      Size = sizeof(POLICY_VALUE_QWORD);
      break;

  default:
      Size = 0;
      break;
  }

  return Size;
}


VOID
SplitPolicyName (
    IN OUT CHAR16  *PolicyNameToSplit, // modified
    OUT    CHAR16 **SubKeyName,
    OUT    CHAR16 **ValueName
)
{
  CHAR16 *BeforeSep;
  CHAR16 *AfterSep;
  CONST CHAR16 Separator = POLICY_NAME_SEPARATOR;
  CONST CHAR16 W_NULL = L'\0';

  ASSERT(PolicyNameToSplit != NULL  &&  SubKeyName != NULL  && ValueName != NULL);

  // below adapted from SplitStr()
  BeforeSep = AfterSep = PolicyNameToSplit;

  while (*AfterSep != W_NULL) {
    if (*AfterSep == Separator) {
      break;
    }
    AfterSep++;
  }

  if (*AfterSep == Separator) {
    //
    // Find a sub-string, terminate it
    //
    *AfterSep = W_NULL;
    AfterSep++;
  }
  *SubKeyName = BeforeSep;
  *ValueName = AfterSep;  // NULL when not found
}

EFI_STATUS
EFIAPI
FindRule (
    IN  CONST  MfciPolicyBlob       *Policy,
    IN  CONST  CHAR16               *MfciPolicyName,
    OUT        POLICY_VALUE_HEADER **Value
 )
{
  UINT16 RulesCount = Policy->RulesCount;
  UINTN  RulesSize = RulesCount * sizeof(RULE);
  UINTN  ValueTableOffset = sizeof(MfciPolicyBlob) + RulesSize;
  CHAR16 LocalString[POLICY_STRING_MAX_LENGTH];
  CHAR16 *SubKeyExpected;
  CHAR16 *ValueNameExpected;

  if ( Policy == NULL || MfciPolicyName == NULL || Value == NULL) {
    DEBUG ((DEBUG_ERROR, "Policy is NULL, Name is NULL, or Value is NULL\n"));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_VERBOSE, "Searching for: '%s'\n", MfciPolicyName));

  StrCpyS(LocalString, sizeof(LocalString), MfciPolicyName);
  SplitPolicyName(LocalString, &SubKeyExpected, &ValueNameExpected);
  DEBUG ((DEBUG_VERBOSE, "Split SubKeyName '%s' & ValueName '%s'\n", SubKeyExpected, ValueNameExpected));

  RULE  *Rules = (RULE*)((CHAR8*)Policy + sizeof(MfciPolicyBlob));
  CHAR8 *ValueTable = (CHAR8*)Policy + ValueTableOffset;

  for (UINT16 i=0; i < RulesCount; i++) {
    RULE *Rule = &Rules[i];
    DEBUG ((DEBUG_VERBOSE, "Rule #: %d  Rule* 0x%p\n", i, Rule));

    if (Rule->RootKey != UEFI_POLICIES_ROOT_KEY) {
      DEBUG ((DEBUG_ERROR, "Incorrect Root Key found: %x\n", Rule->RootKey));
      continue;
    }

    POLICY_STRING* PolicyString = (POLICY_STRING*) (ValueTable + Rule->OffsetToSubKeyName);
    CONST CHAR16 *SubKeyName = PolicyString->String;
    CONST UINT16 SubKeyLength = PolicyString->StringLength / sizeof(CHAR16);
    DEBUG ((DEBUG_VERBOSE, "SubKeyLength and Name are %d and '%s'\n", SubKeyLength, SubKeyName));
    if (0 != StrnCmp(SubKeyExpected, SubKeyName, SubKeyLength)) {
      continue;
    }

    PolicyString = (POLICY_STRING*) (ValueTable + Rule->OffsetToValueName);
    CONST CHAR16 *ValueName = PolicyString->String;
    CONST UINT16 ValueLength = PolicyString->StringLength / sizeof(CHAR16);
    if (0 != StrnCmp(ValueNameExpected, ValueName, ValueLength)) {
      continue;
    }

    *Value = (POLICY_VALUE_HEADER*) (ValueTable + Rule->OffsetToValue);
    DEBUG ((DEBUG_VERBOSE, "Found: %p\n", *Value));
    return EFI_SUCCESS;
  }
  DEBUG ((DEBUG_ERROR, "Not Found\n"));
  return EFI_NOT_FOUND;
}


EFI_STATUS
EFIAPI
ExtractChar16 (
    IN  CONST  VOID         *SignedPolicy,
               UINTN         SignedPolicySize,
    IN  CONST  CHAR16       *MfciPolicyName,
    OUT        CHAR16      **MfciPolicyStringValue // allocated, caller must call FreePool()
 )
{
  EFI_STATUS Status;
  MfciPolicyBlob *Policy = NULL;
  UINTN PolicySize = 0;
  POLICY_VALUE_HEADER *PolicyValue = NULL;
  CHAR16 *TargetString = NULL;
  POLICY_STRING *PolicyString;

  DEBUG(( DEBUG_INFO, "%a()\n", __FUNCTION__ ));

  if ( SignedPolicy == NULL || SignedPolicySize == 0 || MfciPolicyName == NULL || MfciPolicyStringValue == NULL ) {
    DEBUG ((DEBUG_ERROR, "SignedPolicy NULL or SignedPolicySize 0, or other parameters NULL"));
    return EFI_INVALID_PARAMETER;
  }

  if (TRUE != Pkcs7GetAttachedContent (SignedPolicy, SignedPolicySize, &Policy, &PolicySize)) {
    DEBUG ((DEBUG_ERROR, "Pkcs7GetAttachedContent() returns FALSE\n"));
    Status = EFI_COMPROMISED_DATA;
    goto _Exit;
  }

  Status = FindRule(Policy, MfciPolicyName, &PolicyValue);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "FindRule returned EFI_ERROR: %r\n", Status));
    goto _Exit;
  }
  DEBUG ((DEBUG_VERBOSE, "PolicyValue 0x%p\n", PolicyValue));

  if (PolicyValue->Type != POLICY_VALUE_TYPE_STRING) {
    DEBUG ((DEBUG_ERROR, "Value Type not String, found: 0x%x\n", PolicyValue->Type));
    Status = EFI_COMPROMISED_DATA;
    goto _Exit;
  }

  PolicyString = &((POLICY_VALUE_STRING*)PolicyValue)->String;
  DEBUG ((DEBUG_VERBOSE, "PolicyString Length %x\n", PolicyString->StringLength));
  DEBUG ((DEBUG_VERBOSE, "PolicyString Value '%s'\n", PolicyString->String));

  TargetString = AllocatePool(PolicyString->StringLength + sizeof(CHAR16)); // Reserving space to add a NULL
  if (TargetString == NULL) {
    DEBUG ((DEBUG_ERROR, "AllocatePool Failed\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto _Exit;
  }

  CopyMem(TargetString, &PolicyString->String, PolicyString->StringLength);
  TargetString[PolicyString->StringLength / sizeof(CHAR16)] = L'\0';
  DEBUG ((DEBUG_VERBOSE, "TargetString '%s'\n", TargetString));
  *MfciPolicyStringValue = TargetString;
  TargetString = NULL;
  Status = EFI_SUCCESS;

_Exit:
  if (Policy!=NULL) {
    FreePool(Policy);
    Policy = NULL;
  }

  if (TargetString!=NULL) {
    FreePool(TargetString);
    TargetString = NULL;
  }

  return Status;
}


EFI_STATUS
EFIAPI
ExtractUint64 (
    IN   CONST  VOID         *SignedPolicy,
                UINTN         SignedPolicySize,
    IN   CONST  CHAR16       *MfciPolicyName,
    OUT         UINT64       *MfciPolicyU64Value  // caller should provide pointer to UINT64
 )
{
  EFI_STATUS Status;
  MfciPolicyBlob *Policy = NULL;
  UINTN PolicySize = 0;
  POLICY_VALUE_HEADER *PolicyValue = NULL;

  DEBUG(( DEBUG_INFO, "%a()\n", __FUNCTION__ ));

  if ( SignedPolicy == NULL || SignedPolicySize == 0 || MfciPolicyName == NULL || MfciPolicyU64Value == NULL ) {
    DEBUG ((DEBUG_ERROR, "SignedPolicy NULL or SignedPolicySize 0, PolicyName is NULL, or PolicyValue is NULL"));
    return EFI_INVALID_PARAMETER;
  }

  if (TRUE != Pkcs7GetAttachedContent (SignedPolicy, SignedPolicySize, &Policy, &PolicySize)) {
    DEBUG ((DEBUG_ERROR, "Pkcs7GetAttachedContent() returns FALSE\n"));
    Status = EFI_COMPROMISED_DATA;
    goto _Exit;
  }

  Status = FindRule(Policy, MfciPolicyName, &PolicyValue);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "FindRule returned EFI_ERROR: %r\n", Status));
    goto _Exit;
  }

  if (PolicyValue->Type != POLICY_VALUE_TYPE_QWORD) {
    DEBUG ((DEBUG_ERROR, "Value Type not QWORD, found: 0x%x\n", PolicyValue->Type));
    Status = EFI_COMPROMISED_DATA;
    goto _Exit;
  }

  *MfciPolicyU64Value = ((POLICY_VALUE_QWORD*)PolicyValue)->Value;
  Status = EFI_SUCCESS;

_Exit:
  if (Policy!=NULL) {
    FreePool(Policy);
    Policy = NULL;
  }
  return Status;
}
