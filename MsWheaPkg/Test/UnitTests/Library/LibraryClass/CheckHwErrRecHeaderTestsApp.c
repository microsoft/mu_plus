/**@file CheckHwErrRecHeaderLib.c

The CheckHwErrRecHeaderLib test application

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Guid/Cper.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UnitTestLib.h>
#include <Library/CheckHwErrRecHeaderLib.h>

#define UNIT_TEST_APP_NAME        "CheckHwErrRecHeader Tests App"
#define UNIT_TEST_APP_VERSION     "1.0"

#define MaxNumSections        5
#define BaseSecDescLength     128
#define BaseSecCount          1
#define BaseSecHeadLength     72
#define BaseSecLength         64
#define BaseSecOffset         200

#pragma pack(1)

typedef struct {
  BOOLEAN     Valid;
  UINT32      SectionLength;
  UINT32      SectionOffset;
} PARAMS_CONTEXT;

typedef struct {
    UINTN               Size;
    UINT32              RecordLength;
    UINT16              SectionCount;
    PARAMS_CONTEXT      Section1;
    PARAMS_CONTEXT      Section2;
    PARAMS_CONTEXT      Section3;
    PARAMS_CONTEXT      Section4;
    PARAMS_CONTEXT      Section5; //Testing 5 sections should be more than enough
    BOOLEAN             ExpectedResult;
} BASIC_TEST_CONTEXT;

#pragma pack()

EFI_COMMON_ERROR_RECORD_HEADER        *Err = NULL;

//PASS
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest1  = {
                                    BaseSecDescLength + (BaseSecLength * 1) + (BaseSecHeadLength * 1),
                                    BaseSecDescLength + (BaseSecLength * 1) + (BaseSecHeadLength * 1),
                                    BaseSecCount * 1,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 1) + (BaseSecLength * 0)},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    TRUE
                                  };

//FAIL BECAUSE SIZE AND LENGTH DO NOT MATCH
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest2  = {
                                    BaseSecDescLength + (BaseSecLength * 1) + (BaseSecHeadLength * 1),
                                    BaseSecDescLength + (BaseSecLength * 1) + (BaseSecHeadLength * 1) + 1,
                                    BaseSecCount * 1,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 1) + (BaseSecLength * 0)},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    FALSE
                                  };

//PASS
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest3  = {
                                    BaseSecDescLength + (BaseSecLength * 2) + (BaseSecHeadLength * 2),
                                    BaseSecDescLength + (BaseSecLength * 2) + (BaseSecHeadLength * 2),
                                    BaseSecCount * 2,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 2) + (BaseSecLength * 0)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 2) + (BaseSecLength * 1)},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    TRUE
                                  };

//FAIL BECAUSE DECLARED SECTION 2 SIZE AND OFFSET WOULD RUN OFF THE END OF THE STRUCTURE
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest4  = {
                                    BaseSecDescLength + (BaseSecLength * 2) + (BaseSecHeadLength * 2),
                                    BaseSecDescLength + (BaseSecLength * 2) + (BaseSecHeadLength * 2),
                                    BaseSecCount * 2,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 2) + (BaseSecLength * 0)},
                                    {TRUE, BaseSecLength + 1, BaseSecDescLength + (BaseSecHeadLength * 2) + (BaseSecLength * 1)},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    FALSE
                                  };

//FAIL BECAUSE SIZE ISN'T LARGE ENOUGH TO HOLD THE SPECIFIED NUMBER OF HEADERS
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest5  = {
                                    BaseSecDescLength + (BaseSecLength * 0) + (BaseSecHeadLength * 1),
                                    BaseSecDescLength + (BaseSecLength * 0) + (BaseSecHeadLength * 1),
                                    BaseSecCount * 2,
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    FALSE
                                  };

//PASS
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest6  = {
                                    BaseSecDescLength + (BaseSecLength * 3) + (BaseSecHeadLength * 3),
                                    BaseSecDescLength + (BaseSecLength * 3) + (BaseSecHeadLength * 3),
                                    BaseSecCount * 3,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 3) + (BaseSecLength * 0)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 3) + (BaseSecLength * 1)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 3) + (BaseSecLength * 2)},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    TRUE
                                  };

//FAIL BECAUSE SECTION 2 AND 3 ARE NOT CONTIGUOUS
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest7  = {
                                    BaseSecDescLength + (BaseSecLength * 3) + (BaseSecHeadLength * 3),
                                    BaseSecDescLength + (BaseSecLength * 3) + (BaseSecHeadLength * 3),
                                    BaseSecCount * 3,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 3) + (BaseSecLength * 0)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 3) + (BaseSecLength * 1)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 3) + (BaseSecLength * 2) + 1},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    FALSE
                                  };

//FAIL BECAUSE NOT ENOUGH SPACE FOR THE SPECIFIED NUMBER OF SECTIONS
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest8  = {
                                    BaseSecDescLength + (BaseSecLength * 2) + (BaseSecHeadLength * 2),
                                    BaseSecDescLength + (BaseSecLength * 2) + (BaseSecHeadLength * 2),
                                    BaseSecCount * 3,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 3) + (BaseSecLength * 0)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 3) + (BaseSecLength * 1)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 3) + (BaseSecLength * 2)},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    FALSE
                                  };

//PASS
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest9  = {
                                    BaseSecDescLength + (BaseSecLength * 3) + (BaseSecHeadLength * 3) + 64,
                                    BaseSecDescLength + (BaseSecLength * 3) + (BaseSecHeadLength * 3) + 64,
                                    BaseSecCount * 3,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 3) + (BaseSecLength * 0)},
                                    {TRUE, BaseSecLength + 64, BaseSecDescLength + (BaseSecHeadLength * 3) + (BaseSecLength * 1)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 3) + (BaseSecLength * 2) + 64},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    TRUE
                                  };

//PASS
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest10 = {
                                    BaseSecDescLength + (BaseSecLength * 4) + (BaseSecHeadLength * 4) + 64,
                                    BaseSecDescLength + (BaseSecLength * 4) + (BaseSecHeadLength * 4) + 64,
                                    BaseSecCount * 4,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 4) + (BaseSecLength * 0)},
                                    {TRUE, BaseSecLength + 64, BaseSecDescLength + (BaseSecHeadLength * 4) + (BaseSecLength * 1)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 4) + (BaseSecLength * 2) + 64},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 4) + (BaseSecLength * 3) + 64},
                                    {FALSE, 0, 0},
                                    TRUE
                                  };

//PASS
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest11 = {
                                    BaseSecDescLength + (BaseSecLength * 5) + (BaseSecHeadLength * 5) + 64,
                                    BaseSecDescLength + (BaseSecLength * 5) + (BaseSecHeadLength * 5) + 64,
                                    BaseSecCount * 5,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 0)},
                                    {TRUE, BaseSecLength + 64, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 1)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 2) + 64},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 3) + 64},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 4) + 64},
                                    TRUE
                                  };
//FAIL BECAUSE OF UINTN OVERFLOW
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest12 = {
                                    BaseSecDescLength + (BaseSecLength * 5) + (BaseSecHeadLength * 5) + 64,
                                    BaseSecDescLength + (BaseSecLength * 5) + (BaseSecHeadLength * 5) + 64,
                                    BaseSecCount * 5,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 0)},
                                    {TRUE, BaseSecLength + 64, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 1)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 2) + 64},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 3) + 64},
                                    {TRUE, MAX_UINT32, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 4) + 64},
                                    FALSE
                                  };

//FAIL BECAUSE SIZE IS GREATER THAN THE SIZE OF HEADERS + SECTIONS
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest13 = {
                                    BaseSecDescLength + (BaseSecLength * 5) + (BaseSecHeadLength * 5) + 1,
                                    BaseSecDescLength + (BaseSecLength * 5) + (BaseSecHeadLength * 5) + 1,
                                    BaseSecCount * 5,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 0)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 1)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 2)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 3)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 4)},
                                    FALSE
                                  };

//FAIL BECAUSE THERE IS SPACE BETWEEN SECTIONS 4 AND 5
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest14 = {
                                    BaseSecDescLength + (BaseSecLength * 5) + (BaseSecHeadLength * 5) + 1,
                                    BaseSecDescLength + (BaseSecLength * 5) + (BaseSecHeadLength * 5) + 1,
                                    BaseSecCount * 5,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 0)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 1)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 2)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 3)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 4) + 1},
                                    FALSE
                                  };

//FAIL BECAUSE SECTIONS 4 AND 5 INTERSECT
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest15 = {
                                    BaseSecDescLength + (BaseSecLength * 5) + (BaseSecHeadLength * 5) + 1,
                                    BaseSecDescLength + (BaseSecLength * 5) + (BaseSecHeadLength * 5) + 1,
                                    BaseSecCount * 5,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 0)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 1)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 2)},
                                    {TRUE, BaseSecLength + 1, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 3)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 5) + (BaseSecLength * 4)},
                                    FALSE
                                  };

//FAIL BECAUSE LENGTH OF SECTION 5 IS ZERO
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest16 = {
                                    BaseSecDescLength + (BaseSecLength * 2) + (BaseSecHeadLength * 2),
                                    BaseSecDescLength + (BaseSecLength * 2) + (BaseSecHeadLength * 2),
                                    BaseSecCount * 2,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 2) + (BaseSecLength * 0)},
                                    {TRUE, 0, BaseSecDescLength + (BaseSecHeadLength * 2) + (BaseSecLength * 1)},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    FALSE
                                  };

//FAIL BECAUSE SECTION 2 OFFSET IS BEFORE THE SECTION HEAD ITSELF
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest17 = {
                                    BaseSecDescLength + (BaseSecLength * 2) + (BaseSecHeadLength * 2),
                                    BaseSecDescLength + (BaseSecLength * 2) + (BaseSecHeadLength * 2),
                                    BaseSecCount * 2,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 2) + (BaseSecLength * 0)},
                                    {TRUE, BaseSecLength, BaseSecDescLength},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    FALSE
                                  };

//FAIL BECAUSE SECTION 2 OFFSET FALLS WITHIN THE SECTION 2 HEADER
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest18 = {
                                    BaseSecDescLength + (BaseSecLength * 2) + (BaseSecHeadLength * 2),
                                    BaseSecDescLength + (BaseSecLength * 2) + (BaseSecHeadLength * 2),
                                    BaseSecCount * 2,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 2) + (BaseSecLength * 0)},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 1) + 20},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    FALSE
                                  };

//FAIL BECAUSE SECTION 1 OFFSET FALLS WITHIN THE SECTION 1 HEADER
STATIC BASIC_TEST_CONTEXT  mBasicRecordTest19 = {
                                    BaseSecDescLength + (BaseSecLength * 2) + (BaseSecHeadLength * 2),
                                    BaseSecDescLength + (BaseSecLength * 2) + (BaseSecHeadLength * 2),
                                    BaseSecCount * 2,
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 0) + 20},
                                    {TRUE, BaseSecLength, BaseSecDescLength + (BaseSecHeadLength * 2) + (BaseSecLength * 1)},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    {FALSE, 0, 0},
                                    FALSE
                                  };
///================================================================================================
///================================================================================================
///
/// HELPER FUNCTIONS
///
///================================================================================================
///================================================================================================

/**
Simple clean up method to make sure tests clean up even if interrupted and fail in the middle.
**/
STATIC
VOID
EFIAPI
CleanupErr (
    IN UNIT_TEST_CONTEXT           Context
  ) {

    if (Err != NULL) {
     FreePool(Err);
    }
}

///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================
STATIC
UNIT_TEST_STATUS
EFIAPI
ErrorRecordHeaderTest(IN UNIT_TEST_CONTEXT           Context)
{

  EFI_ERROR_SECTION_DESCRIPTOR          *SectionHeader;
  PARAMS_CONTEXT                        *CurrentSection;
  UNIT_TEST_STATUS                       Status           = UNIT_TEST_ERROR_TEST_FAILED;
  BASIC_TEST_CONTEXT *Btc = (BASIC_TEST_CONTEXT *)Context;

  Err = AllocateZeroPool(Btc->Size);

  Err->RecordLength = Btc->RecordLength;
  Err->SectionCount = Btc->SectionCount;
  Err->SignatureStart = EFI_ERROR_RECORD_SIGNATURE_START;

  for(UINT8 i = 0; i < MaxNumSections; i++)
  {

    CurrentSection = (PARAMS_CONTEXT *)(&(Btc->Section1.Valid) + (sizeof(PARAMS_CONTEXT) * i));

    if(CurrentSection->Valid)
    {
      SectionHeader = (((EFI_ERROR_SECTION_DESCRIPTOR *)(Err + 1)) + i);

      SectionHeader->SectionLength = CurrentSection->SectionLength;
      SectionHeader->SectionOffset = CurrentSection->SectionOffset;

    }
  }



  if(ValidateCperHeader(Err, Btc->Size) == Btc->ExpectedResult)
  {
    UT_LOG_ERROR("UNIT TEST PASSED");
    Status = UNIT_TEST_PASSED;
  } else
  {
    UT_LOG_ERROR("UNIT TEST FAILED");
  }

  FreePool(Err);
  Err = NULL;

  return Status;
}

///================================================================================================
///================================================================================================
///
/// TEST ENGINE
///
///================================================================================================
///================================================================================================

/**
  Unit Tests

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
CheckHwErrRecHeaderTestsEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    EFI_STATUS                  Status;
    UNIT_TEST_FRAMEWORK_HANDLE  Fw = NULL;
    UNIT_TEST_SUITE_HANDLE      ErrorRecordTests;

    DEBUG(( DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

    //
    // Start setting up the test framework for running the tests.
    //
    Status = InitUnitTestFramework( &Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION );
    if (EFI_ERROR( Status )) {
        DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
        goto EXIT;
    }

    //
    // Populate the test suite.
    //
    Status = CreateUnitTestSuite( &ErrorRecordTests, Fw, "Test Error Record Header Validation", "ErrorRecord.tests", NULL, NULL );
    if (EFI_ERROR( Status )){
        DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for ErrorRecord Tests\n"));
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;
    }

// --------------Suite-------------Description------------------Class Name-------------Function---------------Pre---Post--------Context-----------
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test1",  ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest1);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test2",  ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest2);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test3",  ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest3);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test4",  ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest4);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test5",  ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest5);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test6",  ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest6);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test7",  ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest7);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test8",  ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest8);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test9",  ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest9);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test10", ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest10);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test11", ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest11);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test12", ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest12);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test13", ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest13);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test14", ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest14);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test15", ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest15);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test16", ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest16);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test17", ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest17);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test18", ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest18);
    AddTestCase( ErrorRecordTests, "Test Error Record Header", "ErrorRecord.Test19", ErrorRecordHeaderTest, NULL, CleanupErr, &mBasicRecordTest19);

    //
    // Execute the tests.
    //
    Status = RunAllTestSuites( Fw );

EXIT:
    if (Fw) {
      FreeUnitTestFramework( Fw );
    }

    return Status;
}