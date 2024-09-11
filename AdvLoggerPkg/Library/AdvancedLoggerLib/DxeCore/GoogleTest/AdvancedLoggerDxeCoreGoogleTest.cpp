/** @file AdvancedLoggerDxeCoreGoogleTest.cpp

    This file contains the unit tests for the Advanced Logger DXE Core Library.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
#include <GoogleTest/Library/MockMemoryAllocationLib.h>
#include <GoogleTest/Library/MockHobLib.h>
#include <GoogleTest/Library/MockAdvancedLoggerHdwPortLib.h>
#include <GoogleTest/Protocol/MockAdvancedLogger.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
  #include <AdvancedLoggerInternal.h>
  #include <Protocol/AdvancedLogger.h>
  #include <Guid/AdvancedLoggerPreDxeLogs.h>
  #include <Protocol/VariablePolicy.h>          // to mock (MU_BASECORE MdeModulePkg)
  #include <AdvancedLoggerInternalProtocol.h>
  #include <Library/BaseMemoryLib.h>            // to mock (MU_BASECORE MdePkg)
  #include <Library/PcdLib.h>                   // to mock OR  NULL lib? (MU_BASECORE MdePkg)
  #include <Library/SynchronizationLib.h>       // to mock (MU_BASECORE MdePkg)
  #include <Library/TimerLib.h>                 // to mock (MU_BASECORE MdePkg)
  #include <Library/VariablePolicyHelperLib.h>  // to mock (MU_BASECORE MdeModulePkg)
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
  Test class for AdvancedLoggerDxeCore
**/
class AdvancedLoggerDxeCoreTest : public Test {
protected:
  UINTN DebugLevel;
  CHAR8 *Buffer;
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
    CHAR8  OutputBuf[] = "MyUnitTestLog";

    NumberOfBytes                   = sizeof (OutputBuf);
    Buffer                          = OutputBuf;
    DebugLevel                      = DEBUG_ERROR;
    mInitialized                    = FALSE;
    ImageHandle                     = (EFI_HANDLE)0x12345678;
    testLoggerInfo.Signature        = ADVANCED_LOGGER_SIGNATURE;
    testLoggerInfo.Version          = ADVANCED_LOGGER_VERSION;
    testLoggerInfo.LogBufferOffset  = (ALIGN_VALUE (sizeof (testLoggerInfo), 8));
    testLoggerInfo.LogCurrentOffset = (ALIGN_VALUE (sizeof (testLoggerInfo), 8));
    mLoggerInfo                     = NULL;
  }
};

//
// Test ValidateInfoBlock
//
TEST_F (AdvancedLoggerDxeCoreTest, AdvLoggerGetInfoFail) {
  // NULL LoggerInfo
  status = ValidateInfoBlock ();
  EXPECT_EQ (status, FALSE);

  // Invalid Signature
  mLoggerInfo            = &testLoggerInfo;
  mLoggerInfo->Signature = SIGNATURE_32 ('T', 'E', 'S', 'T');
  status                 = ValidateInfoBlock ();
  EXPECT_EQ (status, FALSE);
  mLoggerInfo->Signature = ADVANCED_LOGGER_SIGNATURE;

  // Mismatched Version is okay? Wouldn't expect mismatched version with valid signature?

  // Invalid Buffer Offset
  mLoggerInfo->LogBufferOffset = (UINT32)0;
  status                       = ValidateInfoBlock ();
  EXPECT_EQ (status, FALSE);
  mLoggerInfo->LogBufferOffset = (ALIGN_VALUE (sizeof (testLoggerInfo), 8));

  // Invalid Current Offset
  mLoggerInfo->LogCurrentOffset = (UINT32)0;
  status                        = ValidateInfoBlock ();
  EXPECT_EQ (status, FALSE);
  mLoggerInfo->LogCurrentOffset = (ALIGN_VALUE (sizeof (testLoggerInfo), 8));

  // Invalid Buffer Size
  mLoggerInfo->LogBufferSize = (UINT32)0;
  mBufferSize                = (UINT32)0x1000;
  status                     = ValidateInfoBlock ();
  EXPECT_EQ (status, FALSE);
}

// Test AdvancedLoggerGetLoggerInfo when the logger info is already initialized
// and is currently valid.
TEST_F (AdvancedLoggerDxeCoreTest, AdvLoggerGetInfoAlreadyInitializedValid) {
  mInitialized = TRUE;
  mLoggerInfo  = &testLoggerInfo;

  ADVANCED_LOGGER_INFO  *LocalLoggerInfo = AdvancedLoggerGetLoggerInfo ();

  EXPECT_EQ (LocalLoggerInfo, mLoggerInfo);
}

// Test AdvancedLoggerGetLoggerInfo when the logger info is already initialized
// and is currently INVALID.
TEST_F (AdvancedLoggerDxeCoreTest, AdvLoggerGetInfoAlreadyInitializedInvalid) {
  mInitialized           = TRUE;
  mLoggerInfo            = &testLoggerInfo;
  mLoggerInfo->Signature = SIGNATURE_32 ('T', 'E', 'S', 'T');

  ADVANCED_LOGGER_INFO  *LocalLoggerInfo = AdvancedLoggerGetLoggerInfo ();

  EXPECT_EQ (LocalLoggerInfo, nullptr);
}

/*/* Commented out, need mock libraries to be implemented.
// Test AdvancedLoggerGetLoggerInfo NULL HOB
TEST_F (AdvancedLoggerDxeCoreTest, AdvLoggerGetInfoNullHob) {
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
TEST_F (AdvancedLoggerDxeCoreTest, AdvLoggerGetInfoSuccess) {
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

// Test DxeCore Advanced Logger constructor
TEST_F (AdvancedLoggerDxeCoreTest, AdvLoggerContructorSuccess) {
  DxeCoreAdvancedLoggerLibConstructor (ImageHandle, SystemTable);
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
