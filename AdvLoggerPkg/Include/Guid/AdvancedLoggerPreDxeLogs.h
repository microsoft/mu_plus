/** @file
  This file defines GUIDs and data structure for producing HOB entry describing
  the location of pre-DXE logs. For platforms which use a Pre-DXE AdvancedLoggerLib
  implementation, this HOB is not necessary.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ADVANCED_LOGGER_PRE_DXE_LOGS_H_
#define ADVANCED_LOGGER_PRE_DXE_LOGS_H_

#pragma pack(1)
// The region specified by this HOB entry should contain raw log output.
// The Advanced Logger library will serialize the contents of the buffer
// when adding it to the advanced logger output.
typedef struct _ADVANCED_LOGGER_PRE_DXE_LOGS_HOB {
  UINT32    Signature;
  UINT64    BaseAddress;
  UINT64    LengthInBytes;
} ADVANCED_LOGGER_PRE_DXE_LOGS_HOB;
#pragma pack()

#define ADVANCED_LOGGER_PRE_DXE_LOGS_GUID \
{ \
  0x751fc006, 0x5804, 0x440d, { 0x8b, 0x15, 0x61, 0x8c, 0xf6, 0x56, 0xae, 0x76 } \
}

extern EFI_GUID  gAdvancedLoggerPreDxeLogsGuid;

#define ADVANCED_LOGGER_PRE_DXE_LOGS_SIGNATURE  SIGNATURE_32 ('A', 'L', 'P', 'D')

#endif
