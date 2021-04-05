/** @file
HwhMenuVfr.h

Header for the .vfr and .c hwh files

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/HwhMenuGuid.h>

#ifndef __HWH_MENU_VFR__
#define __HWH_MENU_VFR__

#define EFI_OTHER_DEVICE_CLASS            0x20
#define EFI_GENERAL_APPLICATION_SUBCLASS  0x01

//                           Aligned with 0x3000
#define HWH_MENU_MAIN_ID                  0x3001
#define HWH_MENU_LEFT_ID                  0x3002
#define HWH_MENU_RIGHT_ID                 0x3003
#define LABEL_UPDATE_LOCATION             0x3100
#define LABEL_UPDATE_END                  0x3101
#define HWH_MENU_VARID                    0x3200

// Grid class Start delimeter (GUID opcode).
//
#define GRID_CLASS_START_OPCODE_GUID                                               \
  {                                                                                \
    0xc0b6e247, 0xe140, 0x4b4d, { 0xa6, 0x4, 0xc3, 0xae, 0x1f, 0xa6, 0xcc, 0x12 }  \
  }

// Grid class End delimeter (GUID opcode).
//
#define GRID_CLASS_END_OPCODE_GUID                                                 \
  {                                                                                \
    0x30879de9, 0x7e69, 0x4f1b, { 0xb5, 0xa5, 0xda, 0x15, 0xbf, 0x6, 0x25, 0xce }  \
  }           

#define LOGS_FALSE  0x00
#define LOGS_TRUE   0x01

#pragma pack(1)                                                               

typedef struct {
  UINT8      Logs; // If equal to LOGS_TRUE, there are errors to display
} HWH_MENU_CONFIG;

#pragma pack()

#endif