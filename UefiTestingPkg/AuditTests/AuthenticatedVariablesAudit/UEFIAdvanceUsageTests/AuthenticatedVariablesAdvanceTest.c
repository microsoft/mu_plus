/** @file
  SecureBootKeysAudit.c

  This Test tests UEFI SetVariable and GetVariable Certificate's and Certificate Chains

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h> // AllocateZero FreePool
#include <Library/BaseMemoryLib.h>
#include <Guid/VariableFormat.h>

#include "AuthData.h"

// *----------------------------------------------------------------------------------*
// * Defines                                                                          *
// *----------------------------------------------------------------------------------*
#define UNIT_TEST_NAME     "Authenticated Variables Advanced Usage Tests"
#define UNIT_TEST_VERSION  "0.1"

#define DIGEST_ALGORITHM_384  (0)
#define DIGEST_ALGORITHM_512  (1)
#define DIGEST_ALGORITHM_END  (2)

#define TRUST_ANCHOR_SIGNER_1    (0)
#define TRUST_ANCHOR_SIGNER_2    (1)
#define TRUST_ANCHOR_SIGNER_END  (2)

#define MULTI_SIGNER_SIGNER_1    (0)
#define MULTI_SIGNER_SIGNER_2    (1)
#define MULTI_SIGNER_SIGNER_END  (2)

// This test being tested must only have 2 signatures to test against
#define ADVANCED_USAGE_2_VARIABLES_CHAIN_LENGTH  (2)

// *----------------------------------------------------------------------------------*
// * Test Structures                                                                  *
// *----------------------------------------------------------------------------------*
typedef struct {
  CHAR16    *Name;         // Name of the UEFI Variable
  CHAR16    *Note;         // The note for the variable
  UINT32    Attributes;    // The attributes for the variable
  UINT8     *Data;         // Data to install
  UINT8     *ClearData;    // Data to clear the variable
  UINTN     DataSize;      // Size of the install data
  UINTN     ClearDataSize; // Size of the clear data
  UINT8     *ExpectedData; // The expected result
} VARIABLE_CONTEXT;

typedef struct {
  CHAR16              *TestName;                                      // The test name
  VARIABLE_CONTEXT    Chain[ADVANCED_USAGE_2_VARIABLES_CHAIN_LENGTH]; // The chain
  UINT16              ChainLength;                                    // The length of the chain
} VARIABLES_2_CHAIN_CONTEXT;

// *----------------------------------------------------------------------------------*
// * Test Contexts                                                                    *
// *----------------------------------------------------------------------------------*

STATIC VARIABLES_2_CHAIN_CONTEXT  mDigestAlgorithSupport = {
  .TestName                    = L"Supports multiple digest algorithms",
  .Chain[DIGEST_ALGORITHM_384] =  {
    .Name          = L"MockVar",
    .Note          = L"supports SHA-384",
    .Data          = mSHA384DigestAlgorithmsSupportMockVar,
    .DataSize      = sizeof mSHA384DigestAlgorithmsSupportMockVar,
    .ClearData     = mSHA384DigestAlgorithmsSupportMockVarEmpty,
    .ClearDataSize = sizeof mSHA384DigestAlgorithmsSupportMockVarEmpty,
    .ExpectedData  = mSHA384DigestAlgorithmsSupportMockVarExpected
  },
  .Chain[DIGEST_ALGORITHM_512] =  {
    .Name          = L"MockVar",
    .Note          = L"supports SHA-512",
    .Data          = mSHA512DigestAlgorithmsSupportMockVar,
    .DataSize      = sizeof mSHA512DigestAlgorithmsSupportMockVar,
    .ClearData     = mSHA512DigestAlgorithmsSupportMockVarEmpty,
    .ClearDataSize = sizeof mSHA512DigestAlgorithmsSupportMockVarEmpty,
    .ExpectedData  = mSHA512DigestAlgorithmsSupportMockVarExpected
  },
  .ChainLength                 = TRUST_ANCHOR_SIGNER_END
};

STATIC VARIABLES_2_CHAIN_CONTEXT  mSignedTrustAnchor = {
  .TestName                     = L"Signed up to Trust Anchor",
  .Chain[TRUST_ANCHOR_SIGNER_1] =  {
    .Name          = L"MockVar",
    .Note          = L"initial set with trusted anchor",
    .Data          = mSigner1TrustAnchorSupportMockVar,
    .DataSize      = sizeof mSigner1TrustAnchorSupportMockVar,
    .ClearData     = mSigner1TrustAnchorSupportMockVarEmpty,
    .ClearDataSize = sizeof mSigner1TrustAnchorSupportMockVarEmpty,
    .ExpectedData  = mSigner1TrustAnchorSupportMockVarExpected
  },
  .Chain[TRUST_ANCHOR_SIGNER_2] =  {
    .Name          = L"MockVar",
    .Note          = L"updated with trusted anchor",
    .Data          = mSigner2TrustAnchorSupportMockVar,
    .DataSize      = sizeof mSigner2TrustAnchorSupportMockVar,
    .ClearData     = mSigner2TrustAnchorSupportMockVarEmpty,
    .ClearDataSize = sizeof mSigner2TrustAnchorSupportMockVarEmpty,
    .ExpectedData  = mSigner2TrustAnchorSupportMockVarExpected
  },
  .ChainLength                  = TRUST_ANCHOR_SIGNER_END
};

STATIC VARIABLES_2_CHAIN_CONTEXT  mMultipleSigners = {
  .TestName                     = L"Signed by Multiple Signers",
  .Chain[MULTI_SIGNER_SIGNER_1] =  {
    .Name          = L"MockVar",
    .Note          = L"initial set with multiple signers",
    .Data          = mSigner1MultipleSignersSupportMockVar,
    .DataSize      = sizeof mSigner1MultipleSignersSupportMockVar,
    .ClearData     = mSigner1MultipleSignersSupportMockVarEmpty,
    .ClearDataSize = sizeof mSigner1MultipleSignersSupportMockVarEmpty,
    .ExpectedData  = mSigner1MultipleSignersSupportMockVarExpected
  },
  .Chain[MULTI_SIGNER_SIGNER_2] =  {
    .Name          = L"MockVar",
    .Note          = L"updated with multiple signers",
    .Data          = mSigner2MultipleSignersSupportMockVar,
    .DataSize      = sizeof mSigner2MultipleSignersSupportMockVar,
    .ClearData     = mSigner2MultipleSignersSupportMockVarEmpty,
    .ClearDataSize = sizeof mSigner2MultipleSignersSupportMockVarEmpty,
    .ExpectedData  = mSigner2MultipleSignersSupportMockVarExpected
  },
  .ChainLength                  = MULTI_SIGNER_SIGNER_END
};

// *----------------------------------------------------------------------------------*
// * Test Helpers                                                                     *
// *----------------------------------------------------------------------------------*

// *----------------------------------------------------------------------------------*
// * Test Functions                                                                   *
// *----------------------------------------------------------------------------------*

static
VOID
EFIAPI
BasicUsage2VariablesTestCleanup (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN   BufferSize = 0;
  UINT32  Attributes = 0;

  EFI_STATUS                 Status;
  VARIABLE_CONTEXT           Variable;
  VARIABLES_2_CHAIN_CONTEXT  *BasicUsageContext;

  // Grab the context for installing the variable
  BasicUsageContext = (VARIABLES_2_CHAIN_CONTEXT *)Context;

  // Get the first variable from the chain
  Variable = BasicUsageContext->Chain[0];

  // Since we're in clean up if the variable exists, then we failed the test and need to find the correct
  // variable in the chain to clean up
  Status = gRT->GetVariable (
                  Variable.Name,
                  &gUefiTestingPkgTokenSpaceGuid,
                  &Attributes,
                  &BufferSize,
                  NULL
                  );
  if (Status == EFI_NOT_FOUND) {
    // The variable was successfully cleared
    // clean up is not required
    return;
  }

  UT_LOG_INFO ("Performing cleanup for test %s\n", BasicUsageContext->TestName);

  // otherwise the variable was not cleared so lets loop over the potential clear data to clear it
  for (UINTN i = 0; i < BasicUsageContext->ChainLength; i++) {
    Variable = BasicUsageContext->Chain[i];

    Status = gRT->SetVariable (
                    Variable.Name,
                    &gUefiTestingPkgTokenSpaceGuid,
                    VARIABLE_ATTRIBUTE_NV_BS_RT_AT,
                    Variable.ClearDataSize,
                    Variable.ClearData
                    );
    if (Status == EFI_SUCCESS) {
      UT_LOG_INFO ("Cleanup attempt was successful:\n");
      return;
    }
  }

  if (EFI_ERROR (Status)) {
    UT_LOG_ERROR ("Cleanup attempts exhausted\n");
  }

  return;
}

static
UNIT_TEST_STATUS
EFIAPI
DigestAlgorithmTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VARIABLES_2_CHAIN_CONTEXT  *AdvUsageContext;
  EFI_STATUS                 Status;
  UINT8                      *Buffer    = NULL;
  UINTN                      BufferSize = 0;
  UINT32                     Attributes = 0;
  VARIABLE_CONTEXT           Variable;

  AdvUsageContext = (VARIABLES_2_CHAIN_CONTEXT *)Context;

  UT_LOG_INFO ("TESTING: %s\n", AdvUsageContext->TestName);

  // For each key in the chain loop over and verify that we can set and clear them
  for (UINTN i = 0; i < AdvUsageContext->ChainLength; i++) {
    Variable = AdvUsageContext->Chain[i];

    UT_LOG_INFO ("Context: %s\n", Variable.Note);

    // All we know at this point is that the variable was claimed to have been set
    Status = gRT->GetVariable (
                    Variable.Name,
                    &gUefiTestingPkgTokenSpaceGuid,
                    &Attributes,
                    &BufferSize,
                    NULL
                    );

    // The GetVariable should fail with EFI_NOT_FOUND, since the variable should not exist
    UT_ASSERT_EQUAL (Status, EFI_NOT_FOUND);

    // Set the authenticated varible, if this fails this indicates the crypto package doesn't support that keysize
    Status = gRT->SetVariable (
                    Variable.Name,
                    &gUefiTestingPkgTokenSpaceGuid,
                    VARIABLE_ATTRIBUTE_NV_BS_RT_AT,
                    Variable.DataSize,
                    Variable.Data
                    );

    // The SetVariable should succeed, since the crypto package should support that keysize and digest algorithm
    UT_ASSERT_EQUAL (Status, EFI_SUCCESS);

    // All we know at this point is that the variable was claimed to have been set
    Status = gRT->GetVariable (
                    Variable.Name,
                    &gUefiTestingPkgTokenSpaceGuid,
                    &Attributes,
                    &BufferSize,
                    NULL
                    );
    // The Attributes returned should match the attributes set
    UT_ASSERT_EQUAL (Attributes, VARIABLE_ATTRIBUTE_NV_BS_RT_AT);

    // Assuming the buffer returned correctly we need to allocate the space to put the data
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Buffer = AllocateZeroPool (BufferSize);
      UT_ASSERT_NOT_NULL (Buffer);
    }

    // Retreive the data originally set in our allocated buffer
    Status = gRT->GetVariable (
                    Variable.Name,
                    &gUefiTestingPkgTokenSpaceGuid,
                    NULL,
                    &BufferSize,
                    Buffer
                    );
    // The GetVariable should succeed, since the crypto package should support that keysize
    UT_ASSERT_NOT_EFI_ERROR (Status);

    // Confirm the data has been set correctly
    UT_ASSERT_EQUAL (CompareMem (Buffer, Variable.ExpectedData, BufferSize), 0);

    // Allocated buffer is no longer needed
    if (Buffer) {
      FreePool (Buffer);
      Buffer     = NULL;
      BufferSize = 0;
    }

    // Try removing the variable to ensure we can clear them successfully
    Status = gRT->SetVariable (
                    Variable.Name,
                    &gUefiTestingPkgTokenSpaceGuid,
                    VARIABLE_ATTRIBUTE_NV_BS_RT_AT,
                    Variable.ClearDataSize,
                    Variable.ClearData
                    );

    UT_ASSERT_NOT_EFI_ERROR (Status);

    // Confirm the variable was cleared
    Status = gRT->GetVariable (
                    Variable.Name,
                    &gUefiTestingPkgTokenSpaceGuid,
                    &Attributes,
                    &BufferSize,
                    NULL
                    );
    // The GetVariable should fail with EFI_NOT_FOUND, since the variable should not exist
    UT_ASSERT_EQUAL (Status, EFI_NOT_FOUND);
  }

  return UNIT_TEST_PASSED;
}

static
UNIT_TEST_STATUS
EFIAPI
UpdateVariableTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VARIABLES_2_CHAIN_CONTEXT  *AdvUsageContext;
  EFI_STATUS                 Status;
  UINT8                      *Buffer    = NULL;
  UINTN                      BufferSize = 0;
  UINT32                     Attributes = 0;
  VARIABLE_CONTEXT           Variable;

  AdvUsageContext = (VARIABLES_2_CHAIN_CONTEXT *)Context;

  UT_LOG_INFO ("TESTING: %s\n", AdvUsageContext->TestName);

  // For each key in the chain loop over and verify that we can set and clear them
  for (UINTN i = 0; i < AdvUsageContext->ChainLength; i++) {
    Variable = AdvUsageContext->Chain[i];

    UT_LOG_INFO ("Context: %s\n", Variable.Note);
    UT_LOG_INFO ("Payload: %d\n", i);

    // Set the authenticated varible, if this fails this indicates the crypto package doesn't support that keysize
    Status = gRT->SetVariable (
                    Variable.Name,
                    &gUefiTestingPkgTokenSpaceGuid,
                    VARIABLE_ATTRIBUTE_NV_BS_RT_AT,
                    Variable.DataSize,
                    Variable.Data
                    );

    // The SetVariable should succeed, since the crypto package should support that keysize and digest algorithm
    UT_ASSERT_EQUAL (Status, EFI_SUCCESS);

    // All we know at this point is that the variable was claimed to have been set
    Status = gRT->GetVariable (
                    Variable.Name,
                    &gUefiTestingPkgTokenSpaceGuid,
                    &Attributes,
                    &BufferSize,
                    NULL
                    );
    // The Attributes returned should match the attributes set
    UT_ASSERT_EQUAL (Attributes, VARIABLE_ATTRIBUTE_NV_BS_RT_AT);

    // Assuming the buffer returned correctly we need to allocate the space to put the data
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Buffer = AllocateZeroPool (BufferSize);
      UT_ASSERT_NOT_NULL (Buffer);
    }

    // Retreive the data originally set in our allocated buffer
    Status = gRT->GetVariable (
                    Variable.Name,
                    &gUefiTestingPkgTokenSpaceGuid,
                    NULL,
                    &BufferSize,
                    Buffer
                    );
    // The GetVariable should succeed, since the crypto package should support that keysize
    UT_ASSERT_NOT_EFI_ERROR (Status);

    // Confirm the data has been set correctly
    UT_ASSERT_EQUAL (CompareMem (Buffer, Variable.ExpectedData, BufferSize), 0);

    // Allocated buffer is no longer needed
    if (Buffer) {
      FreePool (Buffer);
      Buffer     = NULL;
      BufferSize = 0;
    }
  }

  // Allocated buffer is no longer needed
  if (Buffer) {
    FreePool (Buffer);
    Buffer     = NULL;
    BufferSize = 0;
  }

  return UNIT_TEST_PASSED;
}

// *----------------------------------------------------------------------------------*
// * Test Runner                                                                      *
// *----------------------------------------------------------------------------------*

/**
  Initialize the unit test framework, suite, and unit tests for the
  sample unit tests and run the unit tests.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
EFI_STATUS
EFIAPI
AuthenticatedVariablesAdvanceTestMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  // Baseline Test
  UNIT_TEST_SUITE_HANDLE  AdvUsageTest;

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

  //
  // Populate the Baseline2kKeysTest Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&AdvUsageTest, Framework, "Advance Usage Test", "AuthenticatedVariableAdvanceAudit", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for \n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  // -----------Suite-----------Description-----------------------Class----------------------------Test Function--------------Pre---Clean--Context
  AddTestCase (AdvUsageTest, "Digest Algorithm Support", "DigestAlgorithmSupport", DigestAlgorithmTest, NULL, BasicUsage2VariablesTestCleanup, &mDigestAlgorithSupport);
  AddTestCase (AdvUsageTest, "Update by Trust Anchor Support", "UpdateByTrustAnchorSupport", UpdateVariableTest, NULL, BasicUsage2VariablesTestCleanup, &mSignedTrustAnchor);
  AddTestCase (AdvUsageTest, "Update by Multiple Signatures Support", "UpdateByMultipleSignaturesSupport", UpdateVariableTest, NULL, BasicUsage2VariablesTestCleanup, &mMultipleSigners);

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
