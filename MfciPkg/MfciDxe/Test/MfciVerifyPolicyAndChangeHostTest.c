/** @file
  This module tests MFCI blob verification and apply
  logic for MfciDxe driver.

  Note: This module does NOT test signature validation
  step, which is covered by MfciPolicyParsingUnitTest.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>

#include <MfciPolicyType.h>
#include <MfciVariables.h>
#include <MfciPolicyFields.h>
#include <Guid/MuVarPolicyFoundationDxe.h>
#include <Protocol/MfciProtocol.h>
#include <Library/MfciDeviceIdSupportLib.h>
#include <Library/MfciPolicyParsingLib.h>
#include <Library/MfciRetrievePolicyLib.h>
#include <Library/BaseCryptLib.h>

#include <Library/BaseLib.h>                             // CpuDeadLoop()
#include <Library/DebugLib.h>                            // DEBUG tracing
#include <Library/BaseMemoryLib.h>                       // CopyGuid()
#include <Library/VariablePolicyHelperLib.h>             // NotifyMfciPolicyChange()
#include <Library/MemoryAllocationLib.h>

#include <Library/UnitTestLib.h>
#include <UnitTests/MfciPolicyParsingUnitTest/data/packets/policy_good_manufacturing.bin.h>
#include <UnitTests/MfciPolicyParsingUnitTest/data/packets/policy_good_manufacturing.bin.p7.h>
#include <UnitTests/MfciPolicyParsingUnitTest/data/packets/policy_target_manufacturing.bin.h>
#include <UnitTests/MfciPolicyParsingUnitTest/data/packets/policy_target_manufacturing.bin.p7.h>

#include "../MfciDxe.h"

#define UNIT_TEST_NAME     "Mfci Verify Policy And Change Host Test"
#define UNIT_TEST_VERSION  "0.1"

// Simply use policy_good_manufacturing based on UnitTests/data/packets/GenPacket.py
#define MFCI_TEST_MANUFACTURER  L"Contoso Computers, LLC"
#define MFCI_TEST_PRODUCT       L"Laptop Foo"
#define MFCI_TEST_SERIAL_NUM    L"F0013-000243546-X02"
#define MFCI_TEST_OEM_01        L"ODM Foo"
#define MFCI_TEST_OEM_02        L""
#define MFCI_TEST_NONCE         0x0123456789abcdef
#define MFCI_TEST_POLICY        (STD_ACTION_SECURE_BOOT_CLEAR | STD_ACTION_TPM_CLEAR)

#define MFCI_TEST_NONCE_TARGET   0xBA5EBA11FEEDF00D
#define MFCI_TEST_TARGET_BIT     BIT16
#define MFCI_TEST_POLICY_TARGET  (STD_ACTION_SECURE_BOOT_CLEAR | STD_ACTION_TPM_CLEAR | MFCI_TEST_TARGET_BIT)

/**
A mocked version of GetVariable.

@retval EFI_NOT_READY                 If requested service is not yet available
@retval Others                        See EFI_GET_VARIABLE for more details

**/
EFI_STATUS
EFIAPI
UnitTestGetVariable (
  IN     CHAR16 *VariableName,
  IN     EFI_GUID *VendorGuid,
  OUT    UINT32 *Attributes, OPTIONAL
  IN OUT UINTN                       *DataSize,
  OUT    VOID                        *Data           OPTIONAL
  );

EFI_STATUS
EFIAPI
UnitTestSetVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
  );

EFI_RUNTIME_SERVICES  mMockRuntime = {
  .GetVariable = UnitTestGetVariable,
  .SetVariable = UnitTestSetVariable,
};

extern BOOLEAN  mVarPolicyRegistered;

typedef struct {
  UINT64              Nonce;
  MFCI_POLICY_TYPE    PolicyValue;
  CONST VOID          *Policy;
  UINTN               PolicySize;
  CONST VOID          *PolicyContent;
  UINTN               PolicyContentSize;
} MFCI_UT_POLICY_INFO;

typedef struct {
  MFCI_UT_POLICY_INFO    CurrentPolicy;
  MFCI_UT_POLICY_INFO    NextPolicy;
} MFCI_UT_VERIFY_CONTEXT;

MFCI_UT_VERIFY_CONTEXT  *mCurrentMfciVerify;

MFCI_UT_VERIFY_CONTEXT  mMfciVerifyContext01 = {
  .CurrentPolicy       = {
    .Nonce             = MFCI_TEST_NONCE,
    .PolicyValue       = MFCI_TEST_POLICY,
    .Policy            = mSigned_policy_good_manufacturing,
    .PolicySize        = sizeof (mSigned_policy_good_manufacturing),
    .PolicyContent     = mBin_policy_good_manufacturing,
    .PolicyContentSize = sizeof (mBin_policy_good_manufacturing),
  },
  .NextPolicy          = {
    .Nonce             = MFCI_TEST_NONCE_TARGET,
    .PolicyValue       = MFCI_TEST_POLICY_TARGET,
    .Policy            = mSigned_policy_target_manufacturing,
    .PolicySize        = sizeof (mSigned_policy_target_manufacturing),
    .PolicyContent     = mPolicy_target_manufacturing,
    .PolicyContentSize = sizeof (mPolicy_target_manufacturing),
  }
};

MFCI_UT_VERIFY_CONTEXT  mMfciVerifyContext02 = {
  .NextPolicy          = {
    .Nonce             = MFCI_TEST_NONCE_TARGET,
    .PolicyValue       = MFCI_TEST_POLICY_TARGET,
    .Policy            = mSigned_policy_target_manufacturing,
    .PolicySize        = sizeof (mSigned_policy_target_manufacturing),
    .PolicyContent     = mPolicy_target_manufacturing,
    .PolicyContentSize = sizeof (mPolicy_target_manufacturing),
  }
};

MFCI_UT_VERIFY_CONTEXT  mMfciVerifyContext03 = {
  .CurrentPolicy       = {
    .Nonce             = MFCI_TEST_NONCE,
    .PolicyValue       = MFCI_TEST_POLICY,
    .Policy            = mBin_policy_good_manufacturing,
    .PolicySize        = sizeof (mBin_policy_good_manufacturing),
    .PolicyContent     = NULL,
    .PolicyContentSize = 0,
  },
  .NextPolicy          = {
    .Nonce             = MFCI_TEST_NONCE_TARGET,
  }
};

MFCI_UT_VERIFY_CONTEXT  mMfciVerifyContext04 = {
  .NextPolicy    = {
    .Nonce       = MFCI_TEST_NONCE_TARGET,
    .PolicyValue = MFCI_TEST_POLICY_TARGET,
    .Policy      = mBin_policy_good_manufacturing,
    .PolicySize  = sizeof (mBin_policy_good_manufacturing),
  }
};

MFCI_UT_VERIFY_CONTEXT  mMfciVerifyContext05 = {
  .NextPolicy = {
    .Nonce    = MFCI_TEST_NONCE_TARGET,
  }
};

/**
A mocked version of GetVariable.

@retval EFI_NOT_READY                 If requested service is not yet available
@retval Others                        See EFI_GET_VARIABLE for more details

**/
EFI_STATUS
EFIAPI
UnitTestGetVariable (
  IN     CHAR16 *VariableName,
  IN     EFI_GUID *VendorGuid,
  OUT    UINT32 *Attributes, OPTIONAL
  IN OUT UINTN                       *DataSize,
  OUT    VOID                        *Data           OPTIONAL
  )
{
  EFI_STATUS  ReturnStatus = EFI_SUCCESS;
  CONST VOID  *ReturnVal   = NULL;
  UINT64      ReturnNumber = 0;
  UINTN       Size         = 0;
  BOOLEAN     IsFound;

  DEBUG ((DEBUG_INFO, "%a: %s\n", __FUNCTION__, VariableName));
  if (StrCmp (VariableName, gPolicyBlobFieldName[MFCI_POLICY_TARGET_MANUFACTURER]) == 0) {
    ReturnVal = MFCI_TEST_MANUFACTURER;
    Size      = sizeof (MFCI_TEST_MANUFACTURER);
  } else if (StrCmp (VariableName, gPolicyBlobFieldName[MFCI_POLICY_TARGET_PRODUCT]) == 0) {
    ReturnVal = MFCI_TEST_PRODUCT;
    Size      = sizeof (MFCI_TEST_PRODUCT);
  } else if (StrCmp (VariableName, gPolicyBlobFieldName[MFCI_POLICY_TARGET_SERIAL_NUMBER]) == 0) {
    ReturnVal = MFCI_TEST_SERIAL_NUM;
    Size      = sizeof (MFCI_TEST_SERIAL_NUM);
  } else if (StrCmp (VariableName, gPolicyBlobFieldName[MFCI_POLICY_TARGET_OEM_01]) == 0) {
    ReturnVal = MFCI_TEST_OEM_01;
    Size      = sizeof (MFCI_TEST_OEM_01);
  } else if (StrCmp (VariableName, gPolicyBlobFieldName[MFCI_POLICY_TARGET_OEM_02]) == 0) {
    ReturnVal = MFCI_TEST_OEM_02;
    Size      = sizeof (MFCI_TEST_OEM_02);
  } else if (StrCmp (VariableName, gPolicyBlobFieldName[MFCI_POLICY_TARGET_NONCE]) == 0) {
    ReturnNumber = MFCI_TEST_NONCE;
    ReturnVal    = &ReturnNumber;
    Size         = sizeof (UINT64);
  } else {
    check_expected (VariableName);
    IsFound = (BOOLEAN)mock ();
    if (!IsFound) {
      return EFI_NOT_FOUND;
    }

    ReturnVal   = (VOID *)mock ();
    Size        = (UINTN)mock ();
    *Attributes = (UINT32)MFCI_POLICY_VARIABLE_ATTR;
  }

  if (Size > *DataSize) {
    ReturnStatus = EFI_BUFFER_TOO_SMALL;
    *DataSize    = Size;
  }

  if (!EFI_ERROR (ReturnStatus)) {
    CopyMem (Data, ReturnVal, *DataSize);
    *DataSize = Size;
  }

  return ReturnStatus;
}

EFI_STATUS
EFIAPI
UnitTestSetVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
  )
{
  DEBUG ((DEBUG_INFO, "%a: %s\n", __FUNCTION__, VariableName));

  check_expected (VariableName);
  check_expected (Data);
  check_expected (DataSize);
  return mock ();
}

BOOLEAN
EFIAPI
GetRandomNumber64 (
  OUT     UINT64  *Rand
  )
{
  if (Rand == NULL) {
    return FALSE;
  }

  *Rand = 0;
  return TRUE;
}

// Initializer for the SecureBoot Callback
EFI_STATUS
EFIAPI
InitSecureBootListener (
  VOID
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InitTpmListener (
  VOID
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
NotifyMfciPolicyChange (
  IN MFCI_POLICY_TYPE  NewPolicy
  )
{
  check_expected (NewPolicy);
  return (EFI_STATUS)mock ();
}

EFI_STATUS
EFIAPI
InitPublicInterface (
  VOID
  )
{
  return EFI_SUCCESS;
}

UNIT_TEST_STATUS
EFIAPI
VerifyPrerequisite (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  mCurrentMfciVerify   = Context;
  mVarPolicyRegistered = TRUE;
  return UNIT_TEST_PASSED;
}

VOID
EFIAPI
VerifyPolicyAndChange (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

// Verified on a normal path from one policy to the next
UNIT_TEST_STATUS
EFIAPI
UnitTestVerifyAndChangeNormal (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN                     Index;
  BASE_LIBRARY_JUMP_BUFFER  JumpBuf;

  expect_any_always (UnitTestSetVariable, VariableName);
  expect_any_always (UnitTestSetVariable, Data);
  expect_any_always (UnitTestSetVariable, DataSize);
  will_return_always (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestGetVariable, VariableName, NEXT_MFCI_NONCE_VARIABLE_NAME, sizeof (NEXT_MFCI_NONCE_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, &mCurrentMfciVerify->NextPolicy.Nonce);
  will_return (UnitTestGetVariable, sizeof (mCurrentMfciVerify->NextPolicy.Nonce));

  expect_memory (UnitTestGetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, NULL);
  will_return (UnitTestGetVariable, mCurrentMfciVerify->CurrentPolicy.PolicySize);

  expect_memory (UnitTestGetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, mCurrentMfciVerify->CurrentPolicy.Policy);
  will_return (UnitTestGetVariable, mCurrentMfciVerify->CurrentPolicy.PolicySize);

  expect_memory (UnitTestGetVariable, VariableName, CURRENT_MFCI_NONCE_VARIABLE_NAME, sizeof (CURRENT_MFCI_NONCE_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, &mCurrentMfciVerify->CurrentPolicy.Nonce);
  will_return (UnitTestGetVariable, sizeof (mCurrentMfciVerify->CurrentPolicy.Nonce));

  expect_memory (Pkcs7Verify, P7Data, mCurrentMfciVerify->CurrentPolicy.Policy, mCurrentMfciVerify->CurrentPolicy.PolicySize);
  expect_value (Pkcs7Verify, P7Length, mCurrentMfciVerify->CurrentPolicy.PolicySize);
  will_return (Pkcs7Verify, TRUE);

  expect_memory (VerifyEKUsInPkcs7Signature, Pkcs7Signature, mCurrentMfciVerify->CurrentPolicy.Policy, mCurrentMfciVerify->CurrentPolicy.PolicySize);
  expect_value (VerifyEKUsInPkcs7Signature, SignatureSize, mCurrentMfciVerify->CurrentPolicy.PolicySize);
  will_return (VerifyEKUsInPkcs7Signature, TRUE);

  expect_memory_count (Pkcs7GetAttachedContent, P7Data, mCurrentMfciVerify->CurrentPolicy.Policy, mCurrentMfciVerify->CurrentPolicy.PolicySize, MFCI_POLICY_FIELD_COUNT + 2);
  expect_value_count (Pkcs7GetAttachedContent, P7Length, mCurrentMfciVerify->CurrentPolicy.PolicySize, MFCI_POLICY_FIELD_COUNT + 2);

  for (Index = 0; Index < MFCI_POLICY_FIELD_COUNT + 2; Index++) {
    will_return (Pkcs7GetAttachedContent, mCurrentMfciVerify->CurrentPolicy.PolicyContent);
    will_return (Pkcs7GetAttachedContent, mCurrentMfciVerify->CurrentPolicy.PolicyContentSize);
  }

  expect_memory (UnitTestGetVariable, VariableName, NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, NULL);
  will_return (UnitTestGetVariable, mCurrentMfciVerify->NextPolicy.PolicySize);

  expect_memory (UnitTestGetVariable, VariableName, NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, mCurrentMfciVerify->NextPolicy.Policy);
  will_return (UnitTestGetVariable, mCurrentMfciVerify->NextPolicy.PolicySize);

  expect_memory (Pkcs7Verify, P7Data, mCurrentMfciVerify->NextPolicy.Policy, mCurrentMfciVerify->NextPolicy.PolicySize);
  expect_value (Pkcs7Verify, P7Length, mCurrentMfciVerify->NextPolicy.PolicySize);
  will_return (Pkcs7Verify, TRUE);

  expect_memory (VerifyEKUsInPkcs7Signature, Pkcs7Signature, mCurrentMfciVerify->NextPolicy.Policy, mCurrentMfciVerify->NextPolicy.PolicySize);
  expect_value (VerifyEKUsInPkcs7Signature, SignatureSize, mCurrentMfciVerify->NextPolicy.PolicySize);
  will_return (VerifyEKUsInPkcs7Signature, TRUE);

  expect_memory_count (Pkcs7GetAttachedContent, P7Data, mCurrentMfciVerify->NextPolicy.Policy, mCurrentMfciVerify->NextPolicy.PolicySize, MFCI_POLICY_FIELD_COUNT + 2);
  expect_value_count (Pkcs7GetAttachedContent, P7Length, mCurrentMfciVerify->NextPolicy.PolicySize, MFCI_POLICY_FIELD_COUNT + 2);

  for (Index = 0; Index < MFCI_POLICY_FIELD_COUNT + 2; Index++) {
    will_return (Pkcs7GetAttachedContent, mCurrentMfciVerify->NextPolicy.PolicyContent);
    will_return (Pkcs7GetAttachedContent, mCurrentMfciVerify->NextPolicy.PolicyContentSize);
  }

  expect_value (NotifyMfciPolicyChange, NewPolicy, mCurrentMfciVerify->NextPolicy.PolicyValue);
  will_return (NotifyMfciPolicyChange, EFI_SUCCESS);

  expect_value (ResetSystemWithSubtype, ResetType, EfiResetCold);
  expect_value (ResetSystemWithSubtype, ResetSubtype, &gMfciPolicyChangeResetGuid);
  will_return (ResetSystemWithSubtype, &JumpBuf);

  if (!SetJump (&JumpBuf)) {
    VerifyPolicyAndChange (NULL, NULL);
  }

  return UNIT_TEST_PASSED;
}

// Verified on a normal path from no policy to the new policy
UNIT_TEST_STATUS
EFIAPI
UnitTestVerifyAndChangeEmptyCurrent (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN                     Index;
  BASE_LIBRARY_JUMP_BUFFER  JumpBuf;

  expect_any_always (UnitTestSetVariable, VariableName);
  expect_any_always (UnitTestSetVariable, Data);
  expect_any_always (UnitTestSetVariable, DataSize);
  will_return_always (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestGetVariable, VariableName, NEXT_MFCI_NONCE_VARIABLE_NAME, sizeof (NEXT_MFCI_NONCE_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, &mCurrentMfciVerify->NextPolicy.Nonce);
  will_return (UnitTestGetVariable, sizeof (mCurrentMfciVerify->NextPolicy.Nonce));

  expect_memory (UnitTestGetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, FALSE);

  expect_memory (UnitTestGetVariable, VariableName, NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, NULL);
  will_return (UnitTestGetVariable, mCurrentMfciVerify->NextPolicy.PolicySize);

  expect_memory (UnitTestGetVariable, VariableName, NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, mCurrentMfciVerify->NextPolicy.Policy);
  will_return (UnitTestGetVariable, mCurrentMfciVerify->NextPolicy.PolicySize);

  expect_memory (Pkcs7Verify, P7Data, mCurrentMfciVerify->NextPolicy.Policy, mCurrentMfciVerify->NextPolicy.PolicySize);
  expect_value (Pkcs7Verify, P7Length, mCurrentMfciVerify->NextPolicy.PolicySize);
  will_return (Pkcs7Verify, TRUE);

  expect_memory (VerifyEKUsInPkcs7Signature, Pkcs7Signature, mCurrentMfciVerify->NextPolicy.Policy, mCurrentMfciVerify->NextPolicy.PolicySize);
  expect_value (VerifyEKUsInPkcs7Signature, SignatureSize, mCurrentMfciVerify->NextPolicy.PolicySize);
  will_return (VerifyEKUsInPkcs7Signature, TRUE);

  expect_memory_count (Pkcs7GetAttachedContent, P7Data, mCurrentMfciVerify->NextPolicy.Policy, mCurrentMfciVerify->NextPolicy.PolicySize, MFCI_POLICY_FIELD_COUNT + 2);
  expect_value_count (Pkcs7GetAttachedContent, P7Length, mCurrentMfciVerify->NextPolicy.PolicySize, MFCI_POLICY_FIELD_COUNT + 2);

  for (Index = 0; Index < MFCI_POLICY_FIELD_COUNT + 2; Index++) {
    will_return (Pkcs7GetAttachedContent, mCurrentMfciVerify->NextPolicy.PolicyContent);
    will_return (Pkcs7GetAttachedContent, mCurrentMfciVerify->NextPolicy.PolicyContentSize);
  }

  expect_value (NotifyMfciPolicyChange, NewPolicy, mCurrentMfciVerify->NextPolicy.PolicyValue);
  will_return (NotifyMfciPolicyChange, EFI_SUCCESS);

  expect_value (ResetSystemWithSubtype, ResetType, EfiResetCold);
  expect_value (ResetSystemWithSubtype, ResetSubtype, &gMfciPolicyChangeResetGuid);
  will_return (ResetSystemWithSubtype, &JumpBuf);

  if (!SetJump (&JumpBuf)) {
    VerifyPolicyAndChange (NULL, NULL);
  }

  return UNIT_TEST_PASSED;
}

// Verified on a initial path to create new nonce
UNIT_TEST_STATUS
EFIAPI
UnitTestVerifyAndChangeCreateNextNonce (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT64            Nonce   = 0;
  MFCI_POLICY_TYPE  Policy  = CUSTOMER_STATE;
  POLICY_LOCK_VAR   LockVar = MFCI_LOCK_VAR_VALUE;

  expect_memory (UnitTestGetVariable, VariableName, NEXT_MFCI_NONCE_VARIABLE_NAME, sizeof (NEXT_MFCI_NONCE_VARIABLE_NAME));
  will_return (UnitTestGetVariable, FALSE);

  expect_memory (UnitTestSetVariable, VariableName, NEXT_MFCI_NONCE_VARIABLE_NAME, sizeof (NEXT_MFCI_NONCE_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, sizeof (UINT64));
  expect_memory (UnitTestSetVariable, Data, &Nonce, sizeof (UINT64));
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestGetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, FALSE);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_NONCE_VARIABLE_NAME, sizeof (CURRENT_MFCI_NONCE_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, sizeof (UINT64));
  expect_memory (UnitTestSetVariable, Data, &Nonce, sizeof (UINT64));
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, 0);
  expect_value (UnitTestSetVariable, Data, NULL);
  will_return (UnitTestSetVariable, EFI_NOT_FOUND);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, sizeof (Policy));
  expect_memory (UnitTestSetVariable, Data, &Policy, sizeof (Policy));
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestGetVariable, VariableName, NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, FALSE);

  expect_memory (UnitTestSetVariable, VariableName, MFCI_LOCK_VAR_NAME, sizeof (MFCI_LOCK_VAR_NAME));
  expect_value (UnitTestSetVariable, DataSize, sizeof (POLICY_LOCK_VAR));
  expect_memory (UnitTestSetVariable, Data, &LockVar, sizeof (POLICY_LOCK_VAR));
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  VerifyPolicyAndChange (NULL, NULL);

  return UNIT_TEST_PASSED;
}

// Verified on a initial path to create new nonce
UNIT_TEST_STATUS
EFIAPI
UnitTestVerifyAndChangeCleanCurrentOnFailure (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT64            Nonce  = 0;
  MFCI_POLICY_TYPE  Policy = CUSTOMER_STATE;

  expect_memory (UnitTestGetVariable, VariableName, NEXT_MFCI_NONCE_VARIABLE_NAME, sizeof (NEXT_MFCI_NONCE_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, &mCurrentMfciVerify->NextPolicy.Nonce);
  will_return (UnitTestGetVariable, sizeof (mCurrentMfciVerify->NextPolicy.Nonce));

  expect_memory (UnitTestGetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, FALSE);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_NONCE_VARIABLE_NAME, sizeof (CURRENT_MFCI_NONCE_VARIABLE_NAME));
  expect_any (UnitTestSetVariable, DataSize);
  expect_any (UnitTestSetVariable, Data);
  will_return (UnitTestSetVariable, EFI_WRITE_PROTECTED);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  expect_any (UnitTestSetVariable, DataSize);
  expect_any (UnitTestSetVariable, Data);
  will_return (UnitTestSetVariable, EFI_NOT_FOUND);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, sizeof (Policy));
  expect_memory (UnitTestSetVariable, Data, &Policy, sizeof (Policy));
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, MFCI_LOCK_VAR_NAME, sizeof (MFCI_LOCK_VAR_NAME));
  expect_any (UnitTestSetVariable, DataSize);
  expect_any (UnitTestSetVariable, Data);
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_NONCE_VARIABLE_NAME, sizeof (CURRENT_MFCI_NONCE_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, sizeof (UINT64));
  expect_memory (UnitTestSetVariable, Data, &Nonce, sizeof (UINT64));
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, 0);
  expect_value (UnitTestSetVariable, Data, NULL);
  will_return (UnitTestSetVariable, EFI_NOT_FOUND);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, sizeof (Policy));
  expect_memory (UnitTestSetVariable, Data, &Policy, sizeof (Policy));
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  VerifyPolicyAndChange (NULL, NULL);

  return UNIT_TEST_PASSED;
}

// Verified on a initial path to create new nonce
UNIT_TEST_STATUS
EFIAPI
UnitTestVerifyAndChangeCleanContinueOnFailure (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT64            Nonce  = 0;
  MFCI_POLICY_TYPE  Policy = CUSTOMER_STATE;

  expect_memory (UnitTestGetVariable, VariableName, NEXT_MFCI_NONCE_VARIABLE_NAME, sizeof (NEXT_MFCI_NONCE_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, &mCurrentMfciVerify->NextPolicy.Nonce);
  will_return (UnitTestGetVariable, sizeof (mCurrentMfciVerify->NextPolicy.Nonce));

  expect_memory (UnitTestGetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, FALSE);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_NONCE_VARIABLE_NAME, sizeof (CURRENT_MFCI_NONCE_VARIABLE_NAME));
  expect_any (UnitTestSetVariable, DataSize);
  expect_any (UnitTestSetVariable, Data);
  will_return (UnitTestSetVariable, EFI_WRITE_PROTECTED);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  expect_any (UnitTestSetVariable, DataSize);
  expect_any (UnitTestSetVariable, Data);
  will_return (UnitTestSetVariable, EFI_NOT_FOUND);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, sizeof (Policy));
  expect_memory (UnitTestSetVariable, Data, &Policy, sizeof (Policy));
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, MFCI_LOCK_VAR_NAME, sizeof (MFCI_LOCK_VAR_NAME));
  expect_any (UnitTestSetVariable, DataSize);
  expect_any (UnitTestSetVariable, Data);
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_NONCE_VARIABLE_NAME, sizeof (CURRENT_MFCI_NONCE_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, sizeof (UINT64));
  expect_memory (UnitTestSetVariable, Data, &Nonce, sizeof (UINT64));
  will_return (UnitTestSetVariable, EFI_WRITE_PROTECTED);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, 0);
  expect_value (UnitTestSetVariable, Data, NULL);
  will_return (UnitTestSetVariable, EFI_NOT_FOUND);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, sizeof (Policy));
  expect_memory (UnitTestSetVariable, Data, &Policy, sizeof (Policy));
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  VerifyPolicyAndChange (NULL, NULL);

  return UNIT_TEST_PASSED;
}

// Verified incorrect current blob should not exist
UNIT_TEST_STATUS
EFIAPI
UnitTestVerifyAndChangePurgeWrongCurrent (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  expect_memory (UnitTestGetVariable, VariableName, NEXT_MFCI_NONCE_VARIABLE_NAME, sizeof (NEXT_MFCI_NONCE_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, &mCurrentMfciVerify->NextPolicy.Nonce);
  will_return (UnitTestGetVariable, sizeof (mCurrentMfciVerify->NextPolicy.Nonce));

  expect_memory (UnitTestGetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, NULL);
  will_return (UnitTestGetVariable, mCurrentMfciVerify->CurrentPolicy.PolicySize);

  expect_memory (UnitTestGetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, mCurrentMfciVerify->CurrentPolicy.Policy);
  will_return (UnitTestGetVariable, mCurrentMfciVerify->CurrentPolicy.PolicySize);

  expect_memory (UnitTestGetVariable, VariableName, CURRENT_MFCI_NONCE_VARIABLE_NAME, sizeof (CURRENT_MFCI_NONCE_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, &mCurrentMfciVerify->CurrentPolicy.Nonce);
  will_return (UnitTestGetVariable, sizeof (mCurrentMfciVerify->CurrentPolicy.Nonce));

  expect_memory (Pkcs7GetAttachedContent, P7Data, mCurrentMfciVerify->CurrentPolicy.Policy, mCurrentMfciVerify->CurrentPolicy.PolicySize);
  expect_value (Pkcs7GetAttachedContent, P7Length, mCurrentMfciVerify->CurrentPolicy.PolicySize);
  will_return (Pkcs7GetAttachedContent, mCurrentMfciVerify->CurrentPolicy.PolicyContent);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_NONCE_VARIABLE_NAME, sizeof (CURRENT_MFCI_NONCE_VARIABLE_NAME));
  expect_any (UnitTestSetVariable, DataSize);
  expect_any (UnitTestSetVariable, Data);
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, 0);
  expect_value (UnitTestSetVariable, Data, NULL);
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_VARIABLE_NAME));
  expect_any (UnitTestSetVariable, DataSize);
  expect_any (UnitTestSetVariable, Data);
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestGetVariable, VariableName, NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, FALSE);

  expect_memory (UnitTestSetVariable, VariableName, MFCI_LOCK_VAR_NAME, sizeof (MFCI_LOCK_VAR_NAME));
  expect_any (UnitTestSetVariable, DataSize);
  expect_any (UnitTestSetVariable, Data);
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  VerifyPolicyAndChange (NULL, NULL);

  return UNIT_TEST_PASSED;
}

// Verified incorrect current blob should not exist
UNIT_TEST_STATUS
EFIAPI
UnitTestVerifyAndChangePurgeWrongTarget (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN  Nonce = 0;

  expect_memory (UnitTestGetVariable, VariableName, NEXT_MFCI_NONCE_VARIABLE_NAME, sizeof (NEXT_MFCI_NONCE_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, &mCurrentMfciVerify->NextPolicy.Nonce);
  will_return (UnitTestGetVariable, sizeof (mCurrentMfciVerify->NextPolicy.Nonce));

  expect_memory (UnitTestGetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, FALSE);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_NONCE_VARIABLE_NAME, sizeof (CURRENT_MFCI_NONCE_VARIABLE_NAME));
  expect_any (UnitTestSetVariable, DataSize);
  expect_any (UnitTestSetVariable, Data);
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, 0);
  expect_value (UnitTestSetVariable, Data, NULL);
  will_return (UnitTestSetVariable, EFI_NOT_FOUND);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_VARIABLE_NAME));
  expect_any (UnitTestSetVariable, DataSize);
  expect_any (UnitTestSetVariable, Data);
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestGetVariable, VariableName, NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, NULL);
  will_return (UnitTestGetVariable, mCurrentMfciVerify->NextPolicy.PolicySize);

  expect_memory (UnitTestGetVariable, VariableName, NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, mCurrentMfciVerify->NextPolicy.Policy);
  will_return (UnitTestGetVariable, mCurrentMfciVerify->NextPolicy.PolicySize);

  expect_memory (Pkcs7GetAttachedContent, P7Data, mCurrentMfciVerify->NextPolicy.Policy, mCurrentMfciVerify->NextPolicy.PolicySize);
  expect_value (Pkcs7GetAttachedContent, P7Length, mCurrentMfciVerify->NextPolicy.PolicySize);
  will_return (Pkcs7GetAttachedContent, mCurrentMfciVerify->NextPolicy.PolicyContent);

  expect_memory (UnitTestSetVariable, VariableName, NEXT_MFCI_NONCE_VARIABLE_NAME, sizeof (NEXT_MFCI_NONCE_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, sizeof (UINT64));
  expect_memory (UnitTestSetVariable, Data, &Nonce, sizeof (UINT64));
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (NEXT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  expect_value (UnitTestSetVariable, DataSize, 0);
  expect_value (UnitTestSetVariable, Data, NULL);
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_NONCE_VARIABLE_NAME, sizeof (CURRENT_MFCI_NONCE_VARIABLE_NAME));
  expect_any (UnitTestSetVariable, DataSize);
  expect_any (UnitTestSetVariable, Data);
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_BLOB_VARIABLE_NAME));
  expect_any (UnitTestSetVariable, DataSize);
  expect_any (UnitTestSetVariable, Data);
  will_return (UnitTestSetVariable, EFI_NOT_FOUND);

  expect_memory (UnitTestSetVariable, VariableName, CURRENT_MFCI_POLICY_VARIABLE_NAME, sizeof (CURRENT_MFCI_POLICY_VARIABLE_NAME));
  expect_any (UnitTestSetVariable, DataSize);
  expect_any (UnitTestSetVariable, Data);
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  expect_memory (UnitTestSetVariable, VariableName, MFCI_LOCK_VAR_NAME, sizeof (MFCI_LOCK_VAR_NAME));
  expect_any (UnitTestSetVariable, DataSize);
  expect_any (UnitTestSetVariable, Data);
  will_return (UnitTestSetVariable, EFI_SUCCESS);

  VerifyPolicyAndChange (NULL, NULL);

  return UNIT_TEST_PASSED;
}

/**
  Initialize the unit test framework, suite, and unit tests for the
  sample unit tests and run the unit tests.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
EFI_STATUS
EFIAPI
UefiTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      VerifyAndChangePhaseSuite;

  Framework = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  // The blob parsing part is tested in MfciPolicyParsingUnitTest, so will not go through those here.

  //
  // Populate the VerifyAndChangePhaseSuite Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&VerifyAndChangePhaseSuite, Framework, "VerifyAndChangePhase", "ReportRouter.Phase", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for VerifyAndChangePhaseSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (VerifyAndChangePhaseSuite, "VerifyAndChange should succeed with correct target information", "VerifyPerfect", UnitTestVerifyAndChangeNormal, VerifyPrerequisite, NULL, &mMfciVerifyContext01);
  AddTestCase (VerifyAndChangePhaseSuite, "VerifyAndChange should succeed even there is not current policy", "VerifyEmptyCurrent", UnitTestVerifyAndChangeEmptyCurrent, VerifyPrerequisite, NULL, &mMfciVerifyContext02);
  AddTestCase (VerifyAndChangePhaseSuite, "VerifyAndChange should purge current blob if verification failed", "VerifyPurgeWrongCurrent", UnitTestVerifyAndChangePurgeWrongCurrent, VerifyPrerequisite, NULL, &mMfciVerifyContext03);
  AddTestCase (VerifyAndChangePhaseSuite, "VerifyAndChange should purge target blob if verification failed", "VerifyPurgeWrongTarget", UnitTestVerifyAndChangePurgeWrongTarget, VerifyPrerequisite, NULL, &mMfciVerifyContext04);
  AddTestCase (VerifyAndChangePhaseSuite, "VerifyAndChange should create next nonce when there is none", "VerifyCreateNextNonce", UnitTestVerifyAndChangeCreateNextNonce, VerifyPrerequisite, NULL, NULL);
  AddTestCase (VerifyAndChangePhaseSuite, "VerifyAndChange should clean up current policy when there are errors", "VerifyCleanCurrent", UnitTestVerifyAndChangeCleanCurrentOnFailure, VerifyPrerequisite, NULL, &mMfciVerifyContext05);
  AddTestCase (VerifyAndChangePhaseSuite, "VerifyAndChange should keep cleaning even when single operation failed", "VerifyCleanCurrentVarClean", UnitTestVerifyAndChangeCleanContinueOnFailure, VerifyPrerequisite, NULL, &mMfciVerifyContext05);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  return UefiTestMain ();
}
