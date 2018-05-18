/** @file
 *DfciMemu

Copyright (c) 2018, Microsoft Corporation.

**/

#ifndef __DFCI_MENU_H__
#define __DFCI_MENU_H__


#include <Guid/DfciMenuGuid.h>   // defines DFCI_* items
// The following defines are defined in Guid/MdeModuleHii.h, but this header
// doesn't play well with the VFR compiler.
#define EFI_OTHER_DEVICE_CLASS               0x20
#define EFI_GENERAL_APPLICATION_SUBCLASS     0x01

// The following defines are from DfciMenuGuid.h.  Keep these in mind when altering
// the values used in this formset.
//
// #define DFCI_MENU_FORM_ID       0x1000
// #define DFCI_MENU_FORMSET_GUID  0x3b82283d, 0x7add, 0x4c6a, {0xad, 0x2b, 0x71, 0x9b, 0x8d, 0x7b, 0x77, 0xc9} \

#define DFCI_MENU_VARID                         0x0900
#define DFCI_MENU_INIT_QUESTION_ID              0x1100
#define DFCI_MENU_HTTPS_UPDATE_NOW_QUESTION_ID  0x1200
#define DFCI_MENU_USB_UPDATE_NOW_QUESTION_ID    0x1300
#define DFCI_MENU_RECOVERY_NOW_QUESTION_ID      0x1400

// These are the VFR compiler generated data representing our VFR data.
//
extern UINT8  DfciMenuVfrBin[];

typedef struct {
    UINT8    DfciRecoveryEnabled;
    UINT8    DfciHttpsRecoveryEnabled;
} DFCI_MENU_CONFIGURATION;

// Grid class form browser extensions.  These should be ignored
// by any browser not implementing the Grid Class ignores these hints
//
//
#define GRID_CLASS_START_OPCODE_GUID                                             \
  {                                                                                \
    0xc0b6e247, 0xe140, 0x4b4d, { 0xa6, 0x4, 0xc3, 0xae, 0x1f, 0xa6, 0xcc, 0x12 }  \
  }

// Grid class End delimeter (GUID opcode).
//
#define GRID_CLASS_END_OPCODE_GUID                                               \
  {                                                                                \
    0x30879de9, 0x7e69, 0x4f1b, { 0xb5, 0xa5, 0xda, 0x15, 0xbf, 0x6, 0x25, 0xce }  \
  }

// Grid class select cell location (GUID opcode).
//
#define GIRD_CLASS_SELECT_CELL_OPCODE_GUID                                       \
  {                                                                                \
    0x3147b040, 0xeac3, 0x4b9f, { 0xb5, 0xec, 0xc2, 0xe2, 0x88, 0x45, 0x17, 0x4e } \
  }


#endif // __DFCI_MENU_H__

