/** @file AdvancedLoggerDxeLibGoogleTest.cpp

    This file contains the unit tests for the Advanced Logger DXE Library.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockUefiBootServicesTableLib.h>
#include <GoogleTest/Protocol/MockAdvancedLogger.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
  #include <PiDxe.h>
  #include <AdvancedLoggerInternal.h>
  #include "../AdvancedLoggerLib.c"
}

using namespace testing;

/**
  Test class for AdvancedLoggerWrite Function
**/
class AdvancedLoggerWriteTest : public Test {
protected:
  StrictMock<MockUefiBootServicesTableLib> gBSMock;
  StrictMock<MockAdvancedLogger> AdvLoggerProtocolMock;
  UINTN DebugLevel;
  CHAR8 *Buffer;
  UINTN NumberOfBytes;
  EFI_GUID gMockAdvLoggerProtocolGuid =
  { 0x434f695c, 0xef26, 0x4a12, { 0x9e, 0xba, 0xdd, 0xef, 0x00, 0x97, 0x49, 0x7c }
  };
  CHAR8 OutputBuf[100];

  void
  SetUp (
    ) override
  {
    mLoggerProtocol        = nullptr;
    NumberOfBytes          = sizeof (OutputBuf);
    Buffer                 = OutputBuf;
    DebugLevel             = DEBUG_ERROR;
    gALProtocol->Signature = ADVANCED_LOGGER_PROTOCOL_SIGNATURE;
    gALProtocol->Version   = ADVANCED_LOGGER_PROTOCOL_VERSION;
    snprintf (Buffer, sizeof (OutputBuf), "MyUnitTestLog");
  }

  void
  ExpectLoggerLookup (
    )
  {
    EXPECT_CALL (
      gBSMock,
      gBS_LocateProtocol (
        BufferEq (&gAdvancedLoggerProtocolGuid, sizeof (EFI_GUID)),
        Eq (nullptr),
        NotNull ()
        )
      )
      .WillOnce (
         DoAll (
           SetArgPointee<2> (ByRef (gALProtocol)),
           Return (EFI_SUCCESS)
           )
         );
    EXPECT_EQ (DxeAdvancedLoggerLibConstructor (nullptr, nullptr), EFI_SUCCESS);
  }
};

TEST_F (AdvancedLoggerWriteTest, AdvLoggerWriteSuccess) {
  ExpectLoggerLookup ();

  EXPECT_CALL (
    AdvLoggerProtocolMock,
    gAL_AdvancedLoggerWriteProtocol (
      Pointer (gALProtocol),
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

TEST_F (AdvancedLoggerWriteTest, AdvLoggerWriteMultiple) {
  ExpectLoggerLookup ();
  //
  // First call to AdvancedLoggerWrite
  //

  EXPECT_CALL (
    AdvLoggerProtocolMock,
    gAL_AdvancedLoggerWriteProtocol (
      Pointer (gALProtocol),
      Eq (DebugLevel),
      BufferEq (Buffer, NumberOfBytes),
      Eq (NumberOfBytes)
      )
    )
    .WillOnce (
       Return ()
       );

  AdvancedLoggerWrite (DebugLevel, Buffer, NumberOfBytes);

  //
  // Second call to AdvancedLoggerWrite
  //
  EXPECT_CALL (
    AdvLoggerProtocolMock,
    gAL_AdvancedLoggerWriteProtocol (
      Pointer (gALProtocol),
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
TEST_F (AdvancedLoggerWriteTest, AdvLoggerWriteInvalidBuffer) {
  ExpectLoggerLookup ();
  Buffer = nullptr;
  UINTN  numBytesZero = 0;

  EXPECT_CALL (
    AdvLoggerProtocolMock,
    gAL_AdvancedLoggerWriteProtocol (
      Pointer (gALProtocol),
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
TEST_F (AdvancedLoggerWriteTest, AdvLoggerWriteZeroBytes) {
  ExpectLoggerLookup ();
  UINTN  NumberOfBytesZero = 0;

  EXPECT_CALL (
    AdvLoggerProtocolMock,
    gAL_AdvancedLoggerWriteProtocol (
      Pointer (gALProtocol),
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

TEST_F (AdvancedLoggerWriteTest, AdvLoggerWriteFailMismatchedSignature) {
  gALProtocol->Signature = SIGNATURE_32 ('T', 'E', 'S', 'T');
  ExpectLoggerLookup ();
  AdvancedLoggerWrite (DebugLevel, Buffer, NumberOfBytes);
}

TEST_F (AdvancedLoggerWriteTest, AdvLoggerWriteFailMismatchedVersion) {
  gALProtocol->Version = 0;
  ExpectLoggerLookup ();
  AdvancedLoggerWrite (DebugLevel, Buffer, NumberOfBytes);
}

TEST_F (AdvancedLoggerWriteTest, MissingProtocolDoesNotFallBack) {
  EXPECT_CALL (
    gBSMock,
    gBS_LocateProtocol (
      BufferEq (&gAdvancedLoggerProtocolGuid, sizeof (EFI_GUID)),
      Eq (nullptr),
      NotNull ()
      )
    ).WillOnce (Return (EFI_NOT_FOUND));

  EXPECT_EQ (DxeAdvancedLoggerLibConstructor (nullptr, nullptr), EFI_SUCCESS);
  AdvancedLoggerWrite (DebugLevel, Buffer, NumberOfBytes);
  AdvancedLoggerWrite (DebugLevel, Buffer, NumberOfBytes);
}

TEST_F (AdvancedLoggerWriteTest, AdvLoggerProtocolInvalidParam) {
  EXPECT_CALL (
    gBSMock,
    gBS_LocateProtocol (
      BufferEq (&gAdvancedLoggerProtocolGuid, sizeof (EFI_GUID)),
      Eq (nullptr), // Registration Key
      NotNull ()    // Protocol Pointer OUT
      )
    )
    .WillOnce (
       Return (EFI_INVALID_PARAMETER)
       );

  EXPECT_EQ (DxeAdvancedLoggerLibConstructor (nullptr, nullptr), EFI_SUCCESS);
  AdvancedLoggerWrite (DebugLevel, Buffer, NumberOfBytes);
  AdvancedLoggerWrite (DebugLevel, Buffer, NumberOfBytes);
}
TEST_F (AdvancedLoggerWriteTest, WriteBeforeConstructorDoesNotLocateProtocol) {
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
