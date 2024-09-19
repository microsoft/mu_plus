/** @file AdvancedLoggerPeiCoreGoogleTest.cpp

    This file contains the unit tests for the Advanced Logger PEI Core Library.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
#include <GoogleTest/Library/MockPeiServicesLib.h>
#include <GoogleTest/Library/MockMemoryAllocationLib.h>
#include <GoogleTest/Library/MockHobLib.h>
#include <GoogleTest/Library/MockAdvancedLoggerHdwPortLib.h>
#include <GoogleTest/Ppi/MockAdvancedLogger.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
  #include <AdvancedLoggerInternal.h>
  #include <Protocol/AdvancedLogger.h>
  #include <Protocol/VariablePolicy.h>         // to mock (MU_BASECORE MdeModulePkg)
  #include <AdvancedLoggerInternalProtocol.h>
  #include <Library/BaseMemoryLib.h>           // to mock (MU_BASECORE MdePkg)
  #include <Library/PcdLib.h>                  // to mock (MU_BASECORE MdePkg)
  #include <Library/SynchronizationLib.h>      // to mock (MU_BASECORE MdePkg)
  #include <Library/TimerLib.h>                // to mock (MU_BASECORE MdePkg)
  #include <Library/VariablePolicyHelperLib.h> // to mock (MU_BASECORE MdeModulePkg)
  #include "../../AdvancedLoggerCommon.h"

  #include <Core/Pei/PeiMain.h>
  #include <Library/MmUnblockMemoryLib.h>
  #include <Library/PeiServicesTablePointerLib.h> // to mock (MU_BASECORE MdePkg)
  #include <Library/PrintLib.h>

  // Static function declaration
  BOOLEAN
  ValidateInfoBlock (
    IN  ADVANCED_LOGGER_INFO  *LoggerInfo
    );
}

using namespace testing;

/**
  Test class for AdvancedLoggerPeiCore
**/
class AdvancedLoggerPeiCoreTest : public Test {
protected:
  ADVANCED_LOGGER_INFO *mLoggerInfo;
  CHAR8 SourceBuf[4096];
  UINTN DebugLevel;
  UINTN NumberOfBytes;
  EFI_HANDLE ImageHandle;
  EFI_SYSTEM_TABLE SystemTable;
  BOOLEAN status;
  ADVANCED_LOGGER_INFO testLoggerInfo;
  // StrictMock<MockHobLib> gHobLib;
  // StrictMock<MockAdvancedLoggerHdwPortLib> gALHdwPortLib;

  void
  SetUp (
    ) override
  {
    mLoggerInfo                     = NULL;
    NumberOfBytes                   = sizeof (SourceBuf);
    DebugLevel                      = DEBUG_ERROR;
    ImageHandle                     = (EFI_HANDLE)0x12345678;
    testLoggerInfo.Signature        = ADVANCED_LOGGER_SIGNATURE;
    testLoggerInfo.Version          = ADVANCED_LOGGER_VERSION;
    testLoggerInfo.LogBufferOffset  = (ALIGN_VALUE (sizeof (testLoggerInfo), 8));
    testLoggerInfo.LogCurrentOffset = (ALIGN_VALUE (sizeof (testLoggerInfo), 8));
    ZeroMem (SourceBuf, NumberOfBytes);
    CopyMem (SourceBuf, "MyUnitTest", 11);
  }
};

//
// Test ValidateInfoBlock
//
TEST_F (AdvancedLoggerPeiCoreTest, AdvLoggerValidateInfoBlock) {
  // NULL LoggerInfo
  status = ValidateInfoBlock (mLoggerInfo);
  EXPECT_EQ (status, FALSE);

  mLoggerInfo = &testLoggerInfo;

  // Success
  status = ValidateInfoBlock (mLoggerInfo);
  EXPECT_EQ (status, TRUE);

  // Invalid Signature
  mLoggerInfo->Signature = SIGNATURE_32 ('T', 'E', 'S', 'T');
  status                 = ValidateInfoBlock (mLoggerInfo);
  EXPECT_EQ (status, FALSE);
  mLoggerInfo->Signature = ADVANCED_LOGGER_SIGNATURE;

  // Invalid Buffer Offset
  mLoggerInfo->LogBufferOffset = (UINT32)0;
  status                       = ValidateInfoBlock (mLoggerInfo);
  EXPECT_EQ (status, FALSE);
  mLoggerInfo->LogBufferOffset = (ALIGN_VALUE (sizeof (testLoggerInfo), 8));

  // Invalid Current Offset
  mLoggerInfo->LogCurrentOffset = (UINT32)0;
  status                        = ValidateInfoBlock (mLoggerInfo);
  EXPECT_EQ (status, FALSE);
}

/*/* Commented out, need mock libraries to be implemented.
// Test AdvancedLoggerGetLoggerInfo when PEI Services is null
TEST_F (AdvancedLoggerPeiCoreTest, AdvLoggerGetInfoAlreadyInitializedValid) {
  ADVANCED_LOGGER_INFO  *LocalLoggerInfo = AdvancedLoggerGetLoggerInfo ();
}

// Test AdvancedLoggerGetLoggerInfo when PEI Services is returned
//
TEST_F (AdvancedLoggerPeiCoreTest, AdvLoggerGetInfoAlreadyInitializedValid) {
  ADVANCED_LOGGER_INFO  *LocalLoggerInfo = AdvancedLoggerGetLoggerInfo ();
}

// Test AdvancedLoggerGetLoggerInfo NULL HOB
TEST_F (AdvancedLoggerPeiCoreTest, AdvLoggerGetInfoNullHob) {
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
TEST_F (AdvancedLoggerPeiCoreTest, AdvLoggerGetInfoSuccess) {
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
