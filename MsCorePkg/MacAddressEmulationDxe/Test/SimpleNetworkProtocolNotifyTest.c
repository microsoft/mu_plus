/** @file
  Tests for SNP discovery and synchronous MAC emulation during initialization.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "SimpleNetworkProtocolNotifyTest.h"

STATIC MAC_EMULATION_SNP_NOTIFY_CONTEXT  mMacContext;
STATIC EFI_SIMPLE_NETWORK_PROTOCOL       mSnps[2];
STATIC EFI_SIMPLE_NETWORK_MODE           mModes[2];
STATIC EFI_STATUS                        mInitializeStatus;
STATIC EFI_STATUS                        mStationAddressStatus;
STATIC UINTN                             mInitializeCalls;
STATIC UINTN                             mStationAddressCalls;
STATIC BOOLEAN                           mCallbackRan;

STATIC
EFI_STATUS
EFIAPI
InitializeSnp (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN UINTN                        ExtraRxBufferSize OPTIONAL,
  IN UINTN                        ExtraTxBufferSize OPTIONAL
  )
{
  EFI_TPL  OldTpl;

  assert_true ((This == &mSnps[0]) || (This == &mSnps[1]));
  assert_int_equal (ExtraRxBufferSize, 123);
  assert_int_equal (ExtraTxBufferSize, 456);
  mInitializeCalls++;
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  if (!EFI_ERROR (mInitializeStatus)) {
    This->Mode->State = EfiSimpleNetworkInitialized;
    CopyMem (&This->Mode->CurrentAddress, &This->Mode->PermanentAddress, sizeof (EFI_MAC_ADDRESS));
  }

  gBS->RestoreTPL (OldTpl);
  return mInitializeStatus;
}

STATIC
EFI_STATUS
EFIAPI
SetStationAddress (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      Reset,
  IN EFI_MAC_ADDRESS              *New OPTIONAL
  )
{
  assert_int_equal (mCurrentTpl, TPL_CALLBACK);
  assert_false (Reset);
  assert_memory_equal (New, &mMacContext.EmulationAddress, sizeof (*New));
  mStationAddressCalls++;
  if (!EFI_ERROR (mStationAddressStatus)) {
    CopyMem (&This->Mode->CurrentAddress, New, sizeof (*New));
  }

  return mStationAddressStatus;
}

STATIC
VOID
ObserveFirstTransmit (
  VOID
  )
{
  assert_memory_equal (&mModes[0].CurrentAddress, &mMacContext.EmulationAddress, NET_ETHER_ADDR_LEN);
  assert_true (mMacContext.Assigned);
  mCallbackRan = TRUE;
}

STATIC
VOID
NotifySnps (
  IN UINTN  Count
  )
{
  EFI_HANDLE  *Handles;
  UINTN       Index;
  EFI_TPL     OldTpl;

  Handles = AllocateZeroPool (Count * sizeof (*Handles));
  for (Index = 0; Index < Count; Index++) {
    Handles[Index] = (EFI_HANDLE)&mSnps[Index];
    will_return (HandleProtocol, &mSnps[Index]);
    will_return (HandleProtocol, EFI_SUCCESS);
  }

  will_return (LocateHandleBuffer, Count);
  will_return (LocateHandleBuffer, Handles);
  will_return (LocateHandleBuffer, EFI_SUCCESS);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  SimpleNetworkProtocolNotify (NULL, &mMacContext);
  gBS->RestoreTPL (OldTpl);
}

STATIC
UNIT_TEST_STATUS
EFIAPI
InitializeProgramsMacBeforeTransmit (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  EFI_TPL     CallerTpl;

  NotifySnps (1);
  CallerTpl        = (EFI_TPL)(UINTN)Context;
  mCurrentTpl      = CallerTpl;
  mPendingCallback = ObserveFirstTransmit;
  will_return (PlatformMacEmulationSnpCheck, TRUE);

  Status = mSnps[0].Initialize (&mSnps[0], 123, 456);

  assert_int_equal (Status, EFI_SUCCESS);
  assert_int_equal (mCurrentTpl, CallerTpl);
  assert_int_equal (mInitializeCalls, 1);
  assert_int_equal (mStationAddressCalls, 1);
  assert_memory_equal (&mMacContext.PermanentAddress, &mModes[0].PermanentAddress, NET_ETHER_ADDR_LEN);
  if (CallerTpl == TPL_CALLBACK) {
    assert_false (mCallbackRan);
    ObserveFirstTransmit ();
    gBS->RestoreTPL (TPL_APPLICATION);
  }

  assert_true (mCallbackRan);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
InitializeFailureDoesNotProgramMac (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  NotifySnps (1);
  mInitializeStatus = EFI_DEVICE_ERROR;

  assert_int_equal (mSnps[0].Initialize (&mSnps[0], 123, 456), EFI_DEVICE_ERROR);
  assert_int_equal (mInitializeCalls, 1);
  assert_int_equal (mStationAddressCalls, 0);
  assert_false (mMacContext.Assigned);
  assert_int_equal (mCurrentTpl, TPL_APPLICATION);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
StationAddressFailurePreservesInitializeSuccess (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  NotifySnps (1);
  mStationAddressStatus = EFI_DEVICE_ERROR;
  will_return (PlatformMacEmulationSnpCheck, TRUE);

  assert_int_equal (mSnps[0].Initialize (&mSnps[0], 123, 456), EFI_SUCCESS);
  assert_int_equal (mStationAddressCalls, 1);
  assert_false (mMacContext.Assigned);
  assert_memory_equal (&mModes[0].CurrentAddress, &mModes[0].PermanentAddress, NET_ETHER_ADDR_LEN);
  assert_int_equal (mCurrentTpl, TPL_APPLICATION);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
DiscoveryIsIdempotentAndReinitializationReappliesMac (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  NotifySnps (1);
  NotifySnps (1);
  assert_true (IsNodeAtEnd (&mSnpContexts, GetFirstNode (&mSnpContexts)));
  will_return (PlatformMacEmulationSnpCheck, TRUE);
  assert_int_equal (mSnps[0].Initialize (&mSnps[0], 123, 456), EFI_SUCCESS);

  mModes[0].State = EfiSimpleNetworkStarted;
  ZeroMem (&mModes[0].CurrentAddress, sizeof (EFI_MAC_ADDRESS));
  will_return (PlatformMacEmulationSnpCheck, TRUE);
  assert_int_equal (mSnps[0].Initialize (&mSnps[0], 123, 456), EFI_SUCCESS);
  assert_int_equal (mInitializeCalls, 2);
  assert_int_equal (mStationAddressCalls, 2);
  assert_memory_equal (&mModes[0].CurrentAddress, &mMacContext.EmulationAddress, NET_ETHER_ADDR_LEN);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
OnlyFirstEligibleAdapterReceivesMac (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  NotifySnps (2);
  will_return (PlatformMacEmulationSnpCheck, TRUE);
  will_return (PlatformMacEmulationSnpCheck, TRUE);

  assert_int_equal (mSnps[0].Initialize (&mSnps[0], 123, 456), EFI_SUCCESS);
  assert_int_equal (mSnps[1].Initialize (&mSnps[1], 123, 456), EFI_SUCCESS);
  assert_int_equal (mInitializeCalls, 2);
  assert_int_equal (mStationAddressCalls, 1);
  assert_memory_equal (&mModes[0].CurrentAddress, &mMacContext.EmulationAddress, NET_ETHER_ADDR_LEN);
  assert_memory_equal (&mModes[1].CurrentAddress, &mModes[1].PermanentAddress, NET_ETHER_ADDR_LEN);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
UnsupportedAdapterRetainsOriginalBehavior (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  NotifySnps (1);
  will_return (PlatformMacEmulationSnpCheck, FALSE);

  assert_int_equal (mSnps[0].Initialize (&mSnps[0], 123, 456), EFI_SUCCESS);
  assert_int_equal (mStationAddressCalls, 0);
  assert_false (mMacContext.Assigned);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
SnpWrapperTestSetup (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN  Index;

  gBS                     = &mBootServices;
  gBS->LocateHandleBuffer = LocateHandleBuffer;
  gBS->HandleProtocol     = HandleProtocol;
  gBS->RaiseTPL           = RaiseTPL;
  gBS->RestoreTPL         = RestoreTPL;
  mCurrentTpl             = TPL_APPLICATION;
  mPendingCallback        = NULL;
  mCallbackRan            = FALSE;
  mInitializeStatus       = EFI_SUCCESS;
  mStationAddressStatus   = EFI_SUCCESS;
  mInitializeCalls        = 0;
  mStationAddressCalls    = 0;
  ZeroMem (&mMacContext, sizeof (mMacContext));
  ZeroMem (mSnps, sizeof (mSnps));
  ZeroMem (mModes, sizeof (mModes));
  SetMem (&mMacContext.EmulationAddress, NET_ETHER_ADDR_LEN, 0xEE);
  for (Index = 0; Index < ARRAY_SIZE (mSnps); Index++) {
    mSnps[Index].Mode                  = &mModes[Index];
    mSnps[Index].Initialize            = InitializeSnp;
    mSnps[Index].StationAddress        = SetStationAddress;
    mModes[Index].State                = EfiSimpleNetworkStarted;
    mModes[Index].IfType               = NET_IFTYPE_ETHERNET;
    mModes[Index].MacAddressChangeable = TRUE;
    SetMem (&mModes[Index].PermanentAddress, NET_ETHER_ADDR_LEN, (UINT8)(0xAA + Index));
  }

  return UNIT_TEST_PASSED;
}

STATIC
VOID
EFIAPI
SnpWrapperTestCleanup (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MAC_EMULATION_SNP_CONTEXT  *SnpContext;

  mPendingCallback = NULL;
  mCurrentTpl      = TPL_APPLICATION;
  while (!IsListEmpty (&mSnpContexts)) {
    SnpContext                  = BASE_CR (GetFirstNode (&mSnpContexts), MAC_EMULATION_SNP_CONTEXT, Link);
    SnpContext->Snp->Initialize = SnpContext->OriginalInitialize;
    RemoveEntryList (&SnpContext->Link);
    FreePool (SnpContext);
  }
}

VOID
RegisterSimpleNetworkProtocolNotifyTests (
  UNIT_TEST_SUITE_HANDLE  SuiteHandle
  )
{
  AddTestCase (SuiteHandle, "MAC precedes pending transmit from application TPL", "Snp.Initialize.Application", InitializeProgramsMacBeforeTransmit, SnpWrapperTestSetup, SnpWrapperTestCleanup, (VOID *)(UINTN)TPL_APPLICATION);
  AddTestCase (SuiteHandle, "MAC precedes immediate transmit from callback TPL", "Snp.Initialize.Callback", InitializeProgramsMacBeforeTransmit, SnpWrapperTestSetup, SnpWrapperTestCleanup, (VOID *)(UINTN)TPL_CALLBACK);
  AddTestCase (SuiteHandle, "Initialize failure does not program MAC", "Snp.Initialize.Failure", InitializeFailureDoesNotProgramMac, SnpWrapperTestSetup, SnpWrapperTestCleanup, NULL);
  AddTestCase (SuiteHandle, "MAC failure preserves initialization success", "Snp.Initialize.MacFailure", StationAddressFailurePreservesInitializeSuccess, SnpWrapperTestSetup, SnpWrapperTestCleanup, NULL);
  AddTestCase (SuiteHandle, "Discovery is idempotent and reinitialization reapplies MAC", "Snp.Initialize.Reinitialize", DiscoveryIsIdempotentAndReinitializationReappliesMac, SnpWrapperTestSetup, SnpWrapperTestCleanup, NULL);
  AddTestCase (SuiteHandle, "Only the first eligible adapter receives MAC", "Snp.Initialize.Selection", OnlyFirstEligibleAdapterReceivesMac, SnpWrapperTestSetup, SnpWrapperTestCleanup, NULL);
  AddTestCase (SuiteHandle, "Unsupported adapters retain original behavior", "Snp.Initialize.Unsupported", UnsupportedAdapterRetainsOriginalBehavior, SnpWrapperTestSetup, SnpWrapperTestCleanup, NULL);
}
