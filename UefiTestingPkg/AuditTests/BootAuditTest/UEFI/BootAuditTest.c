/** @file
  This application will test the extra boot options that are not compatible
  with the MU Locking location of Ready To Boot.

  This means that the following are required:

  1. EFI_OS_INDICATIONS_START_PLATFORM_RECOVERY is not in EFI_OS_INDICATIONS_SUPPORT_VARIABLE_NAME
  2. EFI_OS_INDICATIONS_START_OS_RECOVERY is not in EFI_OS_INDICATIONS_SUPPORT_VARIABLE_NAME
  3. EFI_BOOT_OPTION_SUPPORT_SYSPREP is not in EFI_BOOT_OPTION_SUPPORT_VARIABLE_NAME
  4. The following variables cannot be written, and must not exist.
     SysPrepOrder
     SysPrep####
     PlatformRecovery####
     DriverOrder
     Driver####

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

  **/

#include <Uefi.h>

#include <Guid/GlobalVariable.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UnitTestLib.h>

#define UNIT_TEST_APP_NAME        "BootAuditTest"
#define UNIT_TEST_APP_VERSION     "1.0"


typedef struct {
    CHAR16               *TestName;
    UINT32                Attributes;
    CHAR16               *VariableDeleteName;
    EFI_STATUS            ExpectedStatus1a;
    EFI_STATUS            ExpectedStatus1b;
    EFI_STATUS            ExpectedStatus2;
} BASIC_TEST_CONTEXT;


//*----------------------------------------------------------------------------------*
//* Test Contexts                                                                    *
//*----------------------------------------------------------------------------------*
static BASIC_TEST_CONTEXT mTest1  = { EFI_OS_INDICATIONS_SUPPORT_VARIABLE_NAME, EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, NULL, EFI_SUCCESS, 0, EFI_WRITE_PROTECTED };
static BASIC_TEST_CONTEXT mTest2  = { EFI_SYS_PREP_ORDER_VARIABLE_NAME,         EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, NULL, EFI_NOT_FOUND, 0, EFI_WRITE_PROTECTED };
static BASIC_TEST_CONTEXT mTest3  = { L"SysPrep0001",                           EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, NULL, EFI_NOT_FOUND, 0, EFI_WRITE_PROTECTED };
static BASIC_TEST_CONTEXT mTest4  = { L"PlatformRecovery0001",                                              EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, NULL, EFI_NOT_FOUND, 0, EFI_WRITE_PROTECTED };
static BASIC_TEST_CONTEXT mTest5  = { EFI_DRIVER_ORDER_VARIABLE_NAME,           EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, NULL, EFI_NOT_FOUND, 0, EFI_WRITE_PROTECTED };
static BASIC_TEST_CONTEXT mTest6  = { L"Driver0001",                            EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, NULL, EFI_NOT_FOUND, 0, EFI_WRITE_PROTECTED };

/*
    CleanUpTestContext

    Cleans up after a test case.  Free any allocated buffers if a test
    takes the error exit.

*/
STATIC
VOID
EFIAPI
CleanUpTestContext (
    IN UNIT_TEST_CONTEXT           Context
  ) {
    BASIC_TEST_CONTEXT *Btc;
    EFI_STATUS          Status;

    Btc = (BASIC_TEST_CONTEXT *) Context;

    if (NULL != Btc->VariableDeleteName) {
        Status = gRT->SetVariable (
                  Btc->VariableDeleteName,
                  &gEfiGlobalVariableGuid,
                  0,
                  0,
                  NULL);
    }

    Btc->VariableDeleteName = NULL;
}

/*
    OsIndicationsSupport testing

*/
static
UNIT_TEST_STATUS
EFIAPI
OsIndicationsSupportTest (
    IN UNIT_TEST_CONTEXT           Context
  ) {
    UINT32              Attributes;
    BASIC_TEST_CONTEXT *Btc;
    UINTN               DataSize;
    EFI_STATUS          Status;
    EFI_STATUS          ExpectedStatus;
    UINT64              OsIndicationsSupported;

    Btc = (BASIC_TEST_CONTEXT *) Context;

    UT_ASSERT_STATUS_EQUAL (EFI_SUCCESS, Btc->ExpectedStatus1b);
    ExpectedStatus = Btc->ExpectedStatus1a;

    DataSize = sizeof(OsIndicationsSupported);
    Status = gRT->GetVariable (Btc->TestName,
                              &gEfiGlobalVariableGuid,
                              &Attributes,
                              &DataSize,
                              &OsIndicationsSupported);


    UT_LOG_INFO ("\nGetVariable of %s. Return code %r, expected %r\n", Btc->TestName, Status, ExpectedStatus);
    UT_ASSERT_STATUS_EQUAL (Status, ExpectedStatus);

    UT_LOG_INFO ("\n%s value is %lx\n", EFI_OS_INDICATIONS_SUPPORT_VARIABLE_NAME, OsIndicationsSupported);

    UT_ASSERT_EQUAL (OsIndicationsSupported & EFI_OS_INDICATIONS_START_PLATFORM_RECOVERY, 0);

    // EFI_OS_INDICATIONS_START_OS_RECOVERY is not defined in TianoCore yet
    #define EFI_OS_INDICATIONS_START_OS_RECOVERY 0x0000000000000020
    UT_ASSERT_EQUAL (OsIndicationsSupported & EFI_OS_INDICATIONS_START_OS_RECOVERY, 0);
    UT_ASSERT_EQUAL (Attributes, Btc->Attributes );

    Status = gRT->SetVariable ( Btc->TestName,
                               &gEfiGlobalVariableGuid,
                                Btc->Attributes,
                                sizeof(OsIndicationsSupported),
                               &OsIndicationsSupported);

    UT_LOG_INFO ("\nSetVariable of %s.  Return code %r, expected %r\n", Btc->TestName, Status, Btc->ExpectedStatus2);
    UT_ASSERT_STATUS_EQUAL (Status, Btc->ExpectedStatus2);

    return UNIT_TEST_PASSED;
}

/*
    SysPrep testing

*/
static
UNIT_TEST_STATUS
EFIAPI
VariableLockedTestTest (
    IN UNIT_TEST_CONTEXT           Context
  ) {
    BASIC_TEST_CONTEXT *Btc;
    EFI_STATUS          Status;
    UINTN               DataSize;
    UINT32              Attributes;
    UINT64              Data = 0x1122334455667788;
    Btc = (BASIC_TEST_CONTEXT *) Context;

    DataSize = 0;
    Status = gRT->GetVariable (Btc->TestName,
                               &gEfiGlobalVariableGuid,
                               &Attributes,
                               &DataSize,
                               NULL );

    UT_LOG_INFO ("\nGetVariable of %s.  Return code %r, expected %r\n", Btc->TestName, Status, Btc->ExpectedStatus1a);
    UT_ASSERT_STATUS_EQUAL (Status, Btc->ExpectedStatus1a);

    DataSize = sizeof(Data);

    Btc->VariableDeleteName = Btc->TestName;
    Status = gRT->SetVariable ( Btc->TestName,
                               &gEfiGlobalVariableGuid,
                                Btc->Attributes,
                                DataSize,
                                &Data );

    UT_LOG_INFO ("\nSetVariable of %s.  Return code %r, expected %r\n", Btc->TestName, Status, Btc->ExpectedStatus2);
    UT_ASSERT_STATUS_EQUAL (Status, Btc->ExpectedStatus2);

    // If here, no variable written. Skip delete in testcase cleanup.

    Btc->VariableDeleteName = NULL;

    return UNIT_TEST_PASSED;
}


/**
  BootAuditTestAppEntry

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
BootAuditTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    UNIT_TEST_FRAMEWORK_HANDLE  Fw = NULL;
    UNIT_TEST_SUITE_HANDLE      BootAuditTests;
    EFI_STATUS                  Status;

    DEBUG(( DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

    //
    // Start setting up the test framework for running the tests.
    //
    Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
    if (EFI_ERROR( Status )) {
        DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
        goto EXIT;
    }

    //
    // Populate the DeviceId Library Test Suite.
    //
    Status = CreateUnitTestSuite( &BootAuditTests, Fw, "Test all thing for automatic boot options", "BootAudit.Test", NULL, NULL);
    if (EFI_ERROR( Status )) {
        DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for BootAuditTests\n"));
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;
    }

    //-----------Suite-----------Description-------------Class-------------------Test Function-------------Pre---Clean-Context
    AddTestCase( BootAuditTests, "OsIndicationsSupport", "OsIndicationsSupport", OsIndicationsSupportTest, NULL, CleanUpTestContext, &mTest1);
    AddTestCase( BootAuditTests, "SysPrepOrder",         "Sysprep",              VariableLockedTestTest,   NULL, CleanUpTestContext, &mTest2);
    AddTestCase( BootAuditTests, "SysPrep0001",          "Sysprep",              VariableLockedTestTest,   NULL, CleanUpTestContext, &mTest3);
    AddTestCase( BootAuditTests, "PlatformRecovery0001", "PlatformRecovery",     VariableLockedTestTest,   NULL, CleanUpTestContext, &mTest4);
    AddTestCase( BootAuditTests, "DriverOrder",          "Driver",               VariableLockedTestTest,   NULL, CleanUpTestContext, &mTest5);
    AddTestCase( BootAuditTests, "Driver0001",           "Driver",               VariableLockedTestTest,   NULL, CleanUpTestContext, &mTest6);

    //
    // Execute the tests.
    //
    Status = RunAllTestSuites (Fw);

EXIT:
    if (Fw) {
        FreeUnitTestFramework (Fw);
    }

    return Status;
}