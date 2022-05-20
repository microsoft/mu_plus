/** @file
  This module tests public interface install, uninstall, policy change
  callback registration and clean up for the MfciDxe driver.

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
#include <Protocol/MfciProtocol.h>
#include <Protocol/MfciPolicyChangeNotify.h>
#include <Guid/MuVarPolicyFoundationDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UnitTestLib.h>

#include "../MfciDxe.h"

#define UNIT_TEST_NAME     "Mfci Public Interface Host Test"
#define UNIT_TEST_VERSION  "0.1"

extern MFCI_PROTOCOL  gMfciProtocol;
MFCI_POLICY_TYPE      mCurrentPolicy;
LIST_ENTRY            CreatedEvents = INITIALIZE_LIST_HEAD_VARIABLE (CreatedEvents);

typedef struct {
  LIST_ENTRY          Link;
  EFI_EVENT_NOTIFY    NotifyFunction;
  VOID                *NotifyContext;
  EFI_EVENT           Event;
} CREATE_EVENT_INFO;

EFI_STATUS
EFIAPI
InternalRegisterMfciPolicyChangeNotifyCallback (
  IN CONST MFCI_PROTOCOL          *This,
  IN MFCI_POLICY_CHANGE_CALLBACK  Callback
  );

MFCI_POLICY_TYPE
EFIAPI
InternalGetMfciPolicy (
  IN CONST MFCI_PROTOCOL  *This
  );

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
  assert_memory_equal (VariableName, END_OF_DXE_INDICATOR_VAR_NAME, sizeof (END_OF_DXE_INDICATOR_VAR_NAME));
  assert_ptr_equal (VendorGuid, &gMuVarPolicyDxePhaseGuid);
  assert_non_null (DataSize);
  assert_int_equal (*DataSize, sizeof (PHASE_INDICATOR));
  assert_non_null (Data);
  assert_non_null (Attributes);

  *Attributes                = (UINT32)mock ();
  *((PHASE_INDICATOR *)Data) = (PHASE_INDICATOR)mock ();

  return (EFI_STATUS)mock ();
}

EFI_RUNTIME_SERVICES  mMockRuntime = {
  .GetVariable = UnitTestGetVariable,
};

EFI_STATUS
EFIAPI
UnitTestInstallProtocol (
  IN OUT EFI_HANDLE          *Handle,
  IN     EFI_GUID            *Protocol,
  IN     EFI_INTERFACE_TYPE  InterfaceType,
  IN     VOID                *Interface
  )
{
  DEBUG ((DEBUG_INFO, "%a Installing %g\n", __FUNCTION__, Protocol));
  assert_non_null (Interface);
  assert_non_null (Handle);
  assert_int_equal (InterfaceType, EFI_NATIVE_INTERFACE);
  check_expected_ptr (Protocol);
  *Handle = (EFI_HANDLE)mock ();

  return (EFI_STATUS)mock ();
}

EFI_STATUS
EFIAPI
UnitTestUninstallProtocol (
  IN EFI_HANDLE  Handle,
  IN EFI_GUID    *Protocol,
  IN VOID        *Interface
  )
{
  assert_non_null (Interface);
  check_expected_ptr (Protocol);
  check_expected (Handle);

  return (EFI_STATUS)mock ();
}

EFI_STATUS
EFIAPI
UnitTestLocateHandleBuffer (
  IN     EFI_LOCATE_SEARCH_TYPE  SearchType,
  IN     EFI_GUID                *Protocol       OPTIONAL,
  IN     VOID                    *SearchKey      OPTIONAL,
  OUT    UINTN                   *NoHandles,
  OUT    EFI_HANDLE              **Buffer
  )
{
  EFI_HANDLE  *ret_buf;

  DEBUG ((DEBUG_INFO, "%a - %g\n", __FUNCTION__, Protocol));
  // Check that this is the right protocol being located
  check_expected_ptr (Protocol);

  assert_non_null (NoHandles);
  assert_non_null (Buffer);

  // Set the protocol to one of our mock protocols
  *NoHandles = (UINTN)mock ();
  ret_buf    = (EFI_HANDLE *)mock ();
  *Buffer    = AllocateCopyPool (*NoHandles * sizeof (EFI_HANDLE), ret_buf);

  return (EFI_STATUS)mock ();
}

EFI_STATUS
EFIAPI
UnitTestHandleProtocol (
  IN  EFI_HANDLE  Handle,
  IN  EFI_GUID    *Protocol,
  OUT VOID        **Interface
  )
{
  assert_non_null (Interface);
  check_expected_ptr (Protocol);
  check_expected (Handle);

  *Interface = (VOID *)mock ();

  return (EFI_STATUS)mock ();
}

EFI_STATUS
EFIAPI
UnitTestCreateEventEx (
  IN       UINT32            Type,
  IN       EFI_TPL           NotifyTpl,
  IN       EFI_EVENT_NOTIFY  NotifyFunction OPTIONAL,
  IN CONST VOID              *NotifyContext OPTIONAL,
  IN CONST EFI_GUID          *EventGroup    OPTIONAL,
  OUT      EFI_EVENT         *Event
  )
{
  CREATE_EVENT_INFO  *CacheEvent;

  assert_int_equal (Type, EVT_NOTIFY_SIGNAL);
  assert_int_equal (NotifyTpl, TPL_CALLBACK);
  assert_non_null (NotifyFunction);
  assert_non_null (NotifyContext);
  assert_ptr_equal (EventGroup, &gEfiEndOfDxeEventGroupGuid);

  // Group all temporarily allocated buffer into a linked list, they will be frees at ready to lock event
  CacheEvent = AllocateZeroPool (sizeof (CREATE_EVENT_INFO));
  assert_non_null (CacheEvent);

  *Event                     = (EFI_EVENT)NotifyFunction;
  CacheEvent->NotifyFunction = NotifyFunction;
  CacheEvent->NotifyContext  = (VOID *)NotifyContext;
  CacheEvent->Event          = *Event;
  InsertTailList (&CreatedEvents, &CacheEvent->Link);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UnitTestCloseEvent (
  IN EFI_EVENT  Event
  )
{
  assert_non_null (Event);
  return EFI_SUCCESS;
}

EFI_BOOT_SERVICES  mBootSvc = {
  .InstallProtocolInterface   = UnitTestInstallProtocol,
  .UninstallProtocolInterface = UnitTestUninstallProtocol,
  .LocateHandleBuffer         = UnitTestLocateHandleBuffer,
  .HandleProtocol             = UnitTestHandleProtocol,
  .CreateEventEx              = UnitTestCreateEventEx,
  .CloseEvent                 = UnitTestCloseEvent,
};

EFI_BOOT_SERVICES  *gBS = &mBootSvc;

VOID
EFIAPI
InterfaceCleanup (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  CREATE_EVENT_INFO  *EventInfo;
  LIST_ENTRY         *Link;

  if (IsListEmpty (&CreatedEvents)) {
    return;
  }

  //
  // Iterate through each node in the list and free all the referenced memory block referenced and the node itself.
  //
  Link = CreatedEvents.ForwardLink;
  while (Link != &CreatedEvents) {
    EventInfo = BASE_CR (Link, CREATE_EVENT_INFO, Link);

    Link = RemoveEntryList (Link);
    // Moved iterator, good to free the node itself
    FreePool (EventInfo);
  }
}

EFI_STATUS
EFIAPI
UnitTestMfciCallback (
  IN CONST MFCI_POLICY_TYPE  NewPolicy,
  IN CONST MFCI_POLICY_TYPE  PreviousPolicy
  )
{
  check_expected (NewPolicy);
  check_expected (PreviousPolicy);

  return EFI_SUCCESS;
}

// Verified on a MFCI protocol install is complete
UNIT_TEST_STATUS
EFIAPI
UnitTestInitProtocol (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  expect_value (UnitTestInstallProtocol, Protocol, &gMfciProtocolGuid);
  will_return (UnitTestInstallProtocol, NULL);
  will_return (UnitTestInstallProtocol, EFI_SUCCESS);

  Status = InitPublicInterface ();
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

// Verified on a MFCI policy change callback registration
UNIT_TEST_STATUS
EFIAPI
UnitTestRegisterNotify (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  will_return (UnitTestGetVariable, DXE_PHASE_INDICATOR_ATTR);
  will_return (UnitTestGetVariable, FALSE);
  will_return (UnitTestGetVariable, EFI_NOT_FOUND);

  expect_value (UnitTestInstallProtocol, Protocol, &gMfciPolicyChangeNotifyProtocolGuid);
  will_return (UnitTestInstallProtocol, NULL);
  will_return (UnitTestInstallProtocol, EFI_SUCCESS);

  Status = InternalRegisterMfciPolicyChangeNotifyCallback (NULL, UnitTestMfciCallback);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

// Verified on a MFCI policy change callback registration after End of DXE
UNIT_TEST_STATUS
EFIAPI
UnitTestRegisterAfterDxe (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  will_return (UnitTestGetVariable, DXE_PHASE_INDICATOR_ATTR);
  will_return (UnitTestGetVariable, TRUE);
  will_return (UnitTestGetVariable, EFI_SUCCESS);

  Status = InternalRegisterMfciPolicyChangeNotifyCallback (NULL, UnitTestMfciCallback);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ALREADY_STARTED);

  return UNIT_TEST_PASSED;
}

// Verified on a MFCI policy change callback registration after End of DXE
UNIT_TEST_STATUS
EFIAPI
UnitTestRegisterNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = InternalRegisterMfciPolicyChangeNotifyCallback (NULL, NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

// Verified MFCI policy change notify with 0 registrations
UNIT_TEST_STATUS
EFIAPI
UnitTestNotifyChangeNone (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  expect_value (UnitTestLocateHandleBuffer, Protocol, &gMfciPolicyChangeNotifyProtocolGuid);
  will_return (UnitTestLocateHandleBuffer, 0);
  will_return (UnitTestLocateHandleBuffer, NULL);
  will_return (UnitTestLocateHandleBuffer, EFI_NOT_FOUND);

  Status = NotifyMfciPolicyChange (STD_ACTION_SECURE_BOOT_CLEAR);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

  return UNIT_TEST_PASSED;
}

// Verified MFCI policy change notify with multiple registrations
UNIT_TEST_STATUS
EFIAPI
UnitTestNotifyChangeOne (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          Handle         = (VOID *)UnitTestNotifyChangeOne;
  MFCI_POLICY_CHANGE_NOTIFY_PROTOCOL  NotifyProtocol = {
    .Callback = UnitTestMfciCallback
  };

  expect_value (UnitTestLocateHandleBuffer, Protocol, &gMfciPolicyChangeNotifyProtocolGuid);
  will_return (UnitTestLocateHandleBuffer, 1);
  will_return (UnitTestLocateHandleBuffer, &Handle);
  will_return (UnitTestLocateHandleBuffer, EFI_SUCCESS);

  expect_value (UnitTestHandleProtocol, Handle, UnitTestNotifyChangeOne);
  expect_value (UnitTestHandleProtocol, Protocol, &gMfciPolicyChangeNotifyProtocolGuid);
  will_return (UnitTestHandleProtocol, &NotifyProtocol);
  will_return (UnitTestHandleProtocol, EFI_SUCCESS);

  expect_value (UnitTestMfciCallback, NewPolicy, STD_ACTION_SECURE_BOOT_CLEAR);
  expect_value (UnitTestMfciCallback, PreviousPolicy, mCurrentPolicy);

  Status = NotifyMfciPolicyChange (STD_ACTION_SECURE_BOOT_CLEAR);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

// Verified MFCI policy change notify with multiple registrations
UNIT_TEST_STATUS
EFIAPI
UnitTestNotifyChangeMultiple (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          Handle[2]      = { (VOID *)UnitTestNotifyChangeOne, (VOID *)UnitTestNotifyChangeMultiple };
  MFCI_POLICY_CHANGE_NOTIFY_PROTOCOL  NotifyProtocol = {
    .Callback = UnitTestMfciCallback
  };

  expect_value (UnitTestLocateHandleBuffer, Protocol, &gMfciPolicyChangeNotifyProtocolGuid);
  will_return (UnitTestLocateHandleBuffer, 2);
  will_return (UnitTestLocateHandleBuffer, &Handle);
  will_return (UnitTestLocateHandleBuffer, EFI_SUCCESS);

  expect_value (UnitTestHandleProtocol, Handle, Handle[0]);
  expect_value (UnitTestHandleProtocol, Protocol, &gMfciPolicyChangeNotifyProtocolGuid);
  will_return (UnitTestHandleProtocol, &NotifyProtocol);
  will_return (UnitTestHandleProtocol, EFI_SUCCESS);

  expect_value (UnitTestMfciCallback, NewPolicy, STD_ACTION_SECURE_BOOT_CLEAR);
  expect_value (UnitTestMfciCallback, PreviousPolicy, mCurrentPolicy);

  expect_value (UnitTestHandleProtocol, Handle, Handle[1]);
  expect_value (UnitTestHandleProtocol, Protocol, &gMfciPolicyChangeNotifyProtocolGuid);
  will_return (UnitTestHandleProtocol, &NotifyProtocol);
  will_return (UnitTestHandleProtocol, EFI_SUCCESS);

  expect_value (UnitTestMfciCallback, NewPolicy, STD_ACTION_SECURE_BOOT_CLEAR);
  expect_value (UnitTestMfciCallback, PreviousPolicy, mCurrentPolicy);

  Status = NotifyMfciPolicyChange (STD_ACTION_SECURE_BOOT_CLEAR);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

// Verified MFCI get current policy
UNIT_TEST_STATUS
EFIAPI
UnitTestGetPolicy (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MFCI_POLICY_TYPE  Policy;

  Policy = InternalGetMfciPolicy (&gMfciProtocol);
  UT_ASSERT_EQUAL (Policy, mCurrentPolicy);

  return UNIT_TEST_PASSED;
}

// Verified MFCI policy clean one registration
UNIT_TEST_STATUS
EFIAPI
UnitTestCleanRegistered (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS         Status;
  CREATE_EVENT_INFO  *EventInfo;
  LIST_ENTRY         *Entry;
  EFI_HANDLE         tHandle = NULL;

  // First register a callback
  will_return (UnitTestGetVariable, DXE_PHASE_INDICATOR_ATTR);
  will_return (UnitTestGetVariable, FALSE);
  will_return (UnitTestGetVariable, EFI_NOT_FOUND);

  expect_value (UnitTestInstallProtocol, Protocol, &gMfciPolicyChangeNotifyProtocolGuid);
  will_return (UnitTestInstallProtocol, tHandle);
  will_return (UnitTestInstallProtocol, EFI_SUCCESS);

  Status = InternalRegisterMfciPolicyChangeNotifyCallback (NULL, UnitTestMfciCallback);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  expect_value (UnitTestUninstallProtocol, Protocol, &gMfciPolicyChangeNotifyProtocolGuid);
  expect_value (UnitTestUninstallProtocol, Handle, tHandle);
  will_return (UnitTestUninstallProtocol, EFI_SUCCESS);

  // Mimic invoking all notify functions when the event fires
  BASE_LIST_FOR_EACH (Entry, &CreatedEvents) {
    EventInfo = BASE_CR (Entry, CREATE_EVENT_INFO, Link);
    EventInfo->NotifyFunction (EventInfo->Event, EventInfo->NotifyContext);
  }

  return UNIT_TEST_PASSED;
}

// Verified MFCI policy clean all registrations
UNIT_TEST_STATUS
EFIAPI
UnitTestCleanAllRegistered (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS         Status;
  CREATE_EVENT_INFO  *EventInfo;
  LIST_ENTRY         *Entry;
  EFI_HANDLE         pHandle = NULL;
  EFI_HANDLE         sHandle = NULL;

  // First register a callback
  will_return (UnitTestGetVariable, DXE_PHASE_INDICATOR_ATTR);
  will_return (UnitTestGetVariable, FALSE);
  will_return (UnitTestGetVariable, EFI_NOT_FOUND);

  expect_value (UnitTestInstallProtocol, Protocol, &gMfciPolicyChangeNotifyProtocolGuid);
  will_return (UnitTestInstallProtocol, pHandle);
  will_return (UnitTestInstallProtocol, EFI_SUCCESS);

  Status = InternalRegisterMfciPolicyChangeNotifyCallback (NULL, UnitTestMfciCallback);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // Then register another callback
  will_return (UnitTestGetVariable, DXE_PHASE_INDICATOR_ATTR);
  will_return (UnitTestGetVariable, FALSE);
  will_return (UnitTestGetVariable, EFI_NOT_FOUND);

  expect_value (UnitTestInstallProtocol, Protocol, &gMfciPolicyChangeNotifyProtocolGuid);
  will_return (UnitTestInstallProtocol, sHandle);
  will_return (UnitTestInstallProtocol, EFI_SUCCESS);

  Status = InternalRegisterMfciPolicyChangeNotifyCallback (NULL, UnitTestMfciCallback);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // Specifically expect the uninstall twice in order
  expect_value (UnitTestUninstallProtocol, Protocol, &gMfciPolicyChangeNotifyProtocolGuid);
  expect_value (UnitTestUninstallProtocol, Handle, pHandle);
  will_return (UnitTestUninstallProtocol, EFI_SUCCESS);

  expect_value (UnitTestUninstallProtocol, Protocol, &gMfciPolicyChangeNotifyProtocolGuid);
  expect_value (UnitTestUninstallProtocol, Handle, sHandle);
  will_return (UnitTestUninstallProtocol, EFI_SUCCESS);

  // Mimic invoking all notify functions when the event fires
  BASE_LIST_FOR_EACH (Entry, &CreatedEvents) {
    EventInfo = BASE_CR (Entry, CREATE_EVENT_INFO, Link);
    EventInfo->NotifyFunction (EventInfo->Event, EventInfo->NotifyContext);
  }

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
  UNIT_TEST_SUITE_HANDLE      PublicInterfaceSuite;

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
  // Populate the PublicInterfaceSuite Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&PublicInterfaceSuite, Framework, "PublicInterface", "ReportRouter.Phase", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for PublicInterfaceSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (PublicInterfaceSuite, "Initialize public interface should succeed", "InitProtocol", UnitTestInitProtocol, NULL, InterfaceCleanup, NULL);
  AddTestCase (PublicInterfaceSuite, "Register MFCI change notify should succeed", "RegisterNotify", UnitTestRegisterNotify, NULL, InterfaceCleanup, NULL);
  AddTestCase (PublicInterfaceSuite, "Register MFCI change notify should fail after End of DXE", "RegisterAfterDxe", UnitTestRegisterAfterDxe, NULL, InterfaceCleanup, NULL);
  AddTestCase (PublicInterfaceSuite, "Register MFCI change notify should fail if callback is NULL", "RegisterNull", UnitTestRegisterNull, NULL, InterfaceCleanup, NULL);
  AddTestCase (PublicInterfaceSuite, "Registered MFCI callback should ignore if no callback registered", "NotifyChangeNone", UnitTestNotifyChangeNone, NULL, InterfaceCleanup, NULL);
  AddTestCase (PublicInterfaceSuite, "Registered MFCI callback should invoke if one callback registered", "NotifyChangeOne", UnitTestNotifyChangeOne, NULL, InterfaceCleanup, NULL);
  AddTestCase (PublicInterfaceSuite, "Registered MFCI callback should invoke if multiple callbacks registered", "NotifyChangeMultiple", UnitTestNotifyChangeMultiple, NULL, InterfaceCleanup, NULL);
  AddTestCase (PublicInterfaceSuite, "Get MFCI policy should succeed", "GetPolicy", UnitTestGetPolicy, NULL, InterfaceCleanup, NULL);
  AddTestCase (PublicInterfaceSuite, "Registered MFCI callbacks should be cleaned", "CleanRegistered", UnitTestCleanRegistered, NULL, InterfaceCleanup, NULL);
  AddTestCase (PublicInterfaceSuite, "All registered MFCI callbacks should be cleaned", "CleanAllRegistered", UnitTestCleanAllRegistered, NULL, InterfaceCleanup, NULL);

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
