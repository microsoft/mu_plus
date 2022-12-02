/** @file

  Test file for MAC Address Emulation FindMatchingSnp.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SnpSupportsMacEmuCheckTest.h"

/**
  Unit test for FindMatchingSnp_ReturnsNull_WhenMatchFunctionNull ()

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
FindMatchingSnp_ReturnsNull_WhenMatchFunctionNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  UINTN                        MacContext;
  EFI_SIMPLE_NETWORK_PROTOCOL  *SnpMatch;

  // Act
  SnpMatch = FindMatchingSnp (NULL, (MAC_EMULATION_SNP_NOTIFY_CONTEXT*) &MacContext);

  // Assert
  assert_null (SnpMatch);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpHandleNull ()

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
FindMatchingSnp_Asserts_WhenLocateHandleBufferFails (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  UINTN  MacContext;

  will_return (LocateHandleBuffer, 0);
  will_return (LocateHandleBuffer, NULL);
  will_return (LocateHandleBuffer, EFI_ACCESS_DENIED);

  // Act
  // Assert
  UT_EXPECT_ASSERT_FAILURE (FindMatchingSnp (SnpSupportsMacEmuCheck, &MacContext), NULL);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for FindMatchingSnp_ReturnsNull_WhenNoHandlesAreSupported ()

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
FindMatchingSnp_ReturnsNull_WhenNoHandlesAreSupported (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  MAC_EMULATION_SNP_NOTIFY_CONTEXT  MacContext;
  EFI_HANDLE                        *DummyHandle;
  EFI_SIMPLE_NETWORK_PROTOCOL       ExpectedMatch;
  EFI_SIMPLE_NETWORK_PROTOCOL       *ActualMatch;

  EFI_SIMPLE_NETWORK_MODE  Mode;

  Mode.State         = EfiSimpleNetworkStopped;
  ExpectedMatch.Mode = &Mode;

  size_t  sizeHandle = sizeof (*DummyHandle);

  DummyHandle = AllocateZeroPool (sizeHandle);
  will_return (LocateHandleBuffer, 1);
  will_return (LocateHandleBuffer, DummyHandle);
  will_return (LocateHandleBuffer, EFI_SUCCESS);

  will_return (HandleProtocol, &ExpectedMatch);
  will_return (HandleProtocol, EFI_SUCCESS);

  // Act
  ActualMatch = FindMatchingSnp (SnpSupportsMacEmuCheck, &MacContext);

  // Assert
  assert_null (ActualMatch);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for FindMatchingSnp_ReturnsPreviouslyAssignedSnp_WhenMultipleSnpSupportMacEmu ()

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
FindMatchingSnp_ReturnsPreviouslyAssignedSnp_WhenMultipleSnpSupportMacEmu (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  MAC_EMULATION_SNP_NOTIFY_CONTEXT  MacContext;
  EFI_HANDLE                        *DummyHandle;
  EFI_SIMPLE_NETWORK_PROTOCOL       ExpectedMatch;
  EFI_SIMPLE_NETWORK_PROTOCOL       *ActualMatch;

  EFI_SIMPLE_NETWORK_MODE  Mode;

  MacContext.Assigned = TRUE;
  SetMem (&MacContext.PermanentAddress, NET_ETHER_ADDR_LEN, 0xAA);

  Mode.State                = EfiSimpleNetworkInitialized;
  Mode.IfType               = NET_IFTYPE_ETHERNET;
  Mode.MacAddressChangeable = TRUE;
  SetMem (&Mode.PermanentAddress, NET_ETHER_ADDR_LEN, 0xAA);
  ExpectedMatch.Mode = &Mode;

  will_return (PlatformMacEmulationSnpCheck, TRUE);

  DummyHandle = AllocateZeroPool (sizeof (*DummyHandle));
  SetMem (DummyHandle, sizeof (*DummyHandle), 0x11);
  will_return (LocateHandleBuffer, 2);
  will_return (LocateHandleBuffer, DummyHandle);
  will_return (LocateHandleBuffer, EFI_SUCCESS);

  will_return (HandleProtocol, &ExpectedMatch);
  will_return (HandleProtocol, EFI_SUCCESS);

  // Act
  ActualMatch = FindMatchingSnp (SnpSupportsMacEmuCheck, &MacContext);

  // Assert
  assert_ptr_equal (&ExpectedMatch, ActualMatch);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
FindMatchingSnpTestSetup (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  gBS                     = &mBootServices;
  gBS->LocateHandleBuffer = LocateHandleBuffer;
  gBS->HandleProtocol     = HandleProtocol;

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
FindMatchingSnpTestTeardown (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return UNIT_TEST_PASSED;
}

VOID
RegisterFindMatchingSnpTests (
  UNIT_TEST_SUITE_HANDLE  SuiteHandle
  )
{
  // Negative Test Cases
  AddTestCase (SuiteHandle, "FindMatchingSnp_ReturnsNull_WhenMatchFunctionNull", "FindMatchingSnp_ReturnsNull_WhenMatchFunctionNull", FindMatchingSnp_ReturnsNull_WhenMatchFunctionNull, FindMatchingSnpTestSetup, FindMatchingSnpTestTeardown, NULL);
  AddTestCase (SuiteHandle, "FindMatchingSnp_Asserts_WhenLocateHandleBufferFails", "FindMatchingSnp_Asserts_WhenLocateHandleBufferFails", FindMatchingSnp_Asserts_WhenLocateHandleBufferFails, FindMatchingSnpTestSetup, FindMatchingSnpTestTeardown, NULL);
  AddTestCase (SuiteHandle, "FindMatchingSnp_ReturnsNull_WhenNoHandlesAreSupported", "FindMatchingSnp_ReturnsNull_WhenNoHandlesAreSupported", FindMatchingSnp_ReturnsNull_WhenNoHandlesAreSupported, FindMatchingSnpTestSetup, FindMatchingSnpTestTeardown, NULL);

  // Positive Test Cases
  AddTestCase (SuiteHandle, "FindMatchingSnp_ReturnsPreviouslyAssignedSnp_WhenMultipleSnpSupportMacEmu", "FindMatchingSnp_ReturnsPreviouslyAssignedSnp_WhenMultipleSnpSupportMacEmu", FindMatchingSnp_ReturnsPreviouslyAssignedSnp_WhenMultipleSnpSupportMacEmu, FindMatchingSnpTestSetup, FindMatchingSnpTestTeardown, NULL);
}
