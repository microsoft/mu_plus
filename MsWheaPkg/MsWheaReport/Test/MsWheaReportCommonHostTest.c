/** @file -- MsWheaReportCommonHostTest.c
Host-based UnitTest for Common routines in the MsWheaReport driver.

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
#include <Uefi/UefiBaseType.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>

#include <Library/ReportStatusCodeLib.h>
#include <MsWheaHostTestCommon.h>

#include "../MsWheaReportCommon.h"


#ifndef INTERNAL_UNIT_TEST
#error Make sure to build this with INTERNAL_UNIT_TEST enabled! Otherwise, some important tests may be skipped!
#endif

#define UNIT_TEST_NAME     "MsWheaReport Common Unit Test"
#define UNIT_TEST_VERSION  "0.1"

typedef struct _TEST_STATUS_CODE_DATA_ANY {
  EFI_STATUS_CODE_DATA    Header;
  UINT8                   Data[1024];
} TEST_STATUS_CODE_DATA_ANY;
typedef struct _TEST_STATUS_CODE_DATA_MS_WHEA {
  EFI_STATUS_CODE_DATA              Header;
  MS_WHEA_RSC_INTERNAL_ERROR_DATA   Data;
} TEST_STATUS_CODE_DATA_MS_WHEA;
typedef struct _TEST_STATUS_CODE_DATA_MS_WHEA_PLUS {
  EFI_STATUS_CODE_DATA              Header;
  MS_WHEA_RSC_INTERNAL_ERROR_DATA   WheaData;
  EFI_GUID                          DataPlusId;
  UINT8                             DataPlus[1024];
} TEST_STATUS_CODE_DATA_MS_WHEA_PLUS;
#define     TEST_DATA_STR_1       "This is my sample data for reuse."

typedef UINT32        TEST_REPORT_FN_CHK_PARAMS;
#define   TEST_CHK_REV          BIT0
#define   TEST_CHK_PHASE        BIT1
#define   TEST_CHK_SEV          BIT2
#define   TEST_CHK_SIZE         BIT3
#define   TEST_CHK_STATUS_VAL   BIT4
#define   TEST_CHK_ADDL_INFO_1  BIT5
#define   TEST_CHK_ADDL_INFO_2  BIT6
#define   TEST_CHK_MOD_ID       BIT7
#define   TEST_CHK_LIB_ID       BIT8
#define   TEST_CHK_IHV_ID       BIT9
#define   TEST_CHK_EXTRA_SEC    BIT10

STATIC
VOID
SharedCheckParams (
  IN TEST_REPORT_FN_CHK_PARAMS              ChkParams,
  CONST IN MS_WHEA_ERROR_ENTRY_MD           *TestEntry
  )
{
  UINT32                              TestSize;
  MS_WHEA_ERROR_EXTRA_SECTION_DATA    *TestExtraSection;

  assert_non_null(TestEntry);

  // Check all the required params.
  if (ChkParams & TEST_CHK_REV) {
    assert_int_equal(TestEntry->Rev, mock());
  }
  if (ChkParams & TEST_CHK_PHASE) {
    assert_int_equal(TestEntry->Phase, mock());
  }
  if (ChkParams & TEST_CHK_SEV) {
    assert_int_equal(TestEntry->ErrorSeverity, mock());
  }
  if (ChkParams & TEST_CHK_SIZE) {
    assert_int_equal(TestEntry->PayloadSize, mock());
  }
  if (ChkParams & TEST_CHK_STATUS_VAL) {
    assert_int_equal(TestEntry->ErrorStatusValue, mock());
  }
  if (ChkParams & TEST_CHK_ADDL_INFO_1) {
    assert_int_equal(TestEntry->AdditionalInfo1, mock());
  }
  if (ChkParams & TEST_CHK_ADDL_INFO_2) {
    assert_int_equal(TestEntry->AdditionalInfo2, mock());
  }
  if (ChkParams & TEST_CHK_MOD_ID) {
    assert_memory_equal(&TestEntry->ModuleID, mock(), sizeof(EFI_GUID));
  }
  if (ChkParams & TEST_CHK_LIB_ID) {
    assert_memory_equal(&TestEntry->LibraryID, mock(), sizeof(EFI_GUID));
  }
  if (ChkParams & TEST_CHK_IHV_ID) {
    assert_memory_equal(&TestEntry->IhvSharingGuid, mock(), sizeof(EFI_GUID));
  }
  if (ChkParams & TEST_CHK_EXTRA_SEC) {
    TestSize = (UINT32)mock();
    TestExtraSection = (MS_WHEA_ERROR_EXTRA_SECTION_DATA*) TestEntry->ExtraSection;
    if (TestSize == 0) {
      assert_null(TestExtraSection);
    } else {
      assert_memory_equal(&TestExtraSection->SectionGuid, mock(), sizeof(EFI_GUID));
      assert_int_equal(TestExtraSection->DataSize, TestSize);
      assert_memory_equal(TestExtraSection->Data, mock(), TestSize);
    }
  }
}

/**
Mocked version of MsWheaESStoreEntry.

@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error metadata

@retval EFI_SUCCESS                   Operation is successful
@retval Others                        See MsWheaESFindSlot and MsWheaESWriteData for more
                                      details
**/
EFI_STATUS
EFIAPI
MsWheaESStoreEntry (
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
  )
{
  TEST_REPORT_FN_CHK_PARAMS   ChkParams;
  
  // First thing on the stack should always be the params we want to check.
  ChkParams = (TEST_REPORT_FN_CHK_PARAMS)mock();

  if (ChkParams != 0) {
    SharedCheckParams(ChkParams, MsWheaEntryMD);
  }

  return (EFI_STATUS)mock();
}

/**
Mocked version of a ReportFn()
Only checks for calls.
**/
STATIC
EFI_STATUS
EFIAPI
TestReportFnCheckCall (
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
  )
{
  return (EFI_STATUS)mock();
}
/**
Mocked version of a ReportFn()
Checks all 
**/
STATIC
EFI_STATUS
EFIAPI
TestReportFnCheckParams (
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
  )
{
  TEST_REPORT_FN_CHK_PARAMS   ChkParams;
  
  // First thing on the stack should always be the params we want to check.
  ChkParams = (TEST_REPORT_FN_CHK_PARAMS)mock();

  if (ChkParams != 0) {
    SharedCheckParams(ChkParams, MsWheaEntryMD);
  }

  return (EFI_STATUS)mock();
}

//
//
// UNIT TEST CASES
//
//

UNIT_TEST_STATUS
EFIAPI
ReportRouterCallsEsLib (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  TEST_STATUS_CODE_DATA_MS_WHEA   TestData;

  ZeroMem(&TestData, sizeof(TestData));
  TestData.Header.HeaderSize = sizeof(EFI_STATUS_CODE_DATA);
  TestData.Header.Size       = sizeof(TestData.Data);
  CopyGuid(&TestData.Header.Type, &gMsWheaRSCDataTypeGuid);

  // For PEI phase, expect a call to MsWheaESStoreEntry, but not ReportFn.
  will_return(MsWheaESStoreEntry, 0);       // ChkParams
  will_return(MsWheaESStoreEntry, EFI_SUCCESS);
  // By not pushing a 'will_return' for TestReportFnCheckCall, it will fail if called.
  UT_ASSERT_NOT_EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                                TEST_RSC_CRITICAL_5,
                                                0,
                                                &gEfiCallerIdGuid,
                                                (EFI_STATUS_CODE_DATA*)&TestData,
                                                MS_WHEA_PHASE_PEI,
                                                TestReportFnCheckCall));

  // For DXE phase, expect a call to MsWheaESStoreEntry, but not ReportFn.
  will_return(MsWheaESStoreEntry, 0);       // ChkParams
  will_return(MsWheaESStoreEntry, EFI_SUCCESS);
  // By not pushing a 'will_return' for TestReportFnCheckCall, it will fail if called.
  UT_ASSERT_NOT_EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                                TEST_RSC_CRITICAL_B,
                                                0,
                                                &gEfiCallerIdGuid,
                                                NULL,
                                                MS_WHEA_PHASE_DXE,
                                                TestReportFnCheckCall));  

  // For DXE phase, expect a call to MsWheaESStoreEntry, but not ReportFn.
  will_return(MsWheaESStoreEntry, 0);       // ChkParams
  will_return(MsWheaESStoreEntry, EFI_SUCCESS);

  ZeroMem(&TestData, sizeof(TestData));
  // TCBZ3078: Check to make sure ReportHwErrRecRouter() accepts a zeroed data field
  // By not pushing a 'will_return' for TestReportFnCheckCall, it will fail if called.
  UT_ASSERT_NOT_EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                                TEST_RSC_CRITICAL_B,
                                                0,
                                                &gEfiCallerIdGuid,
                                                (EFI_STATUS_CODE_DATA*)&TestData,
                                                MS_WHEA_PHASE_DXE,
                                                TestReportFnCheckCall));


  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
ReportRouterCallsReportFn (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  TEST_STATUS_CODE_DATA_MS_WHEA   TestData;

  ZeroMem(&TestData, sizeof(TestData));
  TestData.Header.HeaderSize = sizeof(EFI_STATUS_CODE_DATA);
  TestData.Header.Size       = sizeof(TestData.Data);
  CopyGuid(&TestData.Header.Type, &gMsWheaRSCDataTypeGuid);

  // For DXE_VAR phase, expect a call to ReportFn.
  will_return(TestReportFnCheckCall, EFI_SUCCESS);
  UT_ASSERT_NOT_EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                                TEST_RSC_CRITICAL_5,
                                                0,
                                                &gEfiCallerIdGuid,
                                                (EFI_STATUS_CODE_DATA*)&TestData,
                                                MS_WHEA_PHASE_DXE_VAR,
                                                TestReportFnCheckCall));

  // For SMM phase, expect a call to ReportFn.
  will_return(TestReportFnCheckCall, EFI_SUCCESS);
  UT_ASSERT_NOT_EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                                TEST_RSC_CRITICAL_B,
                                                0,
                                                &gEfiCallerIdGuid,
                                                NULL,
                                                MS_WHEA_PHASE_SMM,
                                                TestReportFnCheckCall));

  // For PEI and DXE, INFO should hit ReportFn.
  will_return_count(TestReportFnCheckCall, EFI_SUCCESS, 2);
  UT_ASSERT_NOT_EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_INFO,
                                                TEST_RSC_MISC_A,
                                                0,
                                                &gEfiCallerIdGuid,
                                                NULL,
                                                MS_WHEA_PHASE_PEI,
                                                TestReportFnCheckCall));
  UT_ASSERT_NOT_EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_INFO,
                                                TEST_RSC_MISC_C,
                                                0,
                                                &gEfiCallerIdGuid,
                                                NULL,
                                                MS_WHEA_PHASE_DXE,
                                                TestReportFnCheckCall));

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
ReportRouterFailsWithBadHeader (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  TEST_STATUS_CODE_DATA_MS_WHEA   TestData;

  ZeroMem(&TestData, sizeof(TestData));
  TestData.Header.HeaderSize = sizeof(EFI_STATUS_CODE_DATA) + 1;
  TestData.Header.Size       = sizeof(TestData.Data) - 1;
  CopyGuid(&TestData.Header.Type, &gMsWheaRSCDataTypeGuid);

  UT_ASSERT_TRUE(EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                                TEST_RSC_CRITICAL_5,
                                                0,
                                                &gEfiCallerIdGuid,
                                                (EFI_STATUS_CODE_DATA*)&TestData,
                                                MS_WHEA_PHASE_DXE_VAR,
                                                TestReportFnCheckCall)));

  TestData.Header.HeaderSize = sizeof(EFI_STATUS_CODE_DATA) - 1;
  TestData.Header.Size       = sizeof(TestData.Data) + 1;

  UT_ASSERT_TRUE(EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                                TEST_RSC_CRITICAL_5,
                                                0,
                                                &gEfiCallerIdGuid,
                                                (EFI_STATUS_CODE_DATA*)&TestData,
                                                MS_WHEA_PHASE_DXE_VAR,
                                                TestReportFnCheckCall)));

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
ReportRouterEnforcesDataType (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  TEST_STATUS_CODE_DATA_MS_WHEA   TestData;

  ZeroMem(&TestData, sizeof(TestData));
  TestData.Header.HeaderSize = sizeof(EFI_STATUS_CODE_DATA);
  TestData.Header.Size       = sizeof(TestData.Data);
  CopyGuid(&TestData.Header.Type, &mTestGuid1);

  UT_ASSERT_TRUE(EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                                TEST_RSC_CRITICAL_5,
                                                0,
                                                &gEfiCallerIdGuid,
                                                (EFI_STATUS_CODE_DATA*)&TestData,
                                                MS_WHEA_PHASE_DXE_VAR,
                                                TestReportFnCheckCall)));

  CopyGuid(&TestData.Header.Type, &mTestGuid3);

  UT_ASSERT_TRUE(EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                                TEST_RSC_CRITICAL_5,
                                                0,
                                                &gEfiCallerIdGuid,
                                                (EFI_STATUS_CODE_DATA*)&TestData,
                                                MS_WHEA_PHASE_DXE_VAR,
                                                TestReportFnCheckCall)));

  CopyGuid(&TestData.Header.Type, &gMsWheaRSCDataTypeGuid);

  will_return(TestReportFnCheckCall, EFI_SUCCESS);
  UT_ASSERT_NOT_EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                                TEST_RSC_CRITICAL_5,
                                                0,
                                                &gEfiCallerIdGuid,
                                                (EFI_STATUS_CODE_DATA*)&TestData,
                                                MS_WHEA_PHASE_DXE_VAR,
                                                TestReportFnCheckCall));

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
ReportRouterFailsWithLessThanWheaData (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  TEST_STATUS_CODE_DATA_MS_WHEA_PLUS  TestData;

  ZeroMem(&TestData, sizeof(TestData));
  TestData.Header.HeaderSize = sizeof(EFI_STATUS_CODE_DATA);
  TestData.Header.Size       = sizeof(TestData.WheaData) - 2;
  CopyGuid(&TestData.Header.Type, &gMsWheaRSCDataTypeGuid);

  UT_ASSERT_TRUE(EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                                TEST_RSC_CRITICAL_5,
                                                0,
                                                &gEfiCallerIdGuid,
                                                (EFI_STATUS_CODE_DATA*)&TestData,
                                                MS_WHEA_PHASE_DXE_VAR,
                                                TestReportFnCheckCall)));

  TestData.Header.Size       = sizeof(TestData.WheaData) + 2;

  will_return(TestReportFnCheckCall, EFI_SUCCESS);
  UT_ASSERT_NOT_EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                                TEST_RSC_CRITICAL_5,
                                                0,
                                                &gEfiCallerIdGuid,
                                                (EFI_STATUS_CODE_DATA*)&TestData,
                                                MS_WHEA_PHASE_DXE_VAR,
                                                TestReportFnCheckCall));

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
ReportRouterPopulateWheaData (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  TEST_STATUS_CODE_DATA_MS_WHEA   TestData;

  ZeroMem(&TestData, sizeof(TestData));
  TestData.Header.HeaderSize = sizeof(EFI_STATUS_CODE_DATA);
  TestData.Header.Size       = sizeof(TestData.Data);
  CopyGuid(&TestData.Header.Type, &gMsWheaRSCDataTypeGuid);

  TestData.Data.AdditionalInfo1 = 0xDEADBEEFDEADBEEF;
  TestData.Data.AdditionalInfo2 = 0xFEEDF00DFEEDF00D;
  CopyGuid(&TestData.Data.IhvSharingGuid, &mTestGuid1);
  CopyGuid(&TestData.Data.LibraryID, &mTestGuid2);

  //
  // Set up all our test cases.
  // First, make sure that we're checking for the params that matter.
  will_return(TestReportFnCheckParams, (TEST_CHK_PHASE |
                                        TEST_CHK_SEV |
                                        TEST_CHK_SIZE |
                                        TEST_CHK_STATUS_VAL |
                                        TEST_CHK_ADDL_INFO_1 |
                                        TEST_CHK_ADDL_INFO_2 |
                                        TEST_CHK_MOD_ID |
                                        TEST_CHK_LIB_ID |
                                        TEST_CHK_IHV_ID |
                                        TEST_CHK_EXTRA_SEC));

  // Now, set up all the data for comparison.
  will_return(SharedCheckParams, MS_WHEA_PHASE_DXE_VAR);    // TEST_CHK_PHASE
  will_return(SharedCheckParams, EFI_GENERIC_ERROR_FATAL);  // TEST_CHK_SEV
  will_return(SharedCheckParams, 0);                        // TEST_CHK_SIZE
  will_return(SharedCheckParams, TEST_RSC_CRITICAL_5);      // TEST_CHK_STATUS_VAL
  will_return(SharedCheckParams, 0xDEADBEEFDEADBEEF);       // TEST_CHK_ADDL_INFO_1
  will_return(SharedCheckParams, 0xFEEDF00DFEEDF00D);       // TEST_CHK_ADDL_INFO_2
  will_return(SharedCheckParams, &gEfiCallerIdGuid);        // TEST_CHK_MOD_ID
  will_return(SharedCheckParams, &mTestGuid2);              // TEST_CHK_LIB_ID
  will_return(SharedCheckParams, &mTestGuid1);              // TEST_CHK_IHV_ID
  will_return(SharedCheckParams, 0);                        // TEST_CHK_EXTRA_SEC (None)

  will_return(TestReportFnCheckParams, EFI_SUCCESS);
  UT_ASSERT_NOT_EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                                TEST_RSC_CRITICAL_5,
                                                0,
                                                &gEfiCallerIdGuid,
                                                (EFI_STATUS_CODE_DATA*)&TestData,
                                                MS_WHEA_PHASE_DXE_VAR,
                                                TestReportFnCheckParams));

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
ReportRouterPopulateWheaExtraData (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  TEST_STATUS_CODE_DATA_MS_WHEA_PLUS    TestData;

  ZeroMem(&TestData, sizeof(TestData));
  TestData.Header.HeaderSize = sizeof(EFI_STATUS_CODE_DATA);
  TestData.Header.Size       = sizeof(TestData.WheaData);
  CopyGuid(&TestData.Header.Type, &gMsWheaRSCDataTypeGuid);

  TestData.WheaData.AdditionalInfo1 = 0xDEADBEEFDEADBEEF;
  TestData.WheaData.AdditionalInfo2 = 0xFEEDF00DFEEDF00D;
  CopyGuid(&TestData.WheaData.IhvSharingGuid, &mTestGuid1);
  CopyGuid(&TestData.WheaData.LibraryID, &mTestGuid2);

  TestData.Header.Size += sizeof(EFI_GUID) + sizeof(TEST_DATA_STR_1);
  CopyGuid(&TestData.DataPlusId, &mTestGuid3);
  AsciiStrCpyS((CHAR8*)TestData.DataPlus, sizeof(TEST_DATA_STR_1), TEST_DATA_STR_1);

  //
  // Set up all our test cases.
  // First, make sure that we're checking for the params that matter.
  will_return(TestReportFnCheckParams, (TEST_CHK_PHASE |
                                        TEST_CHK_SEV |
                                        TEST_CHK_SIZE |
                                        TEST_CHK_STATUS_VAL |
                                        TEST_CHK_ADDL_INFO_1 |
                                        TEST_CHK_ADDL_INFO_2 |
                                        TEST_CHK_MOD_ID |
                                        TEST_CHK_LIB_ID |
                                        TEST_CHK_IHV_ID |
                                        TEST_CHK_EXTRA_SEC));

  // Now, set up all the data for comparison.
  will_return(SharedCheckParams, MS_WHEA_PHASE_DXE_VAR);    // TEST_CHK_PHASE
  will_return(SharedCheckParams, EFI_GENERIC_ERROR_FATAL);  // TEST_CHK_SEV
  will_return(SharedCheckParams, 0);                        // TEST_CHK_SIZE
  will_return(SharedCheckParams, TEST_RSC_CRITICAL_5);      // TEST_CHK_STATUS_VAL
  will_return(SharedCheckParams, 0xDEADBEEFDEADBEEF);       // TEST_CHK_ADDL_INFO_1
  will_return(SharedCheckParams, 0xFEEDF00DFEEDF00D);       // TEST_CHK_ADDL_INFO_2
  will_return(SharedCheckParams, &gEfiCallerIdGuid);        // TEST_CHK_MOD_ID
  will_return(SharedCheckParams, &mTestGuid2);              // TEST_CHK_LIB_ID
  will_return(SharedCheckParams, &mTestGuid1);              // TEST_CHK_IHV_ID

  will_return(SharedCheckParams, sizeof(TEST_DATA_STR_1));  // TEST_CHK_EXTRA_SEC
  will_return(SharedCheckParams, &mTestGuid3);  // TEST_CHK_EXTRA_SEC
  will_return(SharedCheckParams, TEST_DATA_STR_1);  // TEST_CHK_EXTRA_SEC

  will_return(TestReportFnCheckParams, EFI_SUCCESS);
  UT_ASSERT_NOT_EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                                TEST_RSC_CRITICAL_5,
                                                0,
                                                &gEfiCallerIdGuid,
                                                (EFI_STATUS_CODE_DATA*)&TestData,
                                                MS_WHEA_PHASE_DXE_VAR,
                                                TestReportFnCheckParams));

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
ReportRouterSkipExtraDataInInvalidPhases (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  TEST_STATUS_CODE_DATA_MS_WHEA_PLUS    TestData;

  ZeroMem(&TestData, sizeof(TestData));
  TestData.Header.HeaderSize = sizeof(EFI_STATUS_CODE_DATA);
  TestData.Header.Size       = sizeof(TestData.WheaData);
  CopyGuid(&TestData.Header.Type, &gMsWheaRSCDataTypeGuid);

  TestData.WheaData.AdditionalInfo1 = 0xDEADBEEFDEADBEEF;
  TestData.WheaData.AdditionalInfo2 = 0xFEEDF00DFEEDF00D;
  CopyGuid(&TestData.WheaData.IhvSharingGuid, &mTestGuid1);
  CopyGuid(&TestData.WheaData.LibraryID, &mTestGuid2);

  TestData.Header.Size += sizeof(TEST_DATA_STR_1);
  AsciiStrCpyS((CHAR8*)TestData.DataPlus, sizeof(TEST_DATA_STR_1), TEST_DATA_STR_1);

  //
  // Set up all our test cases.
  // First, make sure that we're checking for the params that matter.
  will_return(MsWheaESStoreEntry, (TEST_CHK_PHASE |
                                    TEST_CHK_SEV |
                                    TEST_CHK_SIZE |
                                    TEST_CHK_STATUS_VAL |
                                    TEST_CHK_ADDL_INFO_1 |
                                    TEST_CHK_ADDL_INFO_2 |
                                    TEST_CHK_MOD_ID |
                                    TEST_CHK_LIB_ID |
                                    TEST_CHK_IHV_ID |
                                    TEST_CHK_EXTRA_SEC));

  // Now, set up all the data for comparison.
  will_return(SharedCheckParams, MS_WHEA_PHASE_PEI);        // TEST_CHK_PHASE
  will_return(SharedCheckParams, EFI_GENERIC_ERROR_FATAL);  // TEST_CHK_SEV
  will_return(SharedCheckParams, 0);                        // TEST_CHK_SIZE
  will_return(SharedCheckParams, TEST_RSC_CRITICAL_5);      // TEST_CHK_STATUS_VAL
  will_return(SharedCheckParams, 0xDEADBEEFDEADBEEF);       // TEST_CHK_ADDL_INFO_1
  will_return(SharedCheckParams, 0xFEEDF00DFEEDF00D);       // TEST_CHK_ADDL_INFO_2
  will_return(SharedCheckParams, &gEfiCallerIdGuid);        // TEST_CHK_MOD_ID
  will_return(SharedCheckParams, &mTestGuid2);              // TEST_CHK_LIB_ID
  will_return(SharedCheckParams, &mTestGuid1);              // TEST_CHK_IHV_ID
  will_return(SharedCheckParams, 0);                        // TEST_CHK_EXTRA_SEC (None)

  will_return(MsWheaESStoreEntry, EFI_SUCCESS);
  UT_ASSERT_NOT_EFI_ERROR(ReportHwErrRecRouter(MS_WHEA_ERROR_STATUS_TYPE_FATAL,
                                                TEST_RSC_CRITICAL_5,
                                                0,
                                                &gEfiCallerIdGuid,
                                                (EFI_STATUS_CODE_DATA*)&TestData,
                                                MS_WHEA_PHASE_PEI,
                                                TestReportFnCheckParams));

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
  UNIT_TEST_SUITE_HANDLE      ReportRouterPhaseSuite;
  UNIT_TEST_SUITE_HANDLE      ReportRouterDataSuite;
  UNIT_TEST_SUITE_HANDLE      ReportRouterExtraDataSuite;

  Framework = NULL;

  DEBUG(( DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION ));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  // TODO: Test for early return on unrecognized CodeTypes.
  // TODO: Test paths with no extra data.

  //
  // Populate the ReportRouterPhaseSuite Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&ReportRouterPhaseSuite, Framework, "ReportRouterPhase", "ReportRouter.Phase", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for ReportRouterPhaseSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (ReportRouterPhaseSuite, "ReportHwErrRecRouter should call the EarlyStorage lib when phase doesn't support storage", "CallsEsLib", ReportRouterCallsEsLib, NULL, NULL, NULL);
  AddTestCase (ReportRouterPhaseSuite, "ReportHwErrRecRouter should call the ReportFn when phase does support storage", "CallsReportFn", ReportRouterCallsReportFn, NULL, NULL, NULL);

  //
  // Populate the ReportRouterDataSuite Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&ReportRouterDataSuite, Framework, "ReportRouterData", "ReportRouter.Data", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for ReportRouterDataSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (ReportRouterDataSuite, "ReportHwErrRecRouter should fail if the data header looks wrong", "FailsBadHeader", ReportRouterFailsWithBadHeader, NULL, NULL, NULL);
  AddTestCase (ReportRouterDataSuite, "ReportHwErrRecRouter should fail if passed data that isn't of type gMsWheaRSCDataTypeGuid", "EnforcesDataType", ReportRouterEnforcesDataType, NULL, NULL, NULL);
  AddTestCase (ReportRouterDataSuite, "ReportHwErrRecRouter should fail if passed data that isn't at least large enough for the basic WHEA data", "LessThanWhea", ReportRouterFailsWithLessThanWheaData, NULL, NULL, NULL);
  AddTestCase (ReportRouterDataSuite, "ReportHwErrRecRouter should populate the basic WHEA data", "PopulateWheaData", ReportRouterPopulateWheaData, NULL, NULL, NULL);

  //
  // Populate the ReportRouterExtraDataSuite Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&ReportRouterExtraDataSuite, Framework, "ReportRouterExtraData", "ReportRouter.ExtraData", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for ReportRouterExtraDataSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (ReportRouterDataSuite, "ReportHwErrRecRouter should populate the extra WHEA data", "PopulateExtraData", ReportRouterPopulateWheaExtraData, NULL, NULL, NULL);
  AddTestCase (ReportRouterDataSuite, "ReportHwErrRecRouter should not populate the extra WHEA data in invalid phases", "SkipEsPhases", ReportRouterSkipExtraDataInInvalidPhases, NULL, NULL, NULL);

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
  int argc,
  char *argv[]
  )
{
  return UefiTestMain ();
}
