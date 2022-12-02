/** @file
  This module tests MacAddressEmulation behavior for
  MacAddressEmulationDxe driver.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "MacAddressEmulationDxe.h"

#include "SnpSupportsMacEmuCheckTest.h"
#include "FindMatchingSnpTest.h"
#include "SimpleNetworkProtocolNotifyTest.h"
#include "EntryPointTest.h"

#define UNIT_TEST_NAME     "Mac Address Emulation Dxe Host Test"
#define UNIT_TEST_VERSION  "0.1"

EFI_RUNTIME_SERVICES  mMockRuntime;
EFI_BOOT_SERVICES mBootServices;

EFI_STATUS
IsMacEmulationEnabled (
  OUT EFI_MAC_ADDRESS *Address
  )
{
  CopyMem(Address, (EFI_MAC_ADDRESS*)mock(), NET_ETHER_ADDR_LEN);
  return (EFI_STATUS)mock();
}

BOOLEAN
PlatformMacEmulationSnpCheck (
  IN  EFI_HANDLE SnpHandle
  )
{
  return (BOOLEAN)mock();
}

EFI_STATUS
PlatformMacEmulationEnable (
  IN  EFI_MAC_ADDRESS *Address
  )
{
  return (EFI_STATUS)mock();
}

EFI_STATUS
EFIAPI
EfiNamedEventListen (
  IN CONST EFI_GUID    *Name,
  IN EFI_TPL           NotifyTpl,
  IN EFI_EVENT_NOTIFY  NotifyFunction,
  IN CONST VOID        *NotifyContext   OPTIONAL,
  OUT VOID             *Registration OPTIONAL
  )
{
  check_expected_ptr(NotifyContext);
  return (EFI_STATUS)mock();
}

EFI_STATUS
EFIAPI 
LocateHandleBuffer (
  IN     EFI_LOCATE_SEARCH_TYPE       SearchType,
  IN     EFI_GUID                     *Protocol       OPTIONAL,
  IN     VOID                         *SearchKey      OPTIONAL,
  OUT    UINTN                        *NoHandles,
  OUT    EFI_HANDLE                   **Buffer
  )
{
  *NoHandles = (UINTN)mock();
  *Buffer = (EFI_HANDLE*)mock();
  return (EFI_STATUS)mock();
}

EFI_STATUS
EFIAPI
HandleProtocol (
  IN  EFI_HANDLE               Handle,
  IN  EFI_GUID                 *Protocol,
  OUT VOID                     **Interface
  )
{
  *Interface = (VOID*)mock();
  return (EFI_STATUS)mock();
}

VOID
EFIAPI 
RestoreTPL (
  IN EFI_TPL      OldTpl
  )
{
  return;
}

EFI_TPL
EFIAPI 
RaiseTPL (
  IN EFI_TPL      NewTpl
  )
{
  return TPL_CALLBACK;
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
  UNIT_TEST_SUITE_HANDLE      TestSuite;

  Framework = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));

  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
  }

  if (!EFI_ERROR (Status)) {
    Status = CreateUnitTestSuite (&TestSuite, Framework, "TargetVerifyPhase", "ReportRouter.Phase", NULL, NULL);
  }
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for TestSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
  }

  if (!EFI_ERROR (Status)) {
    RegisterSnpSupportsMacEmuCheckTests(TestSuite);
    RegisterFindMatchingSnpTests(TestSuite);
    RegisterSimpleNetworkProtocolNotifyTests(TestSuite);
    RegisterEntryPointTests(TestSuite);

    Status = RunAllTestSuites (Framework);
  }

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
