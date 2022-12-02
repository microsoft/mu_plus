/** @file

  Test file for MAC Address Emulation SnpSupportsMacEmuCheck.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SnpSupportsMacEmuCheckTest.h"

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
SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpHandleNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  BOOLEAN                      SupportsEmu;
  EFI_SIMPLE_NETWORK_PROTOCOL  Snp;
  UINTN                        MacContext;

  // Act
  SupportsEmu = SnpSupportsMacEmuCheck (NULL, &Snp, (MAC_EMULATION_SNP_NOTIFY_CONTEXT *)&MacContext);

  // Assert
  assert_true (SupportsEmu == FALSE);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpNull ()

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
SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  BOOLEAN                      SupportsEmu;
  EFI_HANDLE                   SnpHandle;
  EFI_SIMPLE_NETWORK_PROTOCOL  Snp;

  // Act
  SupportsEmu = SnpSupportsMacEmuCheck (&SnpHandle, &Snp, NULL);

  // Assert
  assert_true (SupportsEmu == FALSE);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SnpSupportsMacEmuCheck_ReturnsFalse_WhenContextNull ()

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
SnpSupportsMacEmuCheck_ReturnsFalse_WhenContextNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  BOOLEAN     SupportsEmu;
  EFI_HANDLE  SnpHandle;
  UINTN       MacContext;

  // Act
  SupportsEmu = SnpSupportsMacEmuCheck (&SnpHandle, NULL, (MAC_EMULATION_SNP_NOTIFY_CONTEXT *)&MacContext);

  // Assert
  assert_true (SupportsEmu == FALSE);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpNotInitialized ()

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
SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpNotInitialized (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  BOOLEAN                      SupportsEmu;
  EFI_HANDLE                   SnpHandle;
  EFI_SIMPLE_NETWORK_PROTOCOL  Snp;
  EFI_SIMPLE_NETWORK_MODE      Mode;
  UINTN                        MacContext;

  Mode.State = EfiSimpleNetworkStopped;
  Snp.Mode   = &Mode;

  // Act
  SupportsEmu = SnpSupportsMacEmuCheck (&SnpHandle, &Snp, (MAC_EMULATION_SNP_NOTIFY_CONTEXT *)&MacContext);

  // Assert
  assert_true (SupportsEmu == FALSE);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpNotEthernet ()

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
SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpNotEthernet (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  BOOLEAN                      SupportsEmu;
  EFI_HANDLE                   SnpHandle;
  EFI_SIMPLE_NETWORK_PROTOCOL  Snp;
  EFI_SIMPLE_NETWORK_MODE      Mode;
  UINTN                        MacContext;

  Mode.State  = EfiSimpleNetworkInitialized;
  Mode.IfType = (UINT8)(~NET_IFTYPE_ETHERNET);
  Snp.Mode    = &Mode;

  // Act
  SupportsEmu = SnpSupportsMacEmuCheck (&SnpHandle, &Snp, (MAC_EMULATION_SNP_NOTIFY_CONTEXT *)&MacContext);

  // Assert
  assert_true (SupportsEmu == FALSE);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpMacNotChangable ()

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
SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpMacNotChangable (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  BOOLEAN                      SupportsEmu;
  EFI_HANDLE                   SnpHandle;
  EFI_SIMPLE_NETWORK_PROTOCOL  Snp;
  EFI_SIMPLE_NETWORK_MODE      Mode;
  UINTN                        MacContext;

  Mode.State                = EfiSimpleNetworkInitialized;
  Mode.IfType               = NET_IFTYPE_ETHERNET;
  Mode.MacAddressChangeable = FALSE;
  Snp.Mode                  = &Mode;

  // Act
  SupportsEmu = SnpSupportsMacEmuCheck (&SnpHandle, &Snp, (MAC_EMULATION_SNP_NOTIFY_CONTEXT *)&MacContext);

  // Assert
  assert_true (SupportsEmu == FALSE);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SnpSupportsMacEmuCheck_ReturnsFalse_WhenPlatformCheckReturnsUnsupported ()

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
SnpSupportsMacEmuCheck_ReturnsFalse_WhenPlatformCheckReturnsUnsupported (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  BOOLEAN                      SupportsEmu;
  EFI_HANDLE                   SnpHandle;
  EFI_SIMPLE_NETWORK_PROTOCOL  Snp;
  EFI_SIMPLE_NETWORK_MODE      Mode;
  UINTN                        MacContext;

  Mode.State                = EfiSimpleNetworkInitialized;
  Mode.IfType               = NET_IFTYPE_ETHERNET;
  Mode.MacAddressChangeable = TRUE;
  Snp.Mode                  = &Mode;

  will_return (PlatformMacEmulationSnpCheck, FALSE);

  // Act
  SupportsEmu = SnpSupportsMacEmuCheck (&SnpHandle, &Snp, (MAC_EMULATION_SNP_NOTIFY_CONTEXT *)&MacContext);

  // Assert
  assert_true (SupportsEmu == FALSE);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SnpSupportsMacEmuCheck_ReturnsFalse_WhenMacAlreadyAssignedToAnotherSupportedInterface ()

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
SnpSupportsMacEmuCheck_ReturnsFalse_WhenMacAlreadyAssignedToAnotherSupportedInterface (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  BOOLEAN                           SupportsEmu;
  EFI_HANDLE                        SnpHandle;
  EFI_SIMPLE_NETWORK_PROTOCOL       Snp;
  EFI_SIMPLE_NETWORK_MODE           Mode;
  MAC_EMULATION_SNP_NOTIFY_CONTEXT  MacContext;

  MacContext.Assigned = TRUE;
  SetMem (&MacContext.PermanentAddress, NET_ETHER_ADDR_LEN, 0xBB);

  Mode.State                = EfiSimpleNetworkInitialized;
  Mode.IfType               = NET_IFTYPE_ETHERNET;
  Mode.MacAddressChangeable = TRUE;
  SetMem (&Mode.PermanentAddress, NET_ETHER_ADDR_LEN, 0xAA);
  Snp.Mode = &Mode;

  will_return (PlatformMacEmulationSnpCheck, TRUE);

  // Act
  SupportsEmu = SnpSupportsMacEmuCheck (&SnpHandle, &Snp, &MacContext);

  // Assert
  assert_true (SupportsEmu == FALSE);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SnpSupportsMacEmuCheck_ReturnsTrue_WhenInterfaceIsSupported_AndNoOtherInterfaceHasBeenAssignedYet ()

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
SnpSupportsMacEmuCheck_ReturnsTrue_WhenInterfaceIsSupported_AndNoOtherInterfaceHasBeenAssignedYet (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  BOOLEAN                           SupportsEmu;
  EFI_HANDLE                        SnpHandle;
  EFI_SIMPLE_NETWORK_PROTOCOL       Snp;
  EFI_SIMPLE_NETWORK_MODE           Mode;
  MAC_EMULATION_SNP_NOTIFY_CONTEXT  MacContext;

  MacContext.Assigned = FALSE;

  Mode.State                = EfiSimpleNetworkInitialized;
  Mode.IfType               = NET_IFTYPE_ETHERNET;
  Mode.MacAddressChangeable = TRUE;
  Snp.Mode                  = &Mode;

  will_return (PlatformMacEmulationSnpCheck, TRUE);

  // Act
  SupportsEmu = SnpSupportsMacEmuCheck (&SnpHandle, &Snp, &MacContext);

  // Assert
  assert_true (SupportsEmu == TRUE);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for SnpSupportsMacEmuCheck_ReturnsTrue_WhenInterfaceIsSupported_AndInterfaceMatchesPreviouslyAssignedInterface ()

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
SnpSupportsMacEmuCheck_ReturnsTrue_WhenInterfaceIsSupported_AndInterfaceMatchesPreviouslyAssignedInterface (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  BOOLEAN                           SupportsEmu;
  EFI_HANDLE                        SnpHandle;
  EFI_SIMPLE_NETWORK_PROTOCOL       Snp;
  EFI_SIMPLE_NETWORK_MODE           Mode;
  MAC_EMULATION_SNP_NOTIFY_CONTEXT  MacContext;

  MacContext.Assigned = TRUE;
  SetMem (&MacContext.PermanentAddress, NET_ETHER_ADDR_LEN, 0xAA);

  Mode.State                = EfiSimpleNetworkInitialized;
  Mode.IfType               = NET_IFTYPE_ETHERNET;
  Mode.MacAddressChangeable = TRUE;
  SetMem (&Mode.PermanentAddress, NET_ETHER_ADDR_LEN, 0xAA);
  Snp.Mode = &Mode;

  will_return (PlatformMacEmulationSnpCheck, TRUE);

  // Act
  SupportsEmu = SnpSupportsMacEmuCheck (&SnpHandle, &Snp, &MacContext);

  // Assert
  assert_true (SupportsEmu == TRUE);

  return UNIT_TEST_PASSED;
}

VOID
RegisterSnpSupportsMacEmuCheckTests (
  UNIT_TEST_SUITE_HANDLE  SuiteHandle
  )
{
  // Negative Test Cases
  AddTestCase (SuiteHandle, "SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpHandleNull", "SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpHandleNull", SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpHandleNull, NULL, NULL, NULL);
  AddTestCase (SuiteHandle, "SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpNull", "SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpNull", SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpNull, NULL, NULL, NULL);
  AddTestCase (SuiteHandle, "SnpSupportsMacEmuCheck_ReturnsFalse_WhenContextNull", "SnpSupportsMacEmuCheck_ReturnsFalse_WhenContextNull", SnpSupportsMacEmuCheck_ReturnsFalse_WhenContextNull, NULL, NULL, NULL);
  AddTestCase (SuiteHandle, "SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpNotInitialized", "SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpNotInitialized", SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpNotInitialized, NULL, NULL, NULL);
  AddTestCase (SuiteHandle, "SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpNotEthernet", "SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpNotEthernet", SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpNotEthernet, NULL, NULL, NULL);
  AddTestCase (SuiteHandle, "SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpMacNotChangable", "SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpMacNotChangable", SnpSupportsMacEmuCheck_ReturnsFalse_WhenSnpMacNotChangable, NULL, NULL, NULL);
  AddTestCase (SuiteHandle, "SnpSupportsMacEmuCheck_ReturnsFalse_WhenPlatformCheckReturnsUnsupported", "SnpSupportsMacEmuCheck_ReturnsFalse_WhenPlatformCheckReturnsUnsupported", SnpSupportsMacEmuCheck_ReturnsFalse_WhenPlatformCheckReturnsUnsupported, NULL, NULL, NULL);
  AddTestCase (SuiteHandle, "SnpSupportsMacEmuCheck_ReturnsFalse_WhenMacAlreadyAssignedToAnotherSupportedInterface", "SnpSupportsMacEmuCheck_ReturnsFalse_WhenMacAlreadyAssignedToAnotherSupportedInterface", SnpSupportsMacEmuCheck_ReturnsFalse_WhenMacAlreadyAssignedToAnotherSupportedInterface, NULL, NULL, NULL);

  // Positive Test Cases
  AddTestCase (SuiteHandle, "SnpSupportsMacEmuCheck_ReturnsTrue_WhenInterfaceIsSupported_AndNoOtherInterfaceHasBeenAssignedYet", "SnpSupportsMacEmuCheck_ReturnsTrue_WhenInterfaceIsSupported_AndNoOtherInterfaceHasBeenAssignedYet", SnpSupportsMacEmuCheck_ReturnsTrue_WhenInterfaceIsSupported_AndNoOtherInterfaceHasBeenAssignedYet, NULL, NULL, NULL);
  AddTestCase (SuiteHandle, "SnpSupportsMacEmuCheck_ReturnsTrue_WhenInterfaceIsSupported_AndInterfaceMatchesPreviouslyAssignedInterface", "SnpSupportsMacEmuCheck_ReturnsTrue_WhenInterfaceIsSupported_AndInterfaceMatchesPreviouslyAssignedInterface", SnpSupportsMacEmuCheck_ReturnsTrue_WhenInterfaceIsSupported_AndInterfaceMatchesPreviouslyAssignedInterface, NULL, NULL, NULL);
}
