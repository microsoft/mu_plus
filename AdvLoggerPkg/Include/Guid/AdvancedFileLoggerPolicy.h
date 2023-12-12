/** @file
  This file defines GUIDs and data structure used for Advanced file logger policy.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef  ADVANCED_FILE_LOGGER_POLICY_H_
#define  ADVANCED_FILE_LOGGER_POLICY_H_

#define ADVANCED_FILE_LOGGER_POLICY_GUID  \
  { \
    0x6c3fd4f1, 0xb596, 0x438e, { 0x8a, 0xb8, 0x53, 0xf1, 0xc, 0x9c, 0x27, 0x74 } \
  }

#define ADVANCED_FILE_LOGGER_POLICY_SIZE  sizeof(ADVANCED_FILE_LOGGER_POLICY)

#pragma pack(1)

typedef struct {
  BOOLEAN    FileLoggerEnable;
} ADVANCED_FILE_LOGGER_POLICY;

#pragma pack()

extern EFI_GUID  gAdvancedFileLoggerPolicyGuid;

#endif //ADVANCED_FILE_LOGGER_POLICY_H_
