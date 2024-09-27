/** @file AdvancedLoggerMmCoreGoogleTest.cpp

    This file contains the unit tests for the Advanced Logger MM Core Library.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
#include <GoogleTest/Library/MockHobLib.h>
#include <GoogleTest/Library/MockAdvancedLoggerHdwPortLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
  #include <AdvancedLoggerInternal.h>
  #include <Protocol/AdvancedLogger.h>
  #include <AdvancedLoggerInternalProtocol.h>
  #include <Library/SynchronizationLib.h>       // to mock (MU_BASECORE MdePkg)
  #include "../../AdvancedLoggerCommon.h"

  extern ADVANCED_LOGGER_INFO  *mLoggerInfo;
  extern UINT32                mBufferSize;
  extern EFI_PHYSICAL_ADDRESS  mMaxAddress;
  extern BOOLEAN               mInitialized;

  // Static function declaration
  BOOLEAN
  ValidateInfoBlock (
    VOID
    );
}

using namespace testing;

/**
  Test class for AdvancedLoggerMmCore
**/
class AdvancedLoggerMmCoreTest : public Test {
protected:
  UINTN DebugLevel;
  EFI_HANDLE ImageHandle;
  EFI_SYSTEM_TABLE SystemTable;
  BOOLEAN Status;
  ADVANCED_LOGGER_INFO testLoggerInfo;
  // StrictMock<MockHobLib> gHobLib;
  // StrictMock<MockAdvancedLoggerHdwPortLib> gALHdwPortLib;

  void
  SetUp (
    ) override
  {
    mLoggerInfo                     = NULL;
    DebugLevel                      = DEBUG_ERROR;
    ImageHandle                     = (EFI_HANDLE)0x12345678;
    testLoggerInfo.Signature        = ADVANCED_LOGGER_SIGNATURE;
    testLoggerInfo.Version          = ADVANCED_LOGGER_VERSION;
    testLoggerInfo.LogBufferOffset  = (ALIGN_VALUE (sizeof (testLoggerInfo), 8));
    testLoggerInfo.LogCurrentOffset = (ALIGN_VALUE (sizeof (testLoggerInfo), 8));
  }
};

//
// Test ValidateInfoBlock
//
TEST_F (AdvancedLoggerMmCoreTest, AdvLoggerGetInfoFail) {
  // NULL LoggerInfo
  Status = ValidateInfoBlock ();
  EXPECT_EQ (Status, FALSE);
  mLoggerInfo = &testLoggerInfo;

  // Invalid Signature
  mLoggerInfo->Signature = SIGNATURE_32 ('T', 'E', 'S', 'T');
  Status                 = ValidateInfoBlock ();
  EXPECT_EQ (Status, FALSE);
  mLoggerInfo->Signature = ADVANCED_LOGGER_SIGNATURE;

  // Invalid Version
  mLoggerInfo->Version = (UINT32)ADVANCED_LOGGER_VERSION + 1;
  Status               = ValidateInfoBlock ();
  EXPECT_EQ (Status, FALSE);
  mLoggerInfo->Version = ADVANCED_LOGGER_VERSION;

  // Invalid Buffer Offset
  mLoggerInfo->LogBufferOffset = (UINT32)0;
  Status                       = ValidateInfoBlock ();
  EXPECT_EQ (Status, FALSE);
  mLoggerInfo->LogBufferOffset = (ALIGN_VALUE (sizeof (testLoggerInfo), 8));

  // Invalid Current Offset
  mLoggerInfo->LogCurrentOffset = (UINT32)0;
  Status                        = ValidateInfoBlock ();
  EXPECT_EQ (Status, FALSE);
  mLoggerInfo->LogCurrentOffset = (ALIGN_VALUE (sizeof (testLoggerInfo), 8));

  // Invalid Buffer Size
  mLoggerInfo->LogBufferSize = (UINT32)0;
  mBufferSize                = (UINT32)0x1000;
  Status                     = ValidateInfoBlock ();
  EXPECT_EQ (Status, FALSE);
}

/*/* Commented out, need mock libraries to be implemented.
// Test AdvancedLoggerGetLoggerInfo NULL HOB
TEST_F (AdvancedLoggerMmCoreTest, AdvLoggerGetInfoNullHob) {
  // PcdAdvancedLoggerFixedInRAM is FALSE, so expect to get the logger info from the HOB
  // GetFirstGuidHob and GetNextGuidHob are not mocked
  EXPECT_CALL (
    gHobLib,
    GetFirstGuidHob (
      BufferEq (&gAdvancedLoggerHobGuid, sizeof (EFI_GUID))
      )
    )
    .WillOnce (
       Return (NULL)
       );

  mLoggerInfo = AdvancedLoggerGetLoggerInfo ();

  EXPECT_EQ (mLoggerInfo, nullptr);
}

// Test AdvancedLoggerGetLoggerInfo Success
TEST_F (AdvancedLoggerMmCoreTest, AdvLoggerGetInfoSuccess) {
  EXPECT_CALL (
    gHobLib,
    GetFirstGuidHob (
      BufferEq (&gAdvancedLoggerHobGuid, sizeof (EFI_GUID))
      )
    )
    .WillOnce (
       Return (NULL) // Need to mock the HOB to return a valid logger info

       );

  EXPECT_CALL (
    gALHdwPortLib,
    AdvancedLoggerHdwPortInitialize ()
    )
    .WillOnce (
       Return (EFI_SUCCESS)
       );

  mLoggerInfo = AdvancedLoggerGetLoggerInfo ();
  EXPECT_NE (mLoggerInfo, nullptr);

  // expect mLoggerInfo->Signature to be ADVANCED_LOGGER_SIGNATURE
  // expect mMAXAddress to be 0x1000
}

// Test  Advanced Logger constructor
TEST_F (AdvancedLoggerMmCoreTest, AdvLoggerContructorSuccess) {
  MmCoreAdvancedLoggerLibConstructor (ImageHandle, SystemTable);
}
*/
int
main (
  int   argc,
  char  *argv[]
  )
{
  InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
