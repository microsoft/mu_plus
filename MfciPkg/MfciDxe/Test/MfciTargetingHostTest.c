/** @file
  This module tests targeting logic of MfciDxe driver.

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
#include <Protocol/MfciProtocol.h>
#include <Library/MfciDeviceIdSupportLib.h>
#include <Library/MfciPolicyParsingLib.h>
#include <Library/MfciRetrievePolicyLib.h>

#include <Library/BaseLib.h>                             // CpuDeadLoop()
#include <Library/DebugLib.h>                            // DEBUG tracing
#include <Library/BaseMemoryLib.h>                       // CopyGuid()
#include <Library/VariablePolicyHelperLib.h>             // NotifyMfciPolicyChange()
#include <Library/MemoryAllocationLib.h>

#include <Library/UnitTestLib.h>
#include <UnitTests/MfciPolicyParsingUnitTest/data/packets/policy_good_manufacturing.bin.h>
#include <UnitTests/MfciPolicyParsingUnitTest/data/packets/policy_good_manufacturing.bin.p7.h>

#include "../MfciDxe.h"

#define UNIT_TEST_NAME     "Mfci Targeting Host Test"
#define UNIT_TEST_VERSION  "0.1"

// Simply use policy_good_manufacturing based on UnitTests/data/packets/GenPacket.py
#define MFCI_TEST_MANUFACTURER  L"Contoso Computers, LLC"
#define MFCI_TEST_PRODUCT       L"Laptop Foo"
#define MFCI_TEST_SERIAL_NUM    L"F0013-000243546-X02"
#define MFCI_TEST_OEM_01        L"ODM Foo"
#define MFCI_TEST_OEM_02        L""
#define MFCI_TEST_NONCE         0x0123456789abcdef
#define MFCI_TEST_POLICY        (STD_ACTION_SECURE_BOOT_CLEAR | STD_ACTION_TPM_CLEAR)

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

EFI_RUNTIME_SERVICES  mMockRuntime = {
  .GetVariable = UnitTestGetVariable
};

typedef struct {
  CHAR16        *Manufacturer;
  CHAR16        *Product;
  CHAR16        *SerialNum;
  CHAR16        *Oem01;
  CHAR16        *Oem02;
  UINT64        Nonce;
  EFI_STATUS    ExpectedReturn;
} MFCI_UNIT_TEST_CONTEXT;

MFCI_UNIT_TEST_CONTEXT  *mCurrentMfciTarget;

MFCI_UNIT_TEST_CONTEXT  mMfciTargetContext01 = { MFCI_TEST_MANUFACTURER, MFCI_TEST_PRODUCT, MFCI_TEST_SERIAL_NUM, MFCI_TEST_OEM_01, MFCI_TEST_OEM_02, MFCI_TEST_NONCE, EFI_SUCCESS };
MFCI_UNIT_TEST_CONTEXT  mMfciTargetContext02 = { L"MFCI_TEST_MANUFACTURER", MFCI_TEST_PRODUCT, MFCI_TEST_SERIAL_NUM, MFCI_TEST_OEM_01, MFCI_TEST_OEM_02, MFCI_TEST_NONCE, EFI_SECURITY_VIOLATION };
MFCI_UNIT_TEST_CONTEXT  mMfciTargetContext03 = { MFCI_TEST_MANUFACTURER, L"MFCI_TEST_PRODUCT", MFCI_TEST_SERIAL_NUM, MFCI_TEST_OEM_01, MFCI_TEST_OEM_02, MFCI_TEST_NONCE, EFI_SECURITY_VIOLATION };
MFCI_UNIT_TEST_CONTEXT  mMfciTargetContext04 = { MFCI_TEST_MANUFACTURER, MFCI_TEST_PRODUCT, L"MFCI_TEST_SERIAL_NUM", MFCI_TEST_OEM_01, MFCI_TEST_OEM_02, MFCI_TEST_NONCE, EFI_SECURITY_VIOLATION };
MFCI_UNIT_TEST_CONTEXT  mMfciTargetContext05 = { MFCI_TEST_MANUFACTURER, MFCI_TEST_PRODUCT, MFCI_TEST_SERIAL_NUM, L"MFCI_TEST_OEM_01", MFCI_TEST_OEM_02, MFCI_TEST_NONCE, EFI_SECURITY_VIOLATION };
MFCI_UNIT_TEST_CONTEXT  mMfciTargetContext06 = { MFCI_TEST_MANUFACTURER, MFCI_TEST_PRODUCT, MFCI_TEST_SERIAL_NUM, MFCI_TEST_OEM_01, L"MFCI_TEST_OEM_02", MFCI_TEST_NONCE, EFI_SECURITY_VIOLATION };
MFCI_UNIT_TEST_CONTEXT  mMfciTargetContext07 = { MFCI_TEST_MANUFACTURER, MFCI_TEST_PRODUCT, MFCI_TEST_SERIAL_NUM, MFCI_TEST_OEM_01, MFCI_TEST_OEM_02, 0, EFI_SECURITY_VIOLATION };

BOOLEAN
EFIAPI
Pkcs7GetAttachedContent (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT VOID         **Content,
  OUT UINTN        *ContentSize
  )
{
  UT_ASSERT_EQUAL (P7Data, mSigned_policy_good_manufacturing);
  UT_ASSERT_EQUAL (P7Length, sizeof (mSigned_policy_good_manufacturing));

  *Content     = AllocateCopyPool (sizeof (mBin_policy_good_manufacturing), mBin_policy_good_manufacturing);
  *ContentSize = sizeof (mBin_policy_good_manufacturing);
  return TRUE;
}

BOOLEAN
EFIAPI
Pkcs7Verify (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  IN  CONST UINT8  *TrustedCert,
  IN  UINTN        CertLength,
  IN  CONST UINT8  *InData,
  IN  UINTN        DataLength
  )
{
  ASSERT (FALSE);
  return FALSE;
}

EFI_STATUS
EFIAPI
VerifyEKUsInPkcs7Signature (
  IN CONST UINT8   *Pkcs7Signature,
  IN CONST UINT32  SignatureSize,
  IN CONST CHAR8   *RequiredEKUs[],
  IN CONST UINT32  RequiredEKUsSize,
  IN BOOLEAN       RequireAllPresent
  )
{
  ASSERT (FALSE);
  return EFI_NOT_READY;
}

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
  VOID        *ReturnVal   = NULL;
  UINTN       Size         = 0;

  DEBUG ((DEBUG_ERROR, "%a: %s\n", __FUNCTION__, VariableName));
  if (StrCmp (VariableName, gPolicyBlobFieldName[MFCI_POLICY_TARGET_MANUFACTURER]) == 0) {
    ReturnVal = mCurrentMfciTarget->Manufacturer;
    Size      = (StrLen (ReturnVal) + 1) * 2;
  } else if (StrCmp (VariableName, gPolicyBlobFieldName[MFCI_POLICY_TARGET_PRODUCT]) == 0) {
    ReturnVal = mCurrentMfciTarget->Product;
    Size      = (StrLen (ReturnVal) + 1) * 2;
  } else if (StrCmp (VariableName, gPolicyBlobFieldName[MFCI_POLICY_TARGET_SERIAL_NUMBER]) == 0) {
    ReturnVal = mCurrentMfciTarget->SerialNum;
    Size      = (StrLen (ReturnVal) + 1) * 2;
  } else if (StrCmp (VariableName, gPolicyBlobFieldName[MFCI_POLICY_TARGET_OEM_01]) == 0) {
    ReturnVal = mCurrentMfciTarget->Oem01;
    Size      = (StrLen (ReturnVal) + 1) * 2;
  } else if (StrCmp (VariableName, gPolicyBlobFieldName[MFCI_POLICY_TARGET_OEM_02]) == 0) {
    ReturnVal = mCurrentMfciTarget->Oem02;
    Size      = (StrLen (ReturnVal) + 1) * 2;
  } else if (StrCmp (VariableName, gPolicyBlobFieldName[MFCI_POLICY_TARGET_NONCE]) == 0) {
    ReturnVal = &mCurrentMfciTarget->Nonce;
    Size      = sizeof (mCurrentMfciTarget->Nonce);
  } else {
    ReturnStatus = EFI_NOT_FOUND;
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
MfciIdSupportGetManufacturer (
  OUT CHAR16  **Manufacturer,
  OUT UINTN   *ManufacturerSize  OPTIONAL
  )
{
  *Manufacturer     = AllocateCopyPool (StrLen (mCurrentMfciTarget->Manufacturer), mCurrentMfciTarget->Manufacturer);
  *ManufacturerSize = StrLen (mCurrentMfciTarget->Manufacturer);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MfciIdSupportGetProductName (
  OUT CHAR16  **ProductName,
  OUT UINTN   *ProductNameSize  OPTIONAL
  )
{
  *ProductName     = AllocateCopyPool (StrLen (mCurrentMfciTarget->Product), mCurrentMfciTarget->Product);
  *ProductNameSize = StrLen (mCurrentMfciTarget->Product);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MfciIdSupportGetSerialNumber (
  OUT CHAR16  **SerialNumber,
  OUT UINTN   *SerialNumberSize  OPTIONAL
  )
{
  *SerialNumber     = AllocateCopyPool (StrLen (mCurrentMfciTarget->SerialNum), mCurrentMfciTarget->SerialNum);
  *SerialNumberSize = StrLen (mCurrentMfciTarget->SerialNum);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MfciIdSupportGetOem1 (
  OUT CHAR16  **Oem1,
  OUT UINTN   *Oem1Size  OPTIONAL
  )
{
  *Oem1     = AllocateCopyPool (StrLen (mCurrentMfciTarget->Oem01), mCurrentMfciTarget->Oem01);
  *Oem1Size = StrLen (mCurrentMfciTarget->Oem01);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MfciIdSupportGetOem2 (
  OUT CHAR16  **Oem2,
  OUT UINTN   *Oem2Size  OPTIONAL
  )
{
  *Oem2     = AllocateCopyPool (StrLen (mCurrentMfciTarget->Oem02), mCurrentMfciTarget->Oem02);
  *Oem2Size = StrLen (mCurrentMfciTarget->Oem02);
  return EFI_SUCCESS;
}

UNIT_TEST_STATUS
EFIAPI
TargetingPrerequisite (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  mCurrentMfciTarget = Context;
  return UNIT_TEST_PASSED;
}

// Targeting is properly verified
UNIT_TEST_STATUS
EFIAPI
UnitTestVerifyTarget (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS        Status;
  MFCI_POLICY_TYPE  Policy;

  Status = VerifyTargeting (
             (VOID *)mSigned_policy_good_manufacturing,
             sizeof (mSigned_policy_good_manufacturing),
             mCurrentMfciTarget->Nonce,
             &Policy
             );

  UT_ASSERT_STATUS_EQUAL (Status, mCurrentMfciTarget->ExpectedReturn);
  if (!EFI_ERROR (Status)) {
    UT_ASSERT_EQUAL (Policy, MFCI_TEST_POLICY);
  }

  return UNIT_TEST_PASSED;
}

// It can properly notify when there is a policy change
// Others, TBD

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
  UNIT_TEST_SUITE_HANDLE      TargetVerifyPhaseSuite;

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
  // Populate the TargetVerifyPhaseSuite Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&TargetVerifyPhaseSuite, Framework, "TargetVerifyPhase", "ReportRouter.Phase", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for TargetVerifyPhaseSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (TargetVerifyPhaseSuite, "VerifyTargeting should succeed with correct target information", "VerifyTarget", UnitTestVerifyTarget, TargetingPrerequisite, NULL, &mMfciTargetContext01);
  AddTestCase (TargetVerifyPhaseSuite, "VerifyTargeting should fail with mismatched manufacturer", "VerifyManufacturer", UnitTestVerifyTarget, TargetingPrerequisite, NULL, &mMfciTargetContext02);
  AddTestCase (TargetVerifyPhaseSuite, "VerifyTargeting should fail with mismatched product", "VerifyProduct", UnitTestVerifyTarget, TargetingPrerequisite, NULL, &mMfciTargetContext03);
  AddTestCase (TargetVerifyPhaseSuite, "VerifyTargeting should fail with mismatched serial number", "VerifySerial", UnitTestVerifyTarget, TargetingPrerequisite, NULL, &mMfciTargetContext04);
  AddTestCase (TargetVerifyPhaseSuite, "VerifyTargeting should fail with mismatched OEM 01", "VerifyOEM01", UnitTestVerifyTarget, TargetingPrerequisite, NULL, &mMfciTargetContext05);
  AddTestCase (TargetVerifyPhaseSuite, "VerifyTargeting should fail with mismatched OEM 02", "VerifyOEM02", UnitTestVerifyTarget, TargetingPrerequisite, NULL, &mMfciTargetContext06);
  AddTestCase (TargetVerifyPhaseSuite, "VerifyTargeting should fail with mismatched nonce", "VerifyNonce", UnitTestVerifyTarget, TargetingPrerequisite, NULL, &mMfciTargetContext07);

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
