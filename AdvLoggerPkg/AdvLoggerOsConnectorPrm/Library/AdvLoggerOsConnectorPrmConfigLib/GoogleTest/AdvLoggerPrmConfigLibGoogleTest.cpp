/** @file

  This unit tests the AdvLoggerOsConnectorPrmConfigLib

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
#include <GoogleTest/Library/MockUefiRuntimeLib.h>
#include <GoogleTest/Library/MockMemoryAllocationLib.h>
#include <GoogleTest/Library/MockUefiBootServicesTableLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
  #include <Protocol/AdvancedLogger.h>
  #include <Protocol/PrmConfig.h>
  #include <PrmContextBuffer.h>
  #include <PrmDataBuffer.h>
  #include <AdvancedLoggerInternal.h>
  #include <AdvancedLoggerInternalProtocol.h>
  #include "../../../AdvLoggerOsConnectorPrm.h"

  extern PRM_DATA_BUFFER  *mStaticDataBuffer;

  VOID
  EFIAPI
  AdvLoggerOsConnectorPrmVirtualAddressCallback (
    IN EFI_EVENT  Event,
    IN VOID       *Context
    );

  EFI_STATUS
  EFIAPI
  AdvLoggerOsConnectorPrmConfigLibConstructor (
    IN  EFI_HANDLE        ImageHandle,
    IN  EFI_SYSTEM_TABLE  *SystemTable
    );
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
class AdvLoggerPrmConfigLibTest : public  Test {
protected:
  MockUefiRuntimeLib UefiRuntimeLib;
  MockMemoryAllocationLib MemoryAllocationLib;
  MockUefiBootServicesTableLib UefiBootServicesTableLib;
};

TEST_F (AdvLoggerPrmConfigLibTest, ConstructorTests) {
  EFI_STATUS                          Status;
  CHAR8                               Buf[sizeof (PRM_DATA_BUFFER_HEADER) + sizeof (ADV_LOGGER_PRM_DATA_BUFFER)];
  CHAR8                               ContextBuf[sizeof (PRM_CONTEXT_BUFFER)];
  CHAR8                               ProtocolBuf[sizeof (PRM_CONFIG_PROTOCOL)];
  ADVANCED_LOGGER_PROTOCOL_CONTAINER  LoggerProtocolContainer;
  ADVANCED_LOGGER_INFO                LoggerInfo;
  ADVANCED_LOGGER_PROTOCOL            *LoggerProtocol;
  ADV_LOGGER_PRM_DATA_BUFFER          *DataBuf;

  // test out of resources on mStaticDataBuffer
  // for all of the constructor tests, we are expecting it to return SUCCESS, even in failure cases as
  // this module is not required for boot. However, we want things safely cleaned up
  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateRuntimeZeroPool (
      sizeof (PRM_DATA_BUFFER_HEADER) + sizeof (ADV_LOGGER_PRM_DATA_BUFFER)
      )
    )
    .WillOnce (
       Return ((VOID *)NULL)
       );

  Status = AdvLoggerOsConnectorPrmConfigLibConstructor (NULL, NULL);
  EXPECT_EQ (Status, EFI_SUCCESS);

  // Test locate protocol failure
  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateRuntimeZeroPool (
      sizeof (PRM_DATA_BUFFER_HEADER) + sizeof (ADV_LOGGER_PRM_DATA_BUFFER)
      )
    )
    .WillOnce (
       Return (Buf)
       );

  EXPECT_CALL (
    UefiBootServicesTableLib,
    gBS_LocateProtocol (
      BufferEq (&gAdvancedLoggerProtocolGuid, sizeof (EFI_GUID)),
      NULL,
      _
      )
    )
    .WillOnce (
       Return (EFI_NOT_FOUND)
       );

  EXPECT_CALL (
    MemoryAllocationLib,
    FreePool (
      Eq (Buf)
      )
    );

  Status = AdvLoggerOsConnectorPrmConfigLibConstructor (NULL, NULL);
  EXPECT_EQ (Status, EFI_SUCCESS);

  // Fail validate Info Block
  LoggerProtocolContainer.LoggerInfo = &LoggerInfo;
  LoggerProtocol                     = &LoggerProtocolContainer.AdvLoggerProtocol;
  DataBuf                            = (ADV_LOGGER_PRM_DATA_BUFFER *)((PRM_DATA_BUFFER *)Buf)->Data;

  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateRuntimeZeroPool (
      sizeof (PRM_DATA_BUFFER_HEADER) + sizeof (ADV_LOGGER_PRM_DATA_BUFFER)
      )
    )
    .WillOnce (
       Return (Buf)
       );

  EXPECT_CALL (
    UefiBootServicesTableLib,
    gBS_LocateProtocol (
      BufferEq (&gAdvancedLoggerProtocolGuid, sizeof (EFI_GUID)),
      NULL,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgBuffer<2>(&LoggerProtocol, sizeof (VOID **)),
         Return (EFI_SUCCESS)
         )
       );

  EXPECT_CALL (
    MemoryAllocationLib,
    FreePool (
      Eq (Buf)
      )
    );

  Status = AdvLoggerOsConnectorPrmConfigLibConstructor (NULL, NULL);
  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (DataBuf->LoggerInfo, (ADVANCED_LOGGER_INFO *)NULL);

  // Fail to Allocate PrmContextBuffer
  LoggerProtocolContainer.LoggerInfo                  = &LoggerInfo;
  LoggerProtocol                                      = &LoggerProtocolContainer.AdvLoggerProtocol;
  LoggerProtocolContainer.AdvLoggerProtocol.Signature = ADVANCED_LOGGER_PROTOCOL_SIGNATURE;
  LoggerProtocolContainer.AdvLoggerProtocol.Version   = ADVANCED_LOGGER_PROTOCOL_VERSION;
  LoggerInfo.Signature                                = ADVANCED_LOGGER_SIGNATURE;
  LoggerInfo.LogBufferOffset                          = sizeof (LoggerInfo);
  LoggerInfo.LogCurrentOffset                         = sizeof (LoggerInfo);
  LoggerInfo.LogBufferSize                            = EFI_PAGES_TO_SIZE (FixedPcdGet32 (PcdAdvancedLoggerPages)) - sizeof (ADVANCED_LOGGER_INFO);

  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateRuntimeZeroPool (
      sizeof (PRM_DATA_BUFFER_HEADER) + sizeof (ADV_LOGGER_PRM_DATA_BUFFER)
      )
    )
    .WillOnce (
       Return (Buf)
       );

  EXPECT_CALL (
    UefiBootServicesTableLib,
    gBS_LocateProtocol (
      BufferEq (&gAdvancedLoggerProtocolGuid, sizeof (EFI_GUID)),
      NULL,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgBuffer<2>(&LoggerProtocol, sizeof (VOID **)),
         Return (EFI_SUCCESS)
         )
       );

  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateZeroPool (
      sizeof (PRM_CONTEXT_BUFFER)
      )
    )
    .WillOnce (
       Return ((VOID *)NULL)
       );

  EXPECT_CALL (
    MemoryAllocationLib,
    FreePool (
      Eq (Buf)
      )
    );

  Status = AdvLoggerOsConnectorPrmConfigLibConstructor (NULL, NULL);
  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (DataBuf->LoggerInfo, (ADVANCED_LOGGER_INFO *)NULL);

  // fail to allocate PrmConfigProtocol
  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateRuntimeZeroPool (
      sizeof (PRM_DATA_BUFFER_HEADER) + sizeof (ADV_LOGGER_PRM_DATA_BUFFER)
      )
    )
    .WillOnce (
       Return (Buf)
       );

  EXPECT_CALL (
    UefiBootServicesTableLib,
    gBS_LocateProtocol (
      BufferEq (&gAdvancedLoggerProtocolGuid, sizeof (EFI_GUID)),
      NULL,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgBuffer<2>(&LoggerProtocol, sizeof (VOID **)),
         Return (EFI_SUCCESS)
         )
       );

  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateZeroPool (
      sizeof (PRM_CONTEXT_BUFFER)
      )
    )
    .WillOnce (
       Return (ContextBuf)
       );

  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateZeroPool (
      sizeof (PRM_CONFIG_PROTOCOL)
      )
    )
    .WillOnce (
       Return ((VOID *)NULL)
       );

  EXPECT_CALL (
    MemoryAllocationLib,
    FreePool (
      Eq (Buf)
      )
    );

  EXPECT_CALL (
    MemoryAllocationLib,
    FreePool (
      Eq (ContextBuf)
      )
    );

  Status = AdvLoggerOsConnectorPrmConfigLibConstructor (NULL, NULL);
  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (DataBuf->LoggerInfo, (ADVANCED_LOGGER_INFO *)NULL);

  // test CreateEventEx failure
  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateRuntimeZeroPool (
      sizeof (PRM_DATA_BUFFER_HEADER) + sizeof (ADV_LOGGER_PRM_DATA_BUFFER)
      )
    )
    .WillOnce (
       Return (Buf)
       );

  EXPECT_CALL (
    UefiBootServicesTableLib,
    gBS_LocateProtocol (
      BufferEq (&gAdvancedLoggerProtocolGuid, sizeof (EFI_GUID)),
      NULL,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgBuffer<2>(&LoggerProtocol, sizeof (VOID **)),
         Return (EFI_SUCCESS)
         )
       );

  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateZeroPool (
      sizeof (PRM_CONTEXT_BUFFER)
      )
    )
    .WillOnce (
       Return (ContextBuf)
       );

  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateZeroPool (
      sizeof (PRM_CONFIG_PROTOCOL)
      )
    )
    .WillOnce (
       Return (ProtocolBuf)
       );

  EXPECT_CALL (
    UefiBootServicesTableLib,
    gBS_CreateEventEx (
      EVT_NOTIFY_SIGNAL,
      TPL_NOTIFY,
      AdvLoggerOsConnectorPrmVirtualAddressCallback,
      NULL,
      BufferEq (&gEfiEventVirtualAddressChangeGuid, sizeof (EFI_GUID)),
      _
      )
    )
    .WillOnce (
       Return (EFI_ABORTED)
       );

  EXPECT_CALL (
    MemoryAllocationLib,
    FreePool (
      Eq (Buf)
      )
    );

  EXPECT_CALL (
    MemoryAllocationLib,
    FreePool (
      Eq (ContextBuf)
      )
    );

  EXPECT_CALL (
    MemoryAllocationLib,
    FreePool (
      Eq (ProtocolBuf)
      )
    );

  Status = AdvLoggerOsConnectorPrmConfigLibConstructor (NULL, NULL);
  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (DataBuf->LoggerInfo, (ADVANCED_LOGGER_INFO *)NULL);

  // Test InstallMultipleProtocolInterface failure
  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateRuntimeZeroPool (
      sizeof (PRM_DATA_BUFFER_HEADER) + sizeof (ADV_LOGGER_PRM_DATA_BUFFER)
      )
    )
    .WillOnce (
       Return (Buf)
       );

  EXPECT_CALL (
    UefiBootServicesTableLib,
    gBS_LocateProtocol (
      BufferEq (&gAdvancedLoggerProtocolGuid, sizeof (EFI_GUID)),
      NULL,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgBuffer<2>(&LoggerProtocol, sizeof (VOID **)),
         Return (EFI_SUCCESS)
         )
       );

  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateZeroPool (
      sizeof (PRM_CONTEXT_BUFFER)
      )
    )
    .WillOnce (
       Return (ContextBuf)
       );

  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateZeroPool (
      sizeof (PRM_CONFIG_PROTOCOL)
      )
    )
    .WillOnce (
       Return (ProtocolBuf)
       );

  EXPECT_CALL (
    UefiBootServicesTableLib,
    gBS_CreateEventEx (
      EVT_NOTIFY_SIGNAL,
      TPL_NOTIFY,
      AdvLoggerOsConnectorPrmVirtualAddressCallback,
      NULL,
      BufferEq (&gEfiEventVirtualAddressChangeGuid, sizeof (EFI_GUID)),
      _
      )
    )
    .WillOnce (
       Return (EFI_SUCCESS)
       );

  EXPECT_CALL (
    UefiBootServicesTableLib,
    gBS_InstallProtocolInterface (
      _,
      BufferEq (&gPrmConfigProtocolGuid, sizeof (EFI_GUID)),
      EFI_NATIVE_INTERFACE,
      Eq (ProtocolBuf)
      )
    )
    .WillOnce (
       Return (EFI_ABORTED)
       );

  EXPECT_CALL (
    UefiBootServicesTableLib,
    gBS_CloseEvent (
      _
      )
    );

  EXPECT_CALL (
    MemoryAllocationLib,
    FreePool (
      Eq (Buf)
      )
    );

  EXPECT_CALL (
    MemoryAllocationLib,
    FreePool (
      Eq (ContextBuf)
      )
    );

  EXPECT_CALL (
    MemoryAllocationLib,
    FreePool (
      Eq (ProtocolBuf)
      )
    );

  Status = AdvLoggerOsConnectorPrmConfigLibConstructor (NULL, NULL);
  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (DataBuf->LoggerInfo, (ADVANCED_LOGGER_INFO *)NULL);

  // Test success case
  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateRuntimeZeroPool (
      sizeof (PRM_DATA_BUFFER_HEADER) + sizeof (ADV_LOGGER_PRM_DATA_BUFFER)
      )
    )
    .WillOnce (
       Return (Buf)
       );

  EXPECT_CALL (
    UefiBootServicesTableLib,
    gBS_LocateProtocol (
      BufferEq (&gAdvancedLoggerProtocolGuid, sizeof (EFI_GUID)),
      NULL,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgBuffer<2>(&LoggerProtocol, sizeof (VOID **)),
         Return (EFI_SUCCESS)
         )
       );

  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateZeroPool (
      sizeof (PRM_CONTEXT_BUFFER)
      )
    )
    .WillOnce (
       Return (ContextBuf)
       );

  EXPECT_CALL (
    MemoryAllocationLib,
    AllocateZeroPool (
      sizeof (PRM_CONFIG_PROTOCOL)
      )
    )
    .WillOnce (
       Return (ProtocolBuf)
       );

  EXPECT_CALL (
    UefiBootServicesTableLib,
    gBS_CreateEventEx (
      EVT_NOTIFY_SIGNAL,
      TPL_NOTIFY,
      AdvLoggerOsConnectorPrmVirtualAddressCallback,
      NULL,
      BufferEq (&gEfiEventVirtualAddressChangeGuid, sizeof (EFI_GUID)),
      _
      )
    )
    .WillOnce (
       Return (EFI_SUCCESS)
       );

  EXPECT_CALL (
    UefiBootServicesTableLib,
    gBS_InstallProtocolInterface (
      _,
      BufferEq (&gPrmConfigProtocolGuid, sizeof (EFI_GUID)),
      EFI_NATIVE_INTERFACE,
      Eq (ProtocolBuf)
      )
    )
    .WillOnce (
       Return (EFI_SUCCESS)
       );

  Status = AdvLoggerOsConnectorPrmConfigLibConstructor (NULL, NULL);
  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (DataBuf->LoggerInfo, &LoggerInfo);
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
