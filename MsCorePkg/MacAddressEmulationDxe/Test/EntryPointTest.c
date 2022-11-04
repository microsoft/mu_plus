#include "SnpSupportsMacEmuCheckTest.h"

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
MacAddressEmulationEntry_ReturnsError_IfMacEmulationDisabled (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  EFI_STATUS Status;
  EFI_MAC_ADDRESS DummyAddress = {0x0};

  will_return(IsMacEmulationEnabled, &DummyAddress);
  will_return(IsMacEmulationEnabled, EFI_UNSUPPORTED);

  // Act
  Status = MacAddressEmulationEntry(NULL, NULL);

  // Assert
  assert_true(EFI_ERROR(Status));

  return UNIT_TEST_PASSED;
}

int
CheckEfiNamedEventListenInputs (
  const LargestIntegralType value,
  const LargestIntegralType check_value
  )
{
  MAC_EMULATION_SNP_NOTIFY_CONTEXT *Actual, *Expected;

  Actual = (MAC_EMULATION_SNP_NOTIFY_CONTEXT*) value;
  Expected = (MAC_EMULATION_SNP_NOTIFY_CONTEXT*) check_value;

  assert_true(Expected->Assigned == Actual->Assigned);
  assert_memory_equal(&Actual->EmulationAddress, &Expected->EmulationAddress, NET_ETHER_ADDR_LEN);

  return 1;
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
MacAddressEmulationEntry_EnablesHighLevelOsDriverAndRegistersCallback_WhenEmulationEnabled (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Arrange
  EFI_STATUS Status;
  EFI_MAC_ADDRESS AddressToEmulate;
  MAC_EMULATION_SNP_NOTIFY_CONTEXT ExpectedContext;

  ExpectedContext.Assigned = FALSE;
  SetMem(&ExpectedContext.EmulationAddress, NET_ETHER_ADDR_LEN, 0xEE);

  SetMem(&AddressToEmulate, NET_ETHER_ADDR_LEN, 0xEE);

  will_return(IsMacEmulationEnabled, &AddressToEmulate);
  will_return(IsMacEmulationEnabled, TRUE);

  will_return(PlatformMacEmulationEnable, EFI_SUCCESS);

  expect_check(EfiNamedEventListen, NotifyContext, CheckEfiNamedEventListenInputs, &ExpectedContext);
  will_return(EfiNamedEventListen, EFI_SUCCESS);

  // Act
  Status = MacAddressEmulationEntry(NULL, NULL);

  // Assert
  assert_true(!EFI_ERROR(Status));

  return UNIT_TEST_PASSED;
}

VOID
RegisterEntryPointTests (
  UNIT_TEST_SUITE_HANDLE SuiteHandle
  )
{
  AddTestCase (SuiteHandle, "MacAddressEmulationEntry_ReturnsError_IfMacEmulationDisabled", "MacAddressEmulationEntry_ReturnsError_IfMacEmulationDisabled", MacAddressEmulationEntry_ReturnsError_IfMacEmulationDisabled, NULL, NULL, NULL);
  AddTestCase (SuiteHandle, "MacAddressEmulationEntry_EnablesHighLevelOsDriverAndRegistersCallback_WhenEmulationEnabled", "MacAddressEmulationEntry_EnablesHighLevelOsDriverAndRegistersCallback_WhenEmulationEnabled", MacAddressEmulationEntry_EnablesHighLevelOsDriverAndRegistersCallback_WhenEmulationEnabled, NULL, NULL, NULL);

}