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
#define UNIT_TEST_NAME     "Authenticated Variables Basic Usage Tests"
#define UNIT_TEST_VERSION  "0.1"

#define VARIABLE_KEY_LENGTH_SUPPORT_2048  (0)
#define VARIABLE_KEY_LENGTH_SUPPORT_3072  (1)
#define VARIABLE_KEY_LENGTH_SUPPORT_4096  (2)
#define VARIABLE_KEY_LENGTH_SUPPORT_END   (3)

#define ADDITIONAL_CERTIFICATES_SUPPORT_1    (0)
#define ADDITIONAL_CERTIFICATES_SUPPORT_2    (1)
#define ADDITIONAL_CERTIFICATES_SUPPORT_3    (2)
#define ADDITIONAL_CERTIFICATES_SUPPORT_END  (3)

#define PREVENT_UPDATE_SUPPORT_INITIAL_VARIABLE  (0)
#define PREVENT_UPDATE_SUPPORT_INVALID_VARIABLE  (1)
#define PREVENT_UPDATE_SUPPORT_END               (2)

#define PREVENT_ROLLBACK_SUPPORT_FUTURE_VARIABLE  (0)
#define PREVENT_ROLLBACK_SUPPORT_PAST_VARIABLE    (1)
#define PREVENT_ROLLBACK_SUPPORT_END              (2)

// This test being tested must only have 2 signatures to test against
#define BASIC_USAGE_2_VARIABLES_CHAIN_LENGTH  (2)

// This test being tested must only have 3 signatures to test against
#define BASIC_USAGE_3_VARIABLES_CHAIN_LENGTH  (3)

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
  CHAR16              *TestName;                                   // The test name
  VARIABLE_CONTEXT    Chain[BASIC_USAGE_2_VARIABLES_CHAIN_LENGTH]; // The chain
  UINT16              ChainLength;                                 // The length of the chain
} VARIABLES_2_CHAIN_CONTEXT;

typedef struct {
  CHAR16              *TestName;                                   // The test name
  VARIABLE_CONTEXT    Chain[BASIC_USAGE_3_VARIABLES_CHAIN_LENGTH]; // The chain
  UINT16              ChainLength;                                 // The length of the chain
} VARIABLES_3_CHAIN_CONTEXT;

// *----------------------------------------------------------------------------------*
// * Test Contexts                                                                    *
// *----------------------------------------------------------------------------------*

STATIC VARIABLES_3_CHAIN_CONTEXT  mVariableKeyLengthSupport = {
  .TestName                                = L"Variable Key Length Support",
  .Chain[VARIABLE_KEY_LENGTH_SUPPORT_2048] =  {
    .Name          = L"MockVar",
    .Note          = L"supports 2048 bit keys",
    .Data          = m2048VariableKeyLengthSupportMockVar,
    .DataSize      = sizeof m2048VariableKeyLengthSupportMockVar,
    .ClearData     = m2048VariableKeyLengthSupportMockVarEmpty,
    .ClearDataSize = sizeof m2048VariableKeyLengthSupportMockVarEmpty,
    .ExpectedData  = m2048VariableKeyLengthSupportMockVarExpected
  },
  .Chain[VARIABLE_KEY_LENGTH_SUPPORT_3072] =  {
    .Name          = L"MockVar",
    .Note          = L"supports 3072 bit keys",
    .Data          = m3072VariableKeyLengthSupportMockVar,
    .DataSize      = sizeof m3072VariableKeyLengthSupportMockVar,
    .ClearData     = m3072VariableKeyLengthSupportMockVarEmpty,
    .ClearDataSize = sizeof m3072VariableKeyLengthSupportMockVarEmpty,
    .ExpectedData  = m3072VariableKeyLengthSupportMockVarExpected
  },
  .Chain[VARIABLE_KEY_LENGTH_SUPPORT_4096] =  {
    .Name          = L"MockVar",
    .Note          = L"supports 4096 bit keys",
    .Data          = m4096VariableKeyLengthSupportMockVar,
    .DataSize      = sizeof m4096VariableKeyLengthSupportMockVar,
    .ClearData     = m4096VariableKeyLengthSupportMockVarEmpty,
    .ClearDataSize = sizeof m4096VariableKeyLengthSupportMockVarEmpty,
    .ExpectedData  = m4096VariableKeyLengthSupportMockVarExpected
  },
  .ChainLength      = VARIABLE_KEY_LENGTH_SUPPORT_END
};

STATIC VARIABLES_3_CHAIN_CONTEXT  mAdditionalCertificateSupport = {
  .TestName                                 = L"Additional Certificates Support",
  .Chain[ADDITIONAL_CERTIFICATES_SUPPORT_1] =  {
    .Name          = L"MockVar",
    .Note          = L"supports a chain of 1 certificate",
    .Data          = m1AdditionalCertificatesMockVar,
    .DataSize      = sizeof m1AdditionalCertificatesMockVar,
    .ClearData     = m1AdditionalCertificatesMockVarEmpty,
    .ClearDataSize = sizeof m1AdditionalCertificatesMockVarEmpty,
    .ExpectedData  = m1AdditionalCertificatesMockVarExpected
  },
  .Chain[ADDITIONAL_CERTIFICATES_SUPPORT_2] =  {
    .Name          = L"MockVar",
    .Note          = L"supports a chain of 2 certificates",
    .Data          = m2AdditionalCertificatesMockVar,
    .DataSize      = sizeof m2AdditionalCertificatesMockVar,
    .ClearData     = m2AdditionalCertificatesMockVarEmpty,
    .ClearDataSize = sizeof m2AdditionalCertificatesMockVarEmpty,
    .ExpectedData  = m2AdditionalCertificatesMockVarExpected
  },
  .Chain[ADDITIONAL_CERTIFICATES_SUPPORT_3] =  {
    .Name          = L"MockVar",
    .Note          = L"supports a chain of 3 certificates",
    .Data          = m3AdditionalCertificatesMockVar,
    .DataSize      = sizeof m3AdditionalCertificatesMockVar,
    .ClearData     = m3AdditionalCertificatesMockVarEmpty,
    .ClearDataSize = sizeof m3AdditionalCertificatesMockVarEmpty,
    .ExpectedData  = m3AdditionalCertificatesMockVarExpected
  },
  .ChainLength      = ADDITIONAL_CERTIFICATES_SUPPORT_END
};

STATIC VARIABLES_2_CHAIN_CONTEXT  mPreventUpdateSupport = {
  .TestName                                       = L"Invalid Certificiate Support",
  .Chain[PREVENT_UPDATE_SUPPORT_INITIAL_VARIABLE] =  {
    .Name          = L"MockVar",
    .Note          = L"initial variable data",
    .Data          = mPreventUpdateInitVariableMockVar,
    .DataSize      = sizeof mPreventUpdateInitVariableMockVar,
    .ClearData     = mPreventUpdateInitVariableMockVarEmpty,
    .ClearDataSize = sizeof mPreventUpdateInitVariableMockVarEmpty,
    .ExpectedData  = mPreventUpdateInitVariableMockVarExpected
  },
  .Chain[PREVENT_UPDATE_SUPPORT_INVALID_VARIABLE] =  {
    .Name          = L"MockVar",
    .Note          = L"attempt at updating the variable",
    .Data          = mPreventUpdateInvalidVariableMockVar,
    .DataSize      = sizeof mPreventUpdateInvalidVariableMockVar,
    .ClearData     = mPreventUpdateInvalidVariableMockVarEmpty,
    .ClearDataSize = sizeof mPreventUpdateInvalidVariableMockVarEmpty,
    .ExpectedData  = mPreventUpdateInvalidVariableMockVarExpected
  },
  .ChainLength      = PREVENT_UPDATE_SUPPORT_END
};

// The first variable in the chain should be the variable in the future
// This means that when the second variable in the past comes in it should fail
STATIC VARIABLES_2_CHAIN_CONTEXT  mPreventRollbackSupport = {
  .TestName                                        = L"Invalid Certificiate Support",
  .Chain[PREVENT_ROLLBACK_SUPPORT_FUTURE_VARIABLE] =  {
    .Name          = L"MockVar",
    .Note          = L"variable in the future",
    .Data          = mPreventRollbackFutureVariableMockVar,
    .DataSize      = sizeof mPreventRollbackFutureVariableMockVar,
    .ClearData     = mPreventRollbackFutureVariableMockVarEmpty,
    .ClearDataSize = sizeof mPreventRollbackFutureVariableMockVarEmpty,
    .ExpectedData  = mPreventRollbackFutureVariableMockVarExpected
  },
  .Chain[PREVENT_ROLLBACK_SUPPORT_PAST_VARIABLE] =  {
    .Name          = L"MockVar",
    .Note          = L"variable in the past",
    .Data          = mPreventRollbackPastVariableMockVar,
    .DataSize      = sizeof mPreventRollbackPastVariableMockVar,
    .ClearData     = mPreventRollbackPastVariableMockVarEmpty,
    .ClearDataSize = sizeof mPreventRollbackPastVariableMockVarEmpty,
    .ExpectedData  = mPreventRollbackPastVariableMockVarExpected
  },
  .ChainLength      = PREVENT_UPDATE_SUPPORT_END
  };

/**
  This function is used to try and clean up the variables that were installed during the test
  It will cycle through the chain of variables and attempt to clear them.

  @param   Context             The test context

  @retval  UNIT_TEST_PASSED             The test passed
  @retval  UNIT_TEST_ERROR_TEST_FAILED  The test failed
**/
static
VOID
EFIAPI
BasicUsage3VariablesTestCleanup (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN   BufferSize = 0;
  UINT32  Attributes = 0;

  EFI_STATUS                 Status;
  VARIABLE_CONTEXT           Variable;
  VARIABLES_3_CHAIN_CONTEXT  *BasicUsageContext;

  // Grab the context for installing the variable
  BasicUsageContext = (VARIABLES_3_CHAIN_CONTEXT *)Context;

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

// *----------------------------------------------------------------------------------*
// * Test Functions                                                                   *
// *----------------------------------------------------------------------------------*

/**
  Authenticated Variable shall not be updated by an invalid certificate

  These certificates shall not chain up to the same Trust anchor. Otherwise this *should* break this test.

  @param   Context             The test context

  @retval  UNIT_TEST_PASSED             The test passed
  @retval  UNIT_TEST_ERROR_TEST_FAILED  The test failed
**/
static
UNIT_TEST_STATUS
EFIAPI
PreventUpdateTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VARIABLE_CONTEXT  Variable;
  EFI_STATUS        Status;

  UNIT_TEST_STATUS           TestStatus = UNIT_TEST_ERROR_TEST_FAILED;
  VARIABLES_2_CHAIN_CONTEXT  *PreventContext;

  PreventContext = (VARIABLES_2_CHAIN_CONTEXT *)Context;

  // Assert that the chain length being provided is atleast 2
  UT_ASSERT_EQUAL (PreventContext->ChainLength, BASIC_USAGE_2_VARIABLES_CHAIN_LENGTH);

  for (UINTN i = 0; i < PreventContext->ChainLength; i++) {
    Variable = PreventContext->Chain[i];

    // Set the original variable
    Status = gRT->SetVariable (
                    Variable.Name,
                    &gUefiTestingPkgTokenSpaceGuid,
                    VARIABLE_ATTRIBUTE_NV_BS_RT_AT,
                    Variable.DataSize,
                    Variable.Data
                    );

    if ((!EFI_ERROR (Status) && (i == 0)) || ((Status == EFI_SECURITY_VIOLATION) && (i > 0))) {
      // if the status from the first placement of the variable is Success
      // or if the status is EFI_SECURITY_VIOLATION and we are attempting any other placement other than the first
      continue;
    } else if (!EFI_ERROR (Status)) {
      // Otherwise if the status is success and we are attempting any other placement other than the first
      UT_LOG_ERROR ("SetVariable of \"%s\" worked when it wasn't expected to. Return code %r\n", PreventContext->TestName, Status);
      goto Exit;
    } else if (EFI_ERROR (Status) && (Status != EFI_SECURITY_VIOLATION)) {
      // Otherwise if the placement is a an error but it wasn't a EFI_SECURITY_VIOLATION
      UT_LOG_ERROR ("SetVariable of \"%s\" failed when it wasn't expected to. Return code %r\n", PreventContext->TestName, Status);
      goto Exit;
    }
  }

  // Clean up will be performed in the associated clean up function
  TestStatus = UNIT_TEST_PASSED;
Exit:

  return TestStatus;
}

/**
  Basic usage of a Authenticated Variable
  // TODO

  @param   Context             The test context

  @retval  UNIT_TEST_PASSED             The test passed
  @retval  UNIT_TEST_ERROR_TEST_FAILED  The test failed
**/
static
UNIT_TEST_STATUS
EFIAPI
BasicUsage3VariablesTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VARIABLES_3_CHAIN_CONTEXT  *BasicUsageContext;
  EFI_STATUS                 Status;
  UINT8                      *Buffer    = NULL;
  UINTN                      BufferSize = 0;
  UINT32                     Attributes = 0;
  VARIABLE_CONTEXT           Variable;

  BasicUsageContext = (VARIABLES_3_CHAIN_CONTEXT *)Context;

  UT_LOG_INFO ("TESTING: %s\n", BasicUsageContext->TestName);

  // For each key in the chain loop over and verify that we can set and clear them
  for (UINTN i = 0; i < BasicUsageContext->ChainLength; i++) {
    Variable = BasicUsageContext->Chain[i];

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

    // The SetVariable should succeed, since the crypto package should support that keysize
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
AuthenticatedVariablesBasicTestMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  // Baseline Test
  UNIT_TEST_SUITE_HANDLE  BasicUsageTest;

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
  Status = CreateUnitTestSuite (&BasicUsageTest, Framework, "Basic Usage Test", "AuthenciatedVariables.BasicUsage", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for Baseline2kKeysTest\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  // -----------Suite-----------Description-----------------------Class----------------------------Test Function-------------Pre---Clean----------------------------Context
  AddTestCase (BasicUsageTest, "Variable Key Length Support", "VariableKeyLengthSupport", BasicUsage3VariablesTest, NULL, BasicUsage3VariablesTestCleanup, &mVariableKeyLengthSupport);
  AddTestCase (BasicUsageTest, "Additional Certificates Support", "AdditionalCertificatesSupport", BasicUsage3VariablesTest, NULL, BasicUsage3VariablesTestCleanup, &mAdditionalCertificateSupport);
  AddTestCase (BasicUsageTest, "Prevent Update Support",          "PreventUpdateSupport",          PreventUpdateTest,        NULL, BasicUsage2VariablesTestCleanup, &mPreventUpdateSupport);
  AddTestCase (BasicUsageTest, "Prevent Rollback Support",        "PreventRollbackSupport",        PreventUpdateTest,        NULL, BasicUsage2VariablesTestCleanup, &mPreventRollbackSupport);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);
  DEBUG ((DEBUG_INFO, "SecureBootKeysAudit - Return Code %r\n", Status));

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}
