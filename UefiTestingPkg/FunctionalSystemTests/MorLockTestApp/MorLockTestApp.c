/** @file -- MorLockTestApp.c
This application will test the MorLock v1 and v2 variable protection feature.
https://msdn.microsoft.com/en-us/windows/hardware/drivers/bringup/device-guard-requirements

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/UnitTestLib.h>
#include <Library/UnitTestBootLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Guid/MemoryOverwriteControl.h>
#include <IndustryStandard/MemoryOverwriteRequestControlLock.h>


#define UNIT_TEST_APP_NAME        "MORLock v1 and v2 Test"
#define UNIT_TEST_APP_VERSION     "0.1"

#define MOR_LOCK_DATA_UNLOCKED           0x0
#define MOR_LOCK_DATA_LOCKED_WITHOUT_KEY 0x1
#define MOR_LOCK_DATA_LOCKED_WITH_KEY    0x2

#define MOR_LOCK_V1_SIZE      (1)
#define MOR_LOCK_V2_KEY_SIZE  (8)

#define MOR_VARIABLE_ATTRIBUTES       (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)
#define MOR_VARIABLE_BAD_ATTRIBUTES1  (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS)
#define MOR_VARIABLE_BAD_ATTRIBUTES2  (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)

UINT8     mTestKey1[] = { 0xD5, 0x80, 0xC6, 0x1D, 0x84, 0x44, 0x4E, 0x87 };
UINT8     mTestKey2[] = { 0x94, 0x88, 0x8F, 0xFE, 0x1D, 0x6C, 0xE0, 0x68 };
UINT8     mTestKey3[] = { 0x81, 0x51, 0x1E, 0x00, 0xCB, 0xFE, 0x48, 0xD9 };


///================================================================================================
///================================================================================================
///
/// HELPER FUNCTIONS
///
///================================================================================================
///================================================================================================


/**
  NOTE: Takes in a ResetType, but currently only supports EfiResetCold
        and EfiResetWarm. All other types will return EFI_INVALID_PARAMETER.
        If a more specific reset is required, use SaveFrameworkState() and
        call gRT->ResetSystem() directly.

**/
EFI_STATUS
EFIAPI
SaveFrameworkStateAndReboot (
  IN UNIT_TEST_CONTEXT          ContextToSave     OPTIONAL,
  IN UINTN                      ContextToSaveSize,
  IN EFI_RESET_TYPE             ResetType
  )
{
  EFI_STATUS                  Status;

  //
  // First, let's not make assumptions about the parameters.
  if (ResetType != EfiResetCold && ResetType != EfiResetWarm)
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Now, save all the data associated with this framework.
  Status = SaveFrameworkState( ContextToSave, ContextToSaveSize );

  //
  // If we're all good, let's book...
  if (!EFI_ERROR( Status ))
  {
    //
    // Next, we want to update the BootNext variable to USB
    // so that we have a fighting chance of coming back here.
    //
    SetBootNextDevice();

    //
    // Reset 
    gRT->ResetSystem( ResetType, EFI_SUCCESS, 0, NULL );
    DEBUG(( DEBUG_ERROR, "%a - Unit test failed to quit! Framework can no longer be used!\n", __FUNCTION__ ));

    //
    // We REALLY shouldn't be here.
    Status = EFI_ABORTED;
  }

  return Status;
} // SaveFrameworkStateAndReboot()


UNIT_TEST_STATUS
EFIAPI
MorControlVariableShouldBeCorrect (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS    Status;
  UINT32        Attributes;
  UINTN         DataSize;
  UINT8         Data;

  UT_LOG_VERBOSE( "%a()\n", __FUNCTION__ );

  DataSize = sizeof( Data );
  Status = gRT->GetVariable( MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                             &gEfiMemoryOverwriteControlDataGuid,
                             &Attributes,
                             &DataSize,
                             &Data );

  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(Attributes, MOR_VARIABLE_ATTRIBUTES);
  UT_ASSERT_EQUAL(DataSize, sizeof(Data));

  return UNIT_TEST_PASSED;
} // MorControlVariableShouldBeCorrect()


STATIC
EFI_STATUS
GetMorControlVariable (
  OUT UINT8       *MorControl
  )
{
  EFI_STATUS    Status;
  UINTN         DataSize;
  UINT8         Data;

  DataSize = sizeof( Data );
  Status = gRT->GetVariable( MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                             &gEfiMemoryOverwriteControlDataGuid,
                             NULL,
                             &DataSize,
                             &Data );

  if (!EFI_ERROR( Status ))
  {
    if (DataSize != sizeof( *MorControl ))
    {
      Status = EFI_BAD_BUFFER_SIZE;
    }
    else
    {
      *MorControl = Data;
    }
  }

  return Status;
} // GetMorControlVariable()


STATIC
EFI_STATUS
SetMorControlVariable (
  IN UINT8       *MorControl
  )
{
  EFI_STATUS    Status;

  Status = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                             &gEfiMemoryOverwriteControlDataGuid,
                             MOR_VARIABLE_ATTRIBUTES,
                             sizeof( *MorControl ),
                             MorControl );

  return Status;
} // SetMorControlVariable()


STATIC
VOID
UnitTestCleanupReboot (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  // Reboot this mother.
  SaveFrameworkStateAndReboot( NULL, 0, EfiResetCold );
  return;
} // UnitTestCleanupReboot()


STATIC
EFI_STATUS
GetMorLockVariable (
  OUT UINT8       *MorLock
  )
{
  EFI_STATUS    Status;
  UINTN         DataSize;
  UINT8         Data;

  DataSize = sizeof( Data );
  Status = gRT->GetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                             &gEfiMemoryOverwriteRequestControlLockGuid,
                             NULL,
                             &DataSize,
                             &Data );

  if (!EFI_ERROR( Status ))
  {
    if (DataSize != sizeof( *MorLock ))
    {
      Status = EFI_BAD_BUFFER_SIZE;
    }
    else
    {
      *MorLock = Data;
    }
  }

  return Status;
} // GetMorLockVariable()


UNIT_TEST_STATUS
EFIAPI
MorLockShouldNotBeSet (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS    Status;
  UINT8         MorLock;

  UT_LOG_VERBOSE( "%a()\n", __FUNCTION__ );

  Status = GetMorLockVariable( &MorLock );

  //
  // If not found then its ok
  //
  if (Status == EFI_NOT_FOUND)
  {
    return UNIT_TEST_PASSED;
  }

  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(MorLock, MOR_LOCK_DATA_UNLOCKED);

  return UNIT_TEST_PASSED;
} // MorLockShouldNotBeSet()


///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================


UNIT_TEST_STATUS
EFIAPI
MorControlVariableShouldExist (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS    Status;
  UINTN         DataSize;
  UINT8         Data;

  DataSize = sizeof( Data );
  Status = gRT->GetVariable( MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                             &gEfiMemoryOverwriteControlDataGuid,
                             NULL,
                             &DataSize,
                             &Data );

  UT_ASSERT_NOT_EQUAL(Status, EFI_NOT_FOUND);
  return UNIT_TEST_PASSED;
} // MorControlVariableShouldExist()


UNIT_TEST_STATUS
EFIAPI
MorControlVariableShouldHaveCorrectSize (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS    Status;
  UINTN         DataSize;
  UINT8         Data;

  DataSize = sizeof( Data );
  Status = gRT->GetVariable( MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                             &gEfiMemoryOverwriteControlDataGuid,
                             NULL,
                             &DataSize,
                             &Data );

  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(DataSize, sizeof(Data));

  return UNIT_TEST_PASSED;
} // MorControlVariableShouldHaveCorrectSize()


UNIT_TEST_STATUS
EFIAPI
MorControlVariableShouldHaveCorrectAttributes (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS    Status;
  UINT32        Attributes;
  UINTN         DataSize;
  UINT8         Data;

  DataSize = sizeof( Data );
  Status = gRT->GetVariable( MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                             &gEfiMemoryOverwriteControlDataGuid,
                             &Attributes,
                             &DataSize,
                             &Data );

  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(Attributes, MOR_VARIABLE_ATTRIBUTES);

  return UNIT_TEST_PASSED;
} // MorControlVariableShouldHaveCorrectAttributes()


UNIT_TEST_STATUS
EFIAPI
MorControlShouldNotBeDeletable (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;

  Status  = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                              &gEfiMemoryOverwriteControlDataGuid,
                              0,
                              0,
                              NULL );

  UT_ASSERT_STATUS_EQUAL(Status, EFI_INVALID_PARAMETER);
  return UNIT_TEST_PASSED;

} // MorControlShouldNotBeDeletable()


UNIT_TEST_STATUS
EFIAPI
MorControlShouldEnforceCorrectAttributes (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINT8             Data = FALSE;

  Status  = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                              &gEfiMemoryOverwriteControlDataGuid,
                              MOR_VARIABLE_BAD_ATTRIBUTES1,
                              sizeof( Data ),
                              &Data );

  UT_ASSERT_STATUS_EQUAL(Status, EFI_INVALID_PARAMETER);
  return UNIT_TEST_PASSED;
} // MorControlShouldEnforceCorrectAttributes()


UNIT_TEST_STATUS
EFIAPI
MorControlShouldChangeWhenNotLocked (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS    Status;
  UINT8         MorControl;

  // Make sure that the variable can be set to TRUE.
  MorControl = TRUE;
  Status = SetMorControlVariable( &MorControl );
  UT_ASSERT_NOT_EFI_ERROR(Status);

  Status = GetMorControlVariable(&MorControl);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_TRUE(MorControl);

  // Make sure that the variable can be set to FALSE.
  UT_ASSERT_NOT_EFI_ERROR(Status);

  MorControl = FALSE;
  Status = SetMorControlVariable( &MorControl );
  UT_ASSERT_NOT_EFI_ERROR(Status);

  Status = GetMorControlVariable( &MorControl );
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_FALSE(MorControl);

  return UNIT_TEST_PASSED;
} // MorControlShouldChangeWhenNotLocked()


UNIT_TEST_STATUS
EFIAPI
MorLockv1ShouldNotSetBadValue (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINTN             DataSize;
  UINT8             MorLock;

  // Attempt to set the MorLock to a non-key, non-TRUE/FALSE value.
  MorLock   = 0xAA;
  DataSize  = sizeof( MorLock );
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &MorLock );
  // Make sure that the status is EFI_INVALID_PARAMETER.
  UT_ASSERT_STATUS_EQUAL(Status, EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
} // MorLockv1ShouldNotSetBadValue()


UNIT_TEST_STATUS
EFIAPI
MorLockv1ShouldNotSetBadBufferSize (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINTN             DataSize;
  UINT8             MorLock[] = { 0xDE, 0xAD, 0xBE, 0xEF };

  // Attempt to set the MorLock to a non-key, non-TRUE/FALSE value.
  DataSize  = sizeof( MorLock );
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &MorLock );

  // Make sure that the status is EFI_INVALID_PARAMETER.
  UT_ASSERT_STATUS_EQUAL(Status, EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
} // MorLockv1ShouldNotSetBadBufferSize()


UNIT_TEST_STATUS
EFIAPI
MorLockShouldNotSetBadAttributes (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINTN             DataSize;
  UINT8             MorLock;

  // Attempt to set the MorLock.
  DataSize  = sizeof( MorLock );
  MorLock   = TRUE;
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_BAD_ATTRIBUTES1,
                                DataSize,
                                &MorLock );

  UT_ASSERT_STATUS_EQUAL(Status, EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
} // MorLockShouldNotSetBadAttributes()


UNIT_TEST_STATUS
EFIAPI
MorLockv1ShouldBeLockable (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINTN             DataSize;
  UINT8             MorLock;

  // Attempt to set the MorLock.
  DataSize  = sizeof( MorLock );
  MorLock   = TRUE;
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &MorLock );

  //
  // NOTE: Strictly speaking, this isn't a good unit test.
  //       After this test runs, the MorLock is set and the other tests
  //       have some expectation that the lock will behave a certain way.
  //       We *could* make better unit tests, but there would be a lot more
  //       reboots. So let's say this is for efficiency.
  //
  UT_ASSERT_NOT_EFI_ERROR(Status);

  return UNIT_TEST_PASSED;
} // MorLockv1ShouldBeLockable()


UNIT_TEST_STATUS
EFIAPI
MorLockv1ShouldReportCorrectly (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS    Status;
  UINT8         MorLock;

  UT_LOG_VERBOSE( "%a()\n", __FUNCTION__ );

  Status = GetMorLockVariable( &MorLock );

  UT_LOG_VERBOSE( "%a - Status = %r, MorLock = %d\n", __FUNCTION__, Status, MorLock );

  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(MorLock, MOR_LOCK_DATA_LOCKED_WITHOUT_KEY);

  return UNIT_TEST_PASSED;
} // MorLockv1ShouldReportCorrectly()


UNIT_TEST_STATUS
EFIAPI
MorControlShouldNotChange (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINT8             MorControl;
  UNIT_TEST_STATUS  Result = UNIT_TEST_PASSED;

  // Determine the current status.
  Status = GetMorControlVariable( &MorControl );
  if (EFI_ERROR( Status ))
  {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  // If we're set, try to unset.
  if (MorControl)
  {
    MorControl = FALSE;
    Status = SetMorControlVariable( &MorControl );
    // If this was successful, that's not good.
    if (!EFI_ERROR( Status ))
    {
      Result = UNIT_TEST_ERROR_TEST_FAILED;
    }
  }
  // If we're unset, try to set.
  else
  {
    MorControl = TRUE;
    Status = SetMorControlVariable( &MorControl );
    // If this was successful, that's not good.
    if (!EFI_ERROR( Status ))
    {
      Result = UNIT_TEST_ERROR_TEST_FAILED;
    }
  }

  return Result;
} // MorControlShouldNotChange()


UNIT_TEST_STATUS
EFIAPI
MorLockv1ShouldNotChangeWhenLocked (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINTN             DataSize;
  UINT8             MorLock;

  // Attempt to unset the MorLock.
  DataSize  = sizeof( MorLock );
  MorLock   = FALSE;
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &MorLock );
  UT_ASSERT_STATUS_EQUAL(Status, EFI_ACCESS_DENIED);
  return UNIT_TEST_PASSED;
} // MorLockv1ShouldNotChangeWhenLocked()


UNIT_TEST_STATUS
EFIAPI
MorLockv1ShouldNotBeDeleteable (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;

  // Attempt to delete the MorLock.
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                0,
                                0,
                                NULL );
  UT_ASSERT_STATUS_EQUAL(Status, EFI_WRITE_PROTECTED);

  return  UNIT_TEST_PASSED;
} // MorLockv1ShouldNotBeDeleteable()


UNIT_TEST_STATUS
EFIAPI
MorLockShouldClearAfterReboot (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINT8             MorLock;
  UNIT_TEST_STATUS  Result = UNIT_TEST_ERROR_TEST_FAILED;
  BOOLEAN           IsPostReboot = FALSE;

  // Because we're going to reboot, we need to check for
  // a saved context.
  if (Context != NULL)
  {
    IsPostReboot = *(BOOLEAN*)Context;
  }

  // If we haven't reboot yet, we need to reboot.
  if (!IsPostReboot)
  {
    // Indicate that we've gotten here already...
    IsPostReboot = TRUE;

    UT_LOG_INFO( "Going down for reboot!\n" );
    // A warm reboot should be sufficient.
    SaveFrameworkStateAndReboot( &IsPostReboot, sizeof( IsPostReboot ), EfiResetWarm );
    // We shouldn't get here. If we do, we'll just return failure from this function.
    UT_LOG_ERROR( "Reboot failed! Should never get here!!\n" );
  }
  // Otherwise, we need to check the status of the MorLock.
  else
  {
    // Check the MorLock.
    UT_LOG_INFO("Running after reboot!\n");
    Status = GetMorLockVariable( &MorLock );
    if (!EFI_ERROR( Status ) && MorLock == MOR_LOCK_DATA_UNLOCKED)
    {
      Result = UNIT_TEST_PASSED;
    }
  }

  return Result;
} // MorLockShouldClearAfterReboot()


UNIT_TEST_STATUS
EFIAPI
MorLockv2ShouldNotSetSmallBuffer (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINTN             DataSize;
  UINT8             MorLock[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF };

  // First off, let's just make sure that our test is rational.
  DataSize = sizeof( MorLock );
  ASSERT(DataSize >= (MOR_LOCK_V2_KEY_SIZE - 1));   //just make sure test writer has a buffer of acceptable size

  // Attempt to set the MorLock to smaller than designated key size.
  DataSize = MOR_LOCK_V2_KEY_SIZE - 1;
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &MorLock );

  // Make sure that the status is EFI_INVALID_PARAMETER.
  UT_ASSERT_STATUS_EQUAL(Status, EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
} // MorLockv2ShouldNotSetSmallBuffer()


UNIT_TEST_STATUS
EFIAPI
MorLockv2ShouldNotSetLargeBuffer (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINTN             DataSize;
  UINT8             MorLock[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF };
  UNIT_TEST_STATUS  Result = UNIT_TEST_PASSED;

  // First off, let's just make sure that our test is rational.
  DataSize = sizeof( MorLock );
  ASSERT(DataSize >= MOR_LOCK_V2_KEY_SIZE + 1); //just make sure test writer has a buffer of acceptable size

  // Attempt to set the MorLock to larger than designated key size.
  DataSize = MOR_LOCK_V2_KEY_SIZE + 1;
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &MorLock );

  // Make sure that the status is EFI_INVALID_PARAMETER.
  UT_ASSERT_STATUS_EQUAL(Status, EFI_INVALID_PARAMETER);

  return Result;
} // MorLockv2ShouldNotSetLargeBuffer()


UNIT_TEST_STATUS
EFIAPI
MorLockv2ShouldNotSetNoBuffer (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINTN             DataSize;
  UINT8             MorLock;
  UNIT_TEST_STATUS  Result = UNIT_TEST_PASSED;

  // Attempt to set the MorLock v2 directly;
  DataSize  = sizeof( MorLock );
  MorLock   = MOR_LOCK_DATA_LOCKED_WITH_KEY;
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &MorLock );

  // Make sure that the status is EFI_INVALID_PARAMETER.
  UT_ASSERT_STATUS_EQUAL(Status, EFI_INVALID_PARAMETER);

  return Result;
} // MorLockv2ShouldNotSetNoBuffer()


UNIT_TEST_STATUS
EFIAPI
MorLockv2ShouldBeLockable (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINTN             DataSize;

  // Attempt to set a key for MorLock v2.
  // For this test, we'll use Test Key 1.
  DataSize  = sizeof( mTestKey1 );
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &mTestKey1[0] );

  //
  // NOTE: Strictly speaking, this isn't a good unit test.
  //       After this test runs, the MorLock is set and the other tests
  //       have some expectation that the lock will behave a certain way.
  //       We *could* make better unit tests, but there would be a lot more
  //       reboots. So let's say this is for efficiency.
  //

  UT_ASSERT_NOT_EFI_ERROR(Status);

  return UNIT_TEST_PASSED;
} // MorLockv2ShouldBeLockable()


UNIT_TEST_STATUS
EFIAPI
MorLockv2ShouldReportCorrectly (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS    Status;
  UINT8         MorLock;

  UT_LOG_VERBOSE( "%a()\n", __FUNCTION__ );

  Status = GetMorLockVariable( &MorLock );

  UT_LOG_VERBOSE( "%a - Status = %r, MorLock = %d\n", __FUNCTION__, Status, MorLock );

  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(MorLock, MOR_LOCK_DATA_LOCKED_WITH_KEY);

  return UNIT_TEST_PASSED;
} // MorLockv2ShouldReportCorrectly()


UNIT_TEST_STATUS
EFIAPI
MorLockv2ShouldOnlyReturnOneByte (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS    Status;
  UINTN         DataSize;
  UINT8         MorLock[MOR_LOCK_V2_KEY_SIZE];

  // Blank the buffer so we know it doesn't contain the key.
  ZeroMem( &MorLock[0], sizeof( MorLock ) );

  // Fetch the MorLock so we can see what we get.
  DataSize = sizeof( MorLock );
  Status = gRT->GetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                             &gEfiMemoryOverwriteRequestControlLockGuid,
                             NULL,
                             &DataSize,
                             &MorLock );

  // Check to see how much data was returned.
  if (!EFI_ERROR( Status ) && DataSize > 1)
  {
    Status = EFI_ACCESS_DENIED;
  }

  UT_ASSERT_NOT_EFI_ERROR(Status);

  return UNIT_TEST_PASSED;
} // MorLockv2ShouldOnlyReturnOneByte()


UNIT_TEST_STATUS
EFIAPI
MorLockv2ShouldNotReturnKey (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS    Status;
  UINTN         DataSize;
  UINT8         MorLock[MOR_LOCK_V2_KEY_SIZE];

  // Blank the buffer so we know it doesn't contain the key.
  ZeroMem( &MorLock[0], sizeof( MorLock ) );

  // Fetch the MorLock so we can see what we get.
  DataSize = sizeof( MorLock );
  Status = gRT->GetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                             &gEfiMemoryOverwriteRequestControlLockGuid,
                             NULL,
                             &DataSize,
                             &MorLock );

  // Check for the key in the buffer.
  // We would EXPECT to only receive one byte, but you never know.
  if (!EFI_ERROR( Status ) && DataSize > 1)
  {
    // We're only using three keys in these tests.
    // Might as well check them all.
    if (CompareMem( &MorLock[0], &mTestKey1[0], sizeof( MorLock ) ) == 0 ||
        CompareMem( &MorLock[0], &mTestKey2[0], sizeof( MorLock ) ) == 0 ||
        CompareMem( &MorLock[0], &mTestKey3[0], sizeof( MorLock ) ) == 0)
    {
      Status = EFI_ACCESS_DENIED;
    }
  }
  UT_ASSERT_NOT_EFI_ERROR(Status);

  return UNIT_TEST_PASSED;
} // MorLockv2ShouldNotReturnKey()


UNIT_TEST_STATUS
EFIAPI
MorLockv2ShouldNotChangeWhenLocked (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINTN             DataSize;

  // Attempt to change the key for MorLock v2.
  // For this test, we'll use Test Key 2.
  DataSize  = sizeof( mTestKey2 );
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &mTestKey2[0] );
  UT_ASSERT_STATUS_EQUAL(Status, EFI_ACCESS_DENIED);
  return UNIT_TEST_PASSED;
} // MorLockv2ShouldNotChangeWhenLocked()


UNIT_TEST_STATUS
EFIAPI
MorLockv2ShouldNotChangeTov1 (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINTN             DataSize;
  UINT8             MorLock;

  // Attempt to set the MorLock to v1.
  DataSize  = sizeof( MorLock );
  MorLock   = TRUE;
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &MorLock );
  UT_ASSERT_STATUS_EQUAL(Status, EFI_ACCESS_DENIED);

  return UNIT_TEST_PASSED;
} // MorLockv2ShouldNotChangeTov1()


UNIT_TEST_STATUS
EFIAPI
MorLockv2ShouldNotBeDeleteable (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;

  // Attempt to delete the MorLock.
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                0,
                                0,
                                NULL );
  UT_ASSERT_STATUS_EQUAL(Status, EFI_WRITE_PROTECTED);

  return UNIT_TEST_PASSED;
} // MorLockv2ShouldNotBeDeleteable()


UNIT_TEST_STATUS
EFIAPI
MorLockv2ShouldClearWithCorrectKey (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINTN             DataSize;
  UINT8             MorLock;
  UNIT_TEST_STATUS  Result = UNIT_TEST_ERROR_TEST_FAILED;

  // Attempt to set a key for MorLock v2.
  // For this test, we'll use Test Key 1.
  DataSize  = sizeof( mTestKey1 );
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &mTestKey1[0] );
  // If this key failed to set, we're done here.
  if (EFI_ERROR( Status ))
  {
    goto Exit;
  }

  // Verify that the key was set.
  Status = GetMorLockVariable( &MorLock );
  if (EFI_ERROR( Status ) || MorLock != MOR_LOCK_DATA_LOCKED_WITH_KEY)
  {
    goto Exit;
  }

  // Attempt to clear with the same key.
  DataSize  = sizeof( mTestKey1 );
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &mTestKey1[0] );
  // If this key failed to set, we're done here.
  if (EFI_ERROR( Status ))
  {
    goto Exit;
  }

  // Verify that mode is now disabled.
  Status = GetMorLockVariable( &MorLock );
  if (EFI_ERROR( Status ) || MorLock != MOR_LOCK_DATA_UNLOCKED)
  {
    goto Exit;
  }

  // Only if we've made it this far are we good to return UNIT_TEST_PASSED.
  Result = UNIT_TEST_PASSED;

Exit:
  return Result;
} // MorLockv2ShouldClearWithCorrectKey()


UNIT_TEST_STATUS
EFIAPI
MorLockv2ShouldNotClearWithWrongKey (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINTN             DataSize;
  UINT8             MorLock;

  // Attempt to set a key for MorLock v2.
  // For this test, we'll use Test Key 1.
  DataSize  = sizeof( mTestKey1 );
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &mTestKey1[0] );
  // If this key failed to set, we're done here.
  UT_ASSERT_NOT_EFI_ERROR(Status);


  // Verify that the key was set.
  Status = GetMorLockVariable( &MorLock );
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(MorLock, MOR_LOCK_DATA_LOCKED_WITH_KEY);


  // Attempt to clear with a different key.
  DataSize  = sizeof( mTestKey2 );
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &mTestKey2[0] );
  // If this key was successfully set, we're done here.
  UT_ASSERT_STATUS_EQUAL(Status, EFI_ACCESS_DENIED);

  // Verify that mode is still enabled.
  Status = GetMorLockVariable( &MorLock );
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(MorLock, MOR_LOCK_DATA_LOCKED_WITH_KEY);

  return UNIT_TEST_PASSED;
} // MorLockv2ShouldNotClearWithWrongKey()


UNIT_TEST_STATUS
EFIAPI
MorLockv2ShouldReleaseMorControlAfterClear (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINTN             DataSize;
  UINT8             MorLock;

  // Attempt to set a key for MorLock v2.
  // For this test, we'll use Test Key 1.
  DataSize  = sizeof( mTestKey1 );
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &mTestKey1[0] );
  // If this key failed to set, we're done here.
  UT_ASSERT_NOT_EFI_ERROR(Status);

  // Verify that the key was set.
  Status = GetMorLockVariable( &MorLock );
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_TRUE(MorLock == MOR_LOCK_DATA_LOCKED_WITH_KEY);

  // Attempt to clear with the same key.
  DataSize  = sizeof( mTestKey1 );
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &mTestKey1[0] );
  // If this key failed to set, we're done here.
  UT_ASSERT_NOT_EFI_ERROR(Status);

  // Verify that mode is now disabled.
  Status = GetMorLockVariable( &MorLock );
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(MorLock, MOR_LOCK_DATA_UNLOCKED);

  // If we've made it this far, the only thing left to do is make sure
  // that the MOR Control can change.
  return MorControlShouldChangeWhenNotLocked( NULL );
} // MorLockv2ShouldReleaseMorControlAfterClear()


UNIT_TEST_STATUS
EFIAPI
MorLockv2ShouldSetClearSet (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS        Status;
  UINTN             DataSize;
  UINT8             MorLock;

  //
  // Attempt to set a key for MorLock v2.
  // For this test, we'll use Test Key 1.
  //
  DataSize  = sizeof( mTestKey1 );
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &mTestKey1[0] );
  // If this key failed to set, we're done here.
  UT_ASSERT_NOT_EFI_ERROR(Status);

  // Verify that the key was set.
  Status = GetMorLockVariable( &MorLock );

  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(MorLock, MOR_LOCK_DATA_LOCKED_WITH_KEY);


  //
  // Attempt to clear with the same key.
  //
  DataSize  = sizeof( mTestKey1 );
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &mTestKey1[0] );
  // If this key failed to set, we're done here.
  UT_ASSERT_NOT_EFI_ERROR(Status);


  // Verify that mode is now disabled.
  Status = GetMorLockVariable( &MorLock );
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(MorLock, MOR_LOCK_DATA_UNLOCKED);

  //
  // Attempt to set a second key.
  //
  DataSize  = sizeof( mTestKey2 );
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &mTestKey2[0] );
  // If this key failed to set, we're done here.
  UT_ASSERT_NOT_EFI_ERROR(Status);

  // Verify that the key was set.
  Status = GetMorLockVariable( &MorLock );
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(MorLock, MOR_LOCK_DATA_LOCKED_WITH_KEY);

  //
  // Attempt to clear with a different key.
  //
  DataSize  = sizeof( mTestKey3 );
  Status    = gRT->SetVariable( MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                                &gEfiMemoryOverwriteRequestControlLockGuid,
                                MOR_VARIABLE_ATTRIBUTES,
                                DataSize,
                                &mTestKey3[0] );
  // If this key was successfully set, we're done here.
  UT_ASSERT_STATUS_EQUAL(Status, EFI_ACCESS_DENIED);

  // Verify that mode is still enabled.
  Status = GetMorLockVariable( &MorLock );
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(MorLock, MOR_LOCK_DATA_LOCKED_WITH_KEY);

  return UNIT_TEST_PASSED;
} // MorLockv2ShouldSetClearSet()


///================================================================================================
///================================================================================================
///
/// TEST ENGINE
///
///================================================================================================
///================================================================================================


/**
  MorLockTestApp

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
MorLockTestApp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Fw = NULL;
  UNIT_TEST_SUITE_HANDLE      EnvironmentalTests, MorLockV1Tests, MorLockV2Tests;

  DEBUG(( DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework( &Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the EnvironmentalTests Unit Test Suite.
  //
  Status = CreateUnitTestSuite( &EnvironmentalTests, Fw, "Boot Environment Tests", "Security.MOR", NULL, NULL );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for EnvironmentalTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase( EnvironmentalTests, "On any given boot, the MOR control variable should exist", "Security.MOR.ControlExists", MorControlVariableShouldExist, NULL, NULL, NULL );
  AddTestCase( EnvironmentalTests, "MOR control variable should be the correct size", "Security.MOR.ControlSize", MorControlVariableShouldHaveCorrectSize, NULL, NULL, NULL );
  AddTestCase( EnvironmentalTests, "MOR control variable should have correct attributes", "Security.MOR.ControlAttributesCorrect", MorControlVariableShouldHaveCorrectAttributes, NULL, NULL, NULL );
  AddTestCase( EnvironmentalTests, "Should not be able to delete MOR control variable", "Security.MOR.ControlCannotDelete", MorControlShouldNotBeDeletable, NULL, NULL, NULL );
  AddTestCase( EnvironmentalTests, "Should not be able to create MOR control variable with incorrect attributes", "Security.MOR.ControlAttributesCreate", MorControlShouldEnforceCorrectAttributes, NULL, UnitTestCleanupReboot, NULL );

  // IMPORTANT NOTE: On a reboot test, currently, prereqs will be run each time the test is continued. Ergo, a prereq that may be
  //                 valid on a single boot may not be valid on subsequent boots. THIS MUST BE SOLVED!!

  //
  // Populate the MorLockV1Tests Unit Test Suite.
  //
  Status = CreateUnitTestSuite( &MorLockV1Tests, Fw, "MORLock v1 Tests", "Security.MOR.LockV1", NULL, NULL );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MorLockV1Tests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase( MorLockV1Tests, "Should be able to change MOR control when not locked", "Security.MOR.MorLockV1.MorControlChange", MorControlShouldChangeWhenNotLocked, MorControlVariableShouldBeCorrect, NULL, NULL );
  AddTestCase( MorLockV1Tests, "Should not be able to set MORLock v1 with a bad value", "Security.MOR.MorLockV1.LockValue", MorLockv1ShouldNotSetBadValue, MorLockShouldNotBeSet, NULL, NULL );
  AddTestCase( MorLockV1Tests, "Should not be able to set MORLock v1 with strange buffer size", "Security.MOR.MorLockV1.StrangeSize", MorLockv1ShouldNotSetBadBufferSize, MorLockShouldNotBeSet, NULL, NULL );
  AddTestCase( MorLockV1Tests, "Should not be able to set MORLock v1 with bad attributes", "Security.MOR.MorLockV1.BadAttributes", MorLockShouldNotSetBadAttributes, MorLockShouldNotBeSet, NULL, NULL );
  //
  // NOTE: Strictly speaking, this isn't a good unit test.
  //       After this test runs, the MorLock is set and the other tests
  //       have some expectation that the lock will behave a certain way.
  //       We *could* make better unit tests, but there would be a lot more
  //       reboots. So let's say this is for efficiency.
  //
  AddTestCase( MorLockV1Tests, "Should be able to set the v1 MORLock", "Security.MOR.MorLockV1.SetLock", MorLockv1ShouldBeLockable, MorLockShouldNotBeSet, NULL, NULL );
  AddTestCase( MorLockV1Tests, "Should report version correctly when locked with MORLock v1", "Security.MOR.MorLockV1.LockVersion", MorLockv1ShouldReportCorrectly, NULL, NULL, NULL );
  AddTestCase( MorLockV1Tests, "Should not be able to change the MOR control when locked with MORLock v1", "Security.MOR.MorLockV1.Lock", MorControlShouldNotChange, MorLockv1ShouldReportCorrectly, NULL, NULL );
  AddTestCase( MorLockV1Tests, "Should not be able to change the MORLock when locked with MORLock v1", "Security.MOR.MorLockV1.LockImmutable", MorLockv1ShouldNotChangeWhenLocked, MorLockv1ShouldReportCorrectly, NULL, NULL );
  AddTestCase( MorLockV1Tests, "Should not be able to delete the MORLock when locked with MORLock v1", "Security.MOR.MorLockV1.LockDelete", MorLockv1ShouldNotBeDeleteable, MorLockv1ShouldReportCorrectly, NULL, NULL );
  AddTestCase( MorLockV1Tests, "MORLock v1 should clear after reboot", "Security.MOR.MorLockV1.ClearOnReboot", MorLockShouldClearAfterReboot, MorLockv1ShouldReportCorrectly, NULL, NULL );

  //
  // Populate the MorLockV2Tests Unit Test Suite.
  //
  Status = CreateUnitTestSuite( &MorLockV2Tests, Fw, "MORLock v2 Tests", "Security.MOR.LockV2", NULL, NULL );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MorLockV2Tests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase( MorLockV2Tests, "Should be able to change MOR control when not locked", "Security.MOR.LockV2.MorMutableWhenNotLocked", MorControlShouldChangeWhenNotLocked, MorControlVariableShouldBeCorrect, NULL, NULL );
  AddTestCase( MorLockV2Tests, "Should not be able to set MORLock v2 with buffer too small", "Security.MOR.LockV2.LockValueTooSmall", MorLockv2ShouldNotSetSmallBuffer, MorLockShouldNotBeSet, NULL, NULL );
  AddTestCase( MorLockV2Tests, "Should not be able to set MORLock v2 with buffer too large", "Security.MOR.LockV2.LockValueTooLarge", MorLockv2ShouldNotSetLargeBuffer, MorLockShouldNotBeSet, NULL, NULL );
  AddTestCase( MorLockV2Tests, "Should not be able to set MORLock v2 without a key", "Security.MOR.LockV2.LockWithoutKey", MorLockv2ShouldNotSetNoBuffer, MorLockShouldNotBeSet, NULL, NULL );
  AddTestCase( MorLockV2Tests, "Should not be able to set MORLock v2 with bad attributes", "Security.MOR.LockV2.BadAttributes", MorLockShouldNotSetBadAttributes, MorLockShouldNotBeSet, NULL, NULL );
  //
  // NOTE: Strictly speaking, this isn't a good unit test.
  //       After this test runs, the MorLock is set and the other tests
  //       have some expectation that the lock will behave a certain way.
  //       We *could* make better unit tests, but there would be a lot more
  //       reboots. So let's say this is for efficiency.
  //
  AddTestCase( MorLockV2Tests, "Should be able to set the v2 MORLock", "Security.MOR.LockV2.SetLock", MorLockv2ShouldBeLockable, MorLockShouldNotBeSet, NULL, NULL );
  AddTestCase( MorLockV2Tests, "Should report version correctly when locked with MORLock v2", "Security.MOR.LockV2.LockVersion", MorLockv2ShouldReportCorrectly, NULL, NULL, NULL );
  AddTestCase( MorLockV2Tests, "Should only return one byte when reading MORLock v2", "Security.MOR.LockV2.LockSize", MorLockv2ShouldOnlyReturnOneByte, MorLockv2ShouldReportCorrectly, NULL, NULL );
  AddTestCase( MorLockV2Tests, "Should not return the key contents when locked with MORLock v2", "Security.MOR.LockV2.LockDataProtection", MorLockv2ShouldNotReturnKey, MorLockv2ShouldReportCorrectly, NULL, NULL );
  AddTestCase( MorLockV2Tests, "Should not be able to change the MOR control when locked with MORLock v2", "Security.MOR.LockV2.Lock", MorControlShouldNotChange, MorLockv2ShouldReportCorrectly, NULL, NULL );
  AddTestCase( MorLockV2Tests, "Should not be able to change the key when locked with MORLock v2", "Security.MOR.LockV2.LockImmutable", MorLockv2ShouldNotChangeWhenLocked, MorLockv2ShouldReportCorrectly, NULL, NULL );
  AddTestCase( MorLockV2Tests, "Should not be able to change to MORLock v1 when locked with MORLock v2", "Security.MOR.LockV2.ChangeToV1Lock", MorLockv2ShouldNotChangeTov1, MorLockv2ShouldReportCorrectly, NULL, NULL );
  AddTestCase( MorLockV2Tests, "Should not be able to delete the MORLock when locked with MORLock v2", "Security.MOR.LockV2.LockDelete", MorLockv2ShouldNotBeDeleteable, MorLockv2ShouldReportCorrectly, NULL, NULL );
  AddTestCase( MorLockV2Tests, "MORLock v2 should clear after reboot", "Security.MOR.MorLockV2.ClearOnReboot", MorLockShouldClearAfterReboot, MorLockv2ShouldReportCorrectly, NULL, NULL );
  //
  // End of tests that assume precedence.
  // From here on, each test is isolated and will clean up after itself.
  //
  AddTestCase( MorLockV2Tests, "MORLock v2 should clear with a correct key", "Security.MOR.MorLockV2.LockUnlock", MorLockv2ShouldClearWithCorrectKey, MorLockShouldNotBeSet, UnitTestCleanupReboot, NULL );
  AddTestCase( MorLockV2Tests, "MORLock v2 should not clear with an incorrect key", "Security.MOR.MorLockV2.LockKeyValueWrongUnlock", MorLockv2ShouldNotClearWithWrongKey, MorLockShouldNotBeSet, UnitTestCleanupReboot, NULL );
  AddTestCase( MorLockV2Tests, "Should be able to change MOR control after setting and clearing MORLock v2", "Security.MOR.MorLockV2.Unlock", MorLockv2ShouldReleaseMorControlAfterClear, MorLockShouldNotBeSet, UnitTestCleanupReboot, NULL );
  AddTestCase( MorLockV2Tests, "Should be able to change keys by setting, clearing, and setting MORLock v2", "Security.MOR.MorLockV2.LockUnlockLock", MorLockv2ShouldSetClearSet, MorLockShouldNotBeSet, UnitTestCleanupReboot, NULL );

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites( Fw );

EXIT:

  if (Fw)
  {
    FreeUnitTestFramework( Fw );
  }

  return Status;
}
