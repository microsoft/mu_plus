/** @file

  Test file for MAC Address Emulation SimpleNetworkProtocolNotifyTest.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SimpleNetworkProtocolNotifyTest.h"

EFI_STATUS
EFIAPI
SetStationAddress (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      Reset,
  IN EFI_MAC_ADDRESS              *New OPTIONAL
  )
{
  CopyMem (&(This->Mode->CurrentAddress), New, sizeof (EFI_MAC_ADDRESS));
  return EFI_SUCCESS;
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
SimpleNetworkProtocolNotify_AssignsMacToFirstSupportedSnp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  MAC_EMULATION_SNP_NOTIFY_CONTEXT  MacContext;
  EFI_HANDLE                        *DummyHandleArr;
  EFI_SIMPLE_NETWORK_PROTOCOL       Snp;

  EFI_SIMPLE_NETWORK_MODE  Mode;

  MacContext.Assigned = FALSE;
  SetMem (&MacContext.EmulationAddress, NET_ETHER_ADDR_LEN, 0xEE);

  Mode.State                = EfiSimpleNetworkInitialized;
  Mode.IfType               = NET_IFTYPE_ETHERNET;
  Mode.MacAddressChangeable = TRUE;
  SetMem (&Mode.PermanentAddress, NET_ETHER_ADDR_LEN, 0xAA);
  Snp.Mode           = &Mode;
  Snp.StationAddress = SetStationAddress;

  will_return (PlatformMacEmulationSnpCheck, TRUE);

  DummyHandleArr = AllocateZeroPool (sizeof (EFI_HANDLE *)*2);
  EFI_HANDLE  DummyHandle1 = AllocateZeroPool (sizeof (EFI_HANDLE));
  EFI_HANDLE  DummyHandle2 = AllocateZeroPool (sizeof (EFI_HANDLE));

  DummyHandleArr[0] = DummyHandle1;
  DummyHandleArr[1] = DummyHandle2;
  will_return (LocateHandleBuffer, 2);
  will_return (LocateHandleBuffer, DummyHandleArr);
  will_return (LocateHandleBuffer, EFI_SUCCESS);

  will_return (HandleProtocol, &Snp);
  will_return (HandleProtocol, EFI_SUCCESS);

  // Act
  SimpleNetworkProtocolNotify (NULL, (VOID *)&MacContext);

  // Assert
  assert_true (MacContext.Assigned == TRUE);
  assert_memory_equal (&Mode.PermanentAddress, &MacContext.PermanentAddress, NET_ETHER_ADDR_LEN);
  assert_memory_equal (&Mode.CurrentAddress, &MacContext.EmulationAddress, NET_ETHER_ADDR_LEN);

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
SimpleNetworkProtocolNotify_AssignsMacToOnlySameSnpAsPreviously (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  MAC_EMULATION_SNP_NOTIFY_CONTEXT  MacContext;
  EFI_HANDLE                        *DummyHandleArr;
  EFI_SIMPLE_NETWORK_MODE           Mode1;
  EFI_SIMPLE_NETWORK_MODE           Mode2;
  EFI_SIMPLE_NETWORK_PROTOCOL       Snp1;
  EFI_SIMPLE_NETWORK_PROTOCOL       Snp2;

  MacContext.Assigned = TRUE;
  SetMem (&MacContext.EmulationAddress, NET_ETHER_ADDR_LEN, 0xEE);
  SetMem (&MacContext.PermanentAddress, NET_ETHER_ADDR_LEN, 0xAA);

  Mode1.State                = EfiSimpleNetworkInitialized;
  Mode1.IfType               = NET_IFTYPE_ETHERNET;
  Mode1.MacAddressChangeable = TRUE;
  SetMem (&Mode1.PermanentAddress, NET_ETHER_ADDR_LEN, 0xBB);
  SetMem (&Mode1.CurrentAddress, NET_ETHER_ADDR_LEN, 0x00);
  Snp1.Mode           = &Mode1;
  Snp1.StationAddress = SetStationAddress;

  Mode2.State                = EfiSimpleNetworkInitialized;
  Mode2.IfType               = NET_IFTYPE_ETHERNET;
  Mode2.MacAddressChangeable = TRUE;
  SetMem (&Mode2.PermanentAddress, NET_ETHER_ADDR_LEN, 0xAA);
  SetMem (&Mode2.CurrentAddress, NET_ETHER_ADDR_LEN, 0x00);
  Snp2.Mode           = &Mode2;
  Snp2.StationAddress = SetStationAddress;

  will_return (HandleProtocol, &Snp1);
  will_return (HandleProtocol, EFI_SUCCESS);

  will_return (HandleProtocol, &Snp2);
  will_return (HandleProtocol, EFI_SUCCESS);

  will_return_always (PlatformMacEmulationSnpCheck, TRUE);

  DummyHandleArr = AllocateZeroPool (sizeof (EFI_HANDLE *)*2);
  EFI_HANDLE  DummyHandle1 = AllocateZeroPool (sizeof (EFI_HANDLE));
  EFI_HANDLE  DummyHandle2 = AllocateZeroPool (sizeof (EFI_HANDLE));

  DummyHandleArr[0] = DummyHandle1;
  DummyHandleArr[1] = DummyHandle2;

  will_return (LocateHandleBuffer, 2);
  will_return (LocateHandleBuffer, DummyHandleArr);
  will_return (LocateHandleBuffer, EFI_SUCCESS);

  // Act
  SimpleNetworkProtocolNotify (NULL, (VOID *)&MacContext);

  // Assert
  assert_memory_not_equal (&Mode1.CurrentAddress, &MacContext.EmulationAddress, NET_ETHER_ADDR_LEN);
  assert_memory_equal (&Mode2.PermanentAddress, &MacContext.PermanentAddress, NET_ETHER_ADDR_LEN);
  assert_memory_equal (&Mode2.CurrentAddress, &MacContext.EmulationAddress, NET_ETHER_ADDR_LEN);

  FreePool (DummyHandle1);
  FreePool (DummyHandle2);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
SimpleNetworkProtocolNotifyTestSetup (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  gBS                     = &mBootServices;
  gBS->LocateHandleBuffer = LocateHandleBuffer;
  gBS->HandleProtocol     = HandleProtocol;
  gBS->RaiseTPL           = RaiseTPL;
  gBS->RestoreTPL         = RestoreTPL;

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
SimpleNetworkProtocolNotifyTestTeardown (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return UNIT_TEST_PASSED;
}

VOID
RegisterSimpleNetworkProtocolNotifyTests (
  UNIT_TEST_SUITE_HANDLE  SuiteHandle
  )
{
  AddTestCase (SuiteHandle, "SimpleNetworkProtocolNotify_AssignsMacToFirstSupportedSnp", "SimpleNetworkProtocolNotify_AssignsMacToFirstSupportedSnp", SimpleNetworkProtocolNotify_AssignsMacToFirstSupportedSnp, SimpleNetworkProtocolNotifyTestSetup, SimpleNetworkProtocolNotifyTestTeardown, NULL);
  AddTestCase (SuiteHandle, "SimpleNetworkProtocolNotify_AssignsMacToOnlySameSnpAsPreviously", "SimpleNetworkProtocolNotify_AssignsMacToOnlySameSnpAsPreviously", SimpleNetworkProtocolNotify_AssignsMacToOnlySameSnpAsPreviously, SimpleNetworkProtocolNotifyTestSetup, SimpleNetworkProtocolNotifyTestTeardown, NULL);
}
