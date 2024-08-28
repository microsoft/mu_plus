/** @file AdvancedLoggerPeiLibGoogleTest.cpp

    This file contains the unit tests for the Advanced Logger PEI Library.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockPeiServicesLib.h>
#include <GoogleTest/Ppi/MockAdvancedLogger.h>

extern "C" {
  #include <Uefi.h>
  #include <Base.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
  #include <AdvancedLoggerInternal.h>
  #include "../../AdvancedLoggerCommon.h"
}

using namespace testing;

/**
  Test class for AdvancedLoggerWrite Function
**/
class AdvancedLoggerWriteTestPei : public Test {
protected:
  StrictMock<MockPeiServicesLib> PeiServicesMock;
  StrictMock<MockAdvancedLoggerPpi> AdvancedLoggerPpiMock;
  UINTN DebugLevel;
  UINTN Instance;
  CHAR8 *Buffer;
  UINTN NumberOfBytes;

  void
  SetUp (
    ) override
  {
    CHAR8  OutputBuf[] = "MyUnitTestLog";

    NumberOfBytes     = sizeof (OutputBuf);
    Buffer            = OutputBuf;
    DebugLevel        = DEBUG_ERROR;
    Instance          = 0;
    gALPpi->Signature = ADVANCED_LOGGER_PPI_SIGNATURE;
    gALPpi->Version   = ADVANCED_LOGGER_PPI_VERSION;
  }
};

TEST_F (AdvancedLoggerWriteTestPei, PpiNotFoundFailure) {
  // Expect the locate PPI call to fail
  EXPECT_CALL (
    PeiServicesMock,
    PeiServicesLocatePpi (
      BufferEq (&gAdvancedLoggerPpiGuid, sizeof (EFI_GUID)),
      Eq (Instance),
      IsNull (),
      NotNull ()
      )
    )
    .WillOnce (
       Return (EFI_NOT_FOUND)
       );

  AdvancedLoggerWrite (DebugLevel, Buffer, NumberOfBytes);
}

TEST_F (AdvancedLoggerWriteTestPei, AdvLoggerWriteSuccess) {
  // Expect the call to LocatePpi to return success, "found" AdvancedLoggerPpi (mocked)
  EXPECT_CALL (
    PeiServicesMock,
    PeiServicesLocatePpi (
      BufferEq (&gAdvancedLoggerPpiGuid, sizeof (EFI_GUID)),
      Eq (Instance),
      IsNull (),
      NotNull ()
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(ByRef (gALPpi)),
         Return (EFI_SUCCESS)
         )
       );

  // Expect the call to AdvancedLoggerWritePpi
  EXPECT_CALL (
    AdvancedLoggerPpiMock,
    gAL_AdvancedLoggerWritePpi (
      Eq (DebugLevel),
      BufferEq (Buffer, NumberOfBytes),
      Eq (NumberOfBytes)
      )
    )
    .WillOnce (
       Return ()
       );

  AdvancedLoggerWrite (DebugLevel, Buffer, NumberOfBytes);
}

/* Passing an invalid buffer - should be caught/handled by the protocol */
TEST_F (AdvancedLoggerWriteTestPei, AdvLoggerWriteInvalidBuffer) {
  Buffer = nullptr;
  UINTN  numBytesZero = 0;

  // Expect the call to LocatePpi to return success, "found" AdvancedLoggerPpi (mocked)
  EXPECT_CALL (
    PeiServicesMock,
    PeiServicesLocatePpi (
      BufferEq (&gAdvancedLoggerPpiGuid, sizeof (EFI_GUID)),
      Eq (Instance),
      IsNull (),
      NotNull ()
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(ByRef (gALPpi)),
         Return (EFI_SUCCESS)
         )
       );

  // Expect the call to AdvancedLoggerWritePpi
  EXPECT_CALL (
    AdvancedLoggerPpiMock,
    gAL_AdvancedLoggerWritePpi (
      Eq (DebugLevel),
      BufferEq (Buffer, numBytesZero),
      Eq (NumberOfBytes)
      )
    )
    .WillOnce (
       Return ()
       );

  AdvancedLoggerWrite (DebugLevel, Buffer, NumberOfBytes);
}

/* Attempting to write 0 bytes - should be caught/handled by the protocol */
TEST_F (AdvancedLoggerWriteTestPei, AdvLoggerWriteZeroBytes) {
  UINTN  NumberOfBytesZero = 0;

  // Expect the call to LocatePpi to return success, "found" AdvancedLoggerPpi (mocked)
  EXPECT_CALL (
    PeiServicesMock,
    PeiServicesLocatePpi (
      BufferEq (&gAdvancedLoggerPpiGuid, sizeof (EFI_GUID)),
      Eq (Instance),
      IsNull (),
      NotNull ()
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(ByRef (gALPpi)),
         Return (EFI_SUCCESS)
         )
       );

  // Expect the call to AdvancedLoggerWritePpi
  EXPECT_CALL (
    AdvancedLoggerPpiMock,
    gAL_AdvancedLoggerWritePpi (
      Eq (DebugLevel),
      BufferEq (Buffer, NumberOfBytes),
      Eq (NumberOfBytesZero)
      )
    )
    .WillOnce (
       Return ()
       );

  AdvancedLoggerWrite (DebugLevel, Buffer, NumberOfBytesZero);
}

/* Passing a mismatched signature. Asserts are disabled so execution will continue.
   Expecting the write function to exit gracefully and not call gAL_AdvancedLoggerWritePpi
 */
TEST_F (AdvancedLoggerWriteTestPei, AdvLoggerWriteFailMismatchedSignature) {
  gALPpi->Signature = SIGNATURE_32 ('T', 'E', 'S', 'T');
  gALPpi->Version   = ADVANCED_LOGGER_PPI_VERSION + 1;

  // Expect the call to LocatePpi to return success, "found" AdvancedLoggerPpi (mocked)
  EXPECT_CALL (
    PeiServicesMock,
    PeiServicesLocatePpi (
      BufferEq (&gAdvancedLoggerPpiGuid, sizeof (EFI_GUID)),
      Eq (Instance),
      IsNull (),
      NotNull ()
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(ByRef (gALPpi)),
         Return (EFI_SUCCESS)
         )
       );

  AdvancedLoggerWrite (DebugLevel, Buffer, NumberOfBytes);
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
