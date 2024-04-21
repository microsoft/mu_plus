/** @file AdvLoggerOsConnectorPrm.h

    Definitions to share between the AdvLogger PRM and PRM config lib

    Copyright (C) Microsoft Corporation. All rights reserved.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ADV_LOGGER_OS_CONNECTOR_PRM_H_
#define ADV_LOGGER_OS_CONNECTOR_PRM_H_

// {B4DFA4A2-EAD0-4F55-998B-EA5BE68F73FD}
STATIC CONST EFI_GUID  mPrmModuleGuid = {
  0xb4dfa4a2, 0xead0, 0x4f55, { 0x99, 0x8b, 0xea, 0x5b, 0xe6, 0x8f, 0x73, 0xfd }
};

// {0f8aef11-77b8-4d7f-84cc-fe0cce64ac14}
STATIC CONST EFI_GUID  mAdvLoggerOsConnectorPrmHandlerGuid = {
  0x0f8aef11, 0x77b8, 0x4d7f, { 0x84, 0xcc, 0xfe, 0x0c, 0xce, 0x64, 0xac, 0x14 }
};

// {0f8aef11-77b8-4d7f-84cc-fe0cce64ac14}
#define ADVANCED_LOGGER_OS_CONNECTOR_PRM_HANDLER_GUID  {0x0f8aef11, 0x77b8, 0x4d7f, {0x84, 0xcc, 0xfe, 0x0c, 0xce, 0x64, 0xac, 0x14}}

#pragma pack (push, 1)

typedef struct {
  ADVANCED_LOGGER_INFO    *LoggerInfo;
  UINT32                  ExpectedLogSize;
  UINT32                  ExpectedHeaderSize;
} ADV_LOGGER_PRM_DATA_BUFFER;

#pragma pack (pop)

#endif // ADV_LOGGER_OS_CONNECTOR_PRM_H_
