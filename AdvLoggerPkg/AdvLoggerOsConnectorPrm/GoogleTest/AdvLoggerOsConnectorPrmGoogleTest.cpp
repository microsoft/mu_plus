/** @file

  This unit tests AdvLoggerOsConnectorPrm

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <AdvancedLoggerInternal.h>
  #include <PrmModule.h>
  #include <PrmDataBuffer.h>
  #include "../AdvLoggerOsConnectorPrm.h"

  typedef struct {
    VOID      *OutputBuffer;
    UINT32    *OutputBufferSize;
  } ADVANCED_LOGGER_PRM_PARAMETER_BUFFER;

  BOOLEAN
  ValidateInfoBlock (
    ADV_LOGGER_PRM_DATA_BUFFER  *DataBuf
    );

  PRM_HANDLER_EXPORT (AdvLoggerOsConnectorPrmHandler);
}

// *----------------------------------------------------------------------------------*
// * Test Contexts                                                                    *
// *----------------------------------------------------------------------------------*

using namespace testing;

/// ================================================================================================
/// ================================================================================================
///
/// TEST CASES
///
/// ================================================================================================
/// ================================================================================================

//
// Declarations for unit tests
//
class AdvLoggerOsConnectorPrmTest : public  Test {
protected:
};

TEST_F (AdvLoggerOsConnectorPrmTest, ValidateInfoBlockTests) {
  ADV_LOGGER_PRM_DATA_BUFFER  DataBuf;
  BOOLEAN                     Result;
  ADVANCED_LOGGER_INFO        LoggerInfo;

  // Test NULL DataBuf
  Result = ValidateInfoBlock (NULL);
  EXPECT_EQ (Result, FALSE);

  // Test NULL LoggerInfo
  DataBuf.LoggerInfo = NULL;
  Result             = ValidateInfoBlock (&DataBuf);
  EXPECT_EQ (Result, FALSE);

  // Test Bad LoggerInfo Signature
  DataBuf.LoggerInfo   = &LoggerInfo;
  LoggerInfo.Signature = 0xDEADBEEF;
  Result               = ValidateInfoBlock (&DataBuf);
  EXPECT_EQ (Result, FALSE);

  // Test LogCurrentOffset > Total Log Size
  LoggerInfo.Signature        = ADVANCED_LOGGER_SIGNATURE;
  LoggerInfo.LogBufferOffset  = sizeof (LoggerInfo);
  LoggerInfo.LogBufferSize    = 0x1;
  LoggerInfo.LogCurrentOffset = 0x7777;
  Result                      = ValidateInfoBlock (&DataBuf);
  EXPECT_EQ (Result, FALSE);

  // Test LogCurrentOffset < LogBufferOffset
  LoggerInfo.LogBufferSize    = 0x10000;
  LoggerInfo.LogCurrentOffset = 0x3;
  Result                      = ValidateInfoBlock (&DataBuf);
  EXPECT_EQ (Result, FALSE);

  // Test ExpectedLogSize != LogBufferSize
  DataBuf.ExpectedLogSize     = 0x9999;
  LoggerInfo.LogCurrentOffset = 0x150;
  Result                      = ValidateInfoBlock (&DataBuf);
  EXPECT_EQ (Result, FALSE);

  // Test ExpectedHeaderSize != LogBufferOffset
  DataBuf.ExpectedHeaderSize = sizeof (LoggerInfo) - 0x10;
  DataBuf.ExpectedLogSize    = 0x10000;
  Result                     = ValidateInfoBlock (&DataBuf);
  EXPECT_EQ (Result, FALSE);

  // Test success
  DataBuf.ExpectedHeaderSize = sizeof (LoggerInfo);
  Result                     = ValidateInfoBlock (&DataBuf);
  EXPECT_EQ (Result, TRUE);
}

TEST_F (AdvLoggerOsConnectorPrmTest, AdvLoggerOsConnectorPrmHandlerTests) {
  EFI_STATUS                            Status;
  ADVANCED_LOGGER_PRM_PARAMETER_BUFFER  ParamBuf;
  PRM_CONTEXT_BUFFER                    ContextBuf;
  CHAR8                                 OutputBuf[0x100];
  UINT32                                OutputBufSize = (UINT32)0x0;
  ADV_LOGGER_PRM_DATA_BUFFER            *LogDataBuf;
  CHAR8                                 StaticDataBuffer[sizeof (PRM_DATA_BUFFER) + sizeof (ADV_LOGGER_PRM_DATA_BUFFER)];
  CHAR8                                 BigLoggerInfo[0x100];

  // Test NULL ParameterBuffer
  Status = AdvLoggerOsConnectorPrmHandler (NULL, NULL);
  EXPECT_EQ (Status, EFI_INVALID_PARAMETER);

  // Test NULL ContextBuffer
  Status = AdvLoggerOsConnectorPrmHandler (&ParamBuf, NULL);
  EXPECT_EQ (Status, EFI_INVALID_PARAMETER);

  // Test NULL StaticDataBuffer
  ContextBuf.StaticDataBuffer = NULL;
  ParamBuf.OutputBuffer       = &OutputBuf;
  ParamBuf.OutputBufferSize   = &OutputBufSize;
  memset (OutputBuf, 'O', 0x100);
  Status = AdvLoggerOsConnectorPrmHandler (&ParamBuf, &ContextBuf);
  EXPECT_EQ (Status, EFI_INVALID_PARAMETER);
  EXPECT_EQ (OutputBufSize, (UINT32)0);
  for (int i = 0; i < 0x100; i++) {
    EXPECT_EQ (OutputBuf[i], 'O');
  }

  // Test Bad Context Buffer Signature
  ContextBuf.Signature        = 0xDEADBEEF;
  ContextBuf.StaticDataBuffer = (PRM_DATA_BUFFER *)StaticDataBuffer;
  Status                      = AdvLoggerOsConnectorPrmHandler (&ParamBuf, &ContextBuf);
  EXPECT_EQ (Status, EFI_NOT_FOUND);
  EXPECT_EQ (OutputBufSize, (UINT32)0x0);
  for (int i = 0; i < 0x100; i++) {
    EXPECT_EQ (OutputBuf[i], 'O');
  }

  // Test Bad Static Data Buffer Signature
  ContextBuf.Signature                          = PRM_CONTEXT_BUFFER_SIGNATURE;
  ContextBuf.StaticDataBuffer->Header.Signature = 0xDEADBEEF;
  Status                                        = AdvLoggerOsConnectorPrmHandler (&ParamBuf, &ContextBuf);
  EXPECT_EQ (Status, EFI_NOT_FOUND);
  EXPECT_EQ (OutputBufSize, (UINT32)0x0);
  for (int i = 0; i < 0x100; i++) {
    EXPECT_EQ (OutputBuf[i], 'O');
  }

  // Test ValidateInfoBlock Fail
  ContextBuf.StaticDataBuffer->Header.Signature = PRM_DATA_BUFFER_HEADER_SIGNATURE;
  LogDataBuf                                    = (ADV_LOGGER_PRM_DATA_BUFFER *)ContextBuf.StaticDataBuffer->Data;
  LogDataBuf->LoggerInfo                        = (ADVANCED_LOGGER_INFO *)BigLoggerInfo;
  LogDataBuf->LoggerInfo->Signature             = 0xDEADBEEF;
  Status                                        = AdvLoggerOsConnectorPrmHandler (&ParamBuf, &ContextBuf);
  EXPECT_EQ (Status, EFI_COMPROMISED_DATA);
  EXPECT_EQ (OutputBufSize, (UINT32)0x0);
  for (int i = 0; i < 0x100; i++) {
    EXPECT_EQ (OutputBuf[i], 'O');
  }

  // Test OutputBufferSize == NULL
  LogDataBuf->LoggerInfo->Signature = ADVANCED_LOGGER_SIGNATURE;
  // use a made up header size here, to test we don't have the assumption LogBufferOffset == sizeof (LoggerInfo)
  // which may not be true for the PRM as it can be matched with a FW with a different header size
  LogDataBuf->LoggerInfo->LogBufferSize    = 0x100 - 0x88;
  LogDataBuf->LoggerInfo->LogBufferOffset  = 0x88;
  LogDataBuf->LoggerInfo->LogCurrentOffset = 0x88;
  LogDataBuf->ExpectedHeaderSize           = 0x88;
  LogDataBuf->ExpectedLogSize              = LogDataBuf->LoggerInfo->LogBufferSize;
  ParamBuf.OutputBufferSize                = NULL;
  Status                                   = AdvLoggerOsConnectorPrmHandler (&ParamBuf, &ContextBuf);
  EXPECT_EQ (Status, EFI_INVALID_PARAMETER);
  EXPECT_EQ (OutputBufSize, (UINT32)0x0);
  for (int i = 0; i < 0x100; i++) {
    EXPECT_EQ (OutputBuf[i], 'O');
  }

  // Test too small OutputBufferSize
  ParamBuf.OutputBufferSize = &OutputBufSize;
  Status                    = AdvLoggerOsConnectorPrmHandler (&ParamBuf, &ContextBuf);
  EXPECT_EQ (Status, EFI_BUFFER_TOO_SMALL);
  EXPECT_EQ (OutputBufSize, (UINT32)0x100);
  for (int i = 0; i < 0x100; i++) {
    EXPECT_EQ (OutputBuf[i], 'O');
  }

  // Test NULL OutputBuffer
  ParamBuf.OutputBuffer = NULL;
  Status                = AdvLoggerOsConnectorPrmHandler (&ParamBuf, &ContextBuf);
  EXPECT_EQ (Status, EFI_INVALID_PARAMETER);
  EXPECT_EQ (OutputBufSize, (UINT32)0x100);

  // Test success
  ParamBuf.OutputBuffer = OutputBuf;
  Status                = AdvLoggerOsConnectorPrmHandler (&ParamBuf, &ContextBuf);
  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (OutputBufSize, (UINT32)0x100);
  EXPECT_THAT (OutputBuf, BufferEq (BigLoggerInfo, (UINT32)0x100));
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
