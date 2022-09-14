/** @file
  This module tests MFCI XDR formatted certificate extraction
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
#include <UnitTests/MfciPolicyParsingUnitTest/data/certs/CA.cer.h>
#include <UnitTests/MfciPolicyParsingUnitTest/data/certs/CA.cer.xdr.h>
#include <UnitTests/MfciPolicyParsingUnitTest/data/certs/Root.cer.h>
#include <UnitTests/MfciPolicyParsingUnitTest/data/certs/CA-Root.cer.xdr.h>

#include "../MfciDxe.h"

#define UNIT_TEST_NAME     "Mfci Multiple Certificates Host Test"
#define UNIT_TEST_VERSION  "0.1"

BOOLEAN
EFIAPI
GetRandomNumber64 (
  OUT     UINT64  *Rand
  )
{
  // Not used
  ASSERT (FALSE);
  return FALSE;
}

VOID
EFIAPI
ResetSystemWithSubtype (
  IN EFI_RESET_TYPE  ResetType,
  IN CONST  GUID     *ResetSubtype
  )
{
  // Not used
  ASSERT (FALSE);
}

EFI_STATUS
EFIAPI
InitPublicInterface (
  VOID
  )
{
  // Not used
  ASSERT (FALSE);
  return EFI_DEVICE_ERROR;
}

EFI_STATUS
EFIAPI
VerifyTargeting (
  VOID              *PolicyBlob,
  UINTN             PolicyBlobSize,
  UINT64            ExpectedNonce,
  MFCI_POLICY_TYPE  *ExtractedPolicy
  )
{
  // Not used
  ASSERT (FALSE);
  return EFI_DEVICE_ERROR;
}

EFI_STATUS
EFIAPI
NotifyMfciPolicyChange (
  IN MFCI_POLICY_TYPE  NewPolicy
  )
{
  // Not used
  ASSERT (FALSE);
  return EFI_DEVICE_ERROR;
}

EFI_STATUS
EFIAPI
InitSecureBootListener (
  VOID
  )
{
  // Not used
  ASSERT (FALSE);
  return EFI_DEVICE_ERROR;
}

EFI_STATUS
EFIAPI
InitTpmListener (
  VOID
  )
{
  // Not used
  ASSERT (FALSE);
  return EFI_DEVICE_ERROR;
}

EFI_RUNTIME_SERVICES  mMockRuntime;

/**
 * Validate blob on each certificate from preset XDR buffer.
 *
 * @param SignedPolicy        Pointer to hold the policy buffer to be validated.
 * @param SignedPolicySize    Size of SignedPolicy, in bytes.
 * @param Certificates        Pointer to hold the XDR formatted buffer of certificates.
 * @param CertificatesSize    Size of Certificates, in bytes.
 *
 * @retval EFI_SUCCESS        The one certificate from Certificate is valid for input policy validation.
 * @retval EFI_ABORTED        SignedPolicy is null data or at least one certificate from incoming Certificates is
 *                            malformatted.
 * @retval Others             Other errors from the underlying ValidateBlob function.
 */
EFI_STATUS
ValidateBlobWithXdrCertificates (
  IN CONST UINT8  *SignedPolicy,
  IN UINTN        SignedPolicySize,
  IN CONST UINT8  *Certificates,
  IN UINTN        CertificatesSize
  );

EFI_STATUS
EFIAPI
ValidateBlob (
  IN CONST UINT8  *SignedPolicy,
  UINTN           SignedPolicySize,
  IN CONST UINT8  *TrustAnchorCert,
  IN UINTN        TrustAnchorCertSize,
  IN CONST CHAR8  *EKU
  )
{
  assert_ptr_equal (EKU, (CHAR8 *)FixedPcdGetPtr (PcdMfciPkcs7RequiredLeafEKU));

  check_expected (SignedPolicy);
  check_expected (SignedPolicySize);
  check_expected (TrustAnchorCert);
  check_expected (TrustAnchorCertSize);

  return (EFI_STATUS)mock ();
}

/**
  Unit test for ValidateBlobWithXdrCertificates () API with single certificate
  from the input XDR buffer.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
MfciMultipleCertificatesShouldParseSingleCert (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dummy;

  expect_value (ValidateBlob, SignedPolicy, &Dummy);
  expect_value (ValidateBlob, SignedPolicySize, sizeof (Dummy));
  expect_value (ValidateBlob, TrustAnchorCertSize, sizeof (mCert_Trusted_CA));
  expect_memory (ValidateBlob, TrustAnchorCert, mCert_Trusted_CA, sizeof (mCert_Trusted_CA));
  will_return (ValidateBlob, EFI_SUCCESS);

  Status = ValidateBlobWithXdrCertificates (&Dummy, sizeof (Dummy), mCert_Trusted_CA_xdr, sizeof (mCert_Trusted_CA_xdr));
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for ValidateBlobWithXdrCertificates () API with multiple certificates
  from the input XDR buffer.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
MfciMultipleCertificatesShouldParseMultipleCert (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dummy;

  expect_value (ValidateBlob, SignedPolicy, &Dummy);
  expect_value (ValidateBlob, SignedPolicySize, sizeof (Dummy));
  expect_value (ValidateBlob, TrustAnchorCertSize, sizeof (mCert_Trusted_CA));
  expect_memory (ValidateBlob, TrustAnchorCert, mCert_Trusted_CA, sizeof (mCert_Trusted_CA));
  will_return (ValidateBlob, EFI_SECURITY_VIOLATION);

  expect_value (ValidateBlob, SignedPolicy, &Dummy);
  expect_value (ValidateBlob, SignedPolicySize, sizeof (Dummy));
  expect_value (ValidateBlob, TrustAnchorCertSize, sizeof (mCertRoot_cer));
  expect_memory (ValidateBlob, TrustAnchorCert, mCertRoot_cer, sizeof (mCertRoot_cer));
  will_return (ValidateBlob, EFI_SUCCESS);

  Status = ValidateBlobWithXdrCertificates (&Dummy, sizeof (Dummy), mCert_Trusted_CA_Root_xdr, sizeof (mCert_Trusted_CA_Root_xdr));
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for ValidateBlobWithXdrCertificates () API with all failed validation.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
MfciMultipleCertificatesShouldPropagateResult (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dummy;

  expect_value (ValidateBlob, SignedPolicy, &Dummy);
  expect_value (ValidateBlob, SignedPolicySize, sizeof (Dummy));
  expect_value (ValidateBlob, TrustAnchorCertSize, sizeof (mCert_Trusted_CA));
  expect_memory (ValidateBlob, TrustAnchorCert, mCert_Trusted_CA, sizeof (mCert_Trusted_CA));
  will_return (ValidateBlob, EFI_SECURITY_VIOLATION);

  expect_value (ValidateBlob, SignedPolicy, &Dummy);
  expect_value (ValidateBlob, SignedPolicySize, sizeof (Dummy));
  expect_value (ValidateBlob, TrustAnchorCertSize, sizeof (mCertRoot_cer));
  expect_memory (ValidateBlob, TrustAnchorCert, mCertRoot_cer, sizeof (mCertRoot_cer));
  will_return (ValidateBlob, EFI_COMPROMISED_DATA);

  Status = ValidateBlobWithXdrCertificates (&Dummy, sizeof (Dummy), mCert_Trusted_CA_Root_xdr, sizeof (mCert_Trusted_CA_Root_xdr));
  UT_ASSERT_STATUS_EQUAL (Status, EFI_COMPROMISED_DATA);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for ValidateBlobWithXdrCertificates () API with empty inputs.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
MfciMultipleCertificatesShouldCheckInputs (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dummy;

  Status = ValidateBlobWithXdrCertificates (&Dummy, sizeof (Dummy), NULL, sizeof (mCert_Trusted_CA_Root_xdr));
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ABORTED);

  Status = ValidateBlobWithXdrCertificates (&Dummy, sizeof (Dummy), mCert_Trusted_CA_Root_xdr, 0);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ABORTED);

  Status = ValidateBlobWithXdrCertificates (NULL, sizeof (Dummy), mCert_Trusted_CA_Root_xdr, sizeof (mCert_Trusted_CA_Root_xdr));
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ABORTED);

  Status = ValidateBlobWithXdrCertificates (&Dummy, 0, mCert_Trusted_CA_Root_xdr, sizeof (mCert_Trusted_CA_Root_xdr));
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ABORTED);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for ValidateBlobWithXdrCertificates () API should generally inspect
  incoming certificates.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
MfciMultipleCertificatesShouldCheckGeneralCertificates (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dummy;
  UINT8       FakeCertificate;

  Status = ValidateBlobWithXdrCertificates (&Dummy, sizeof (Dummy), &FakeCertificate, sizeof (FakeCertificate));
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ABORTED);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for ValidateBlobWithXdrCertificates () API should inspect individual
  incoming certificates.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
MfciMultipleCertificatesShouldCheckIndividualCertificate (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dummy;
  // Not enough for individual size field
  UINT8  FakeCertificate1[] = { 0x00, 0x01, 0x02, 0x03, 0x04 };
  // No content individual certificate
  UINT8  FakeCertificate2[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };
  // Content shorter than noted size
  UINT8  FakeCertificate3[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };

  Status = ValidateBlobWithXdrCertificates (&Dummy, sizeof (Dummy), FakeCertificate1, sizeof (FakeCertificate1));
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ABORTED);

  Status = ValidateBlobWithXdrCertificates (&Dummy, sizeof (Dummy), FakeCertificate2, sizeof (FakeCertificate2));
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ABORTED);

  Status = ValidateBlobWithXdrCertificates (&Dummy, sizeof (Dummy), FakeCertificate3, sizeof (FakeCertificate3));
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ABORTED);

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
  UNIT_TEST_SUITE_HANDLE      MultipleCertificatesSuite;

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
  // Populate the MultipleCertificatesSuite Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&MultipleCertificatesSuite, Framework, "TargetVerifyPhase", "ReportRouter.Phase", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MultipleCertificatesSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (MultipleCertificatesSuite, "ValidateBlobWithXdrCertificates should succeed with correct single XDR cert", "GoodSingle", MfciMultipleCertificatesShouldParseSingleCert, NULL, NULL, NULL);
  AddTestCase (MultipleCertificatesSuite, "ValidateBlobWithXdrCertificates should succeed with correct multiple XDR certs", "GoodMultiple", MfciMultipleCertificatesShouldParseMultipleCert, NULL, NULL, NULL);
  AddTestCase (MultipleCertificatesSuite, "ValidateBlobWithXdrCertificates should propagate result if failure", "PropagateResult", MfciMultipleCertificatesShouldPropagateResult, NULL, NULL, NULL);
  AddTestCase (MultipleCertificatesSuite, "ValidateBlobWithXdrCertificates should check inputs for validity", "CheckInputs", MfciMultipleCertificatesShouldCheckInputs, NULL, NULL, NULL);
  AddTestCase (MultipleCertificatesSuite, "ValidateBlobWithXdrCertificates should inspect general input certificates", "CheckGeneral", MfciMultipleCertificatesShouldCheckGeneralCertificates, NULL, NULL, NULL);
  AddTestCase (MultipleCertificatesSuite, "ValidateBlobWithXdrCertificates should inspect individual input certificates", "CheckIndividual", MfciMultipleCertificatesShouldCheckIndividualCertificate, NULL, NULL, NULL);

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
