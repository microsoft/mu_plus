/** @file
 *DfciMemu

Copyright (c) 2018, Microsoft Corporation.

**/

#ifndef __DFCI_MENU_H__
#define __DFCI_MENU_H__


#include <Guid/DfciMenuGuid.h>   // defines MS_DFCI_* items
// The following defines are defined in Guid/MedModuleHii.h, but this header
// doesn't play well with the VFR compiler.
#define EFI_OTHER_DEVICE_CLASS               0x20
#define EFI_GENERAL_APPLICATION_SUBCLASS     0x01

// The following defines are from MsDfciMenuGuid.h.  Keep these in mind when altering
// the values used in this formset.
//
// #define MS_DFCE_MENU_FORM_ID       0x1000
// #define MS_DFCI_MENU_FORMSET_GUID  0xcf9873b2, 0xce63, 0x4f63, {0x9a, 0x5f, 0x16, 0x54, 0xe4, 0x01, 0x61, 0xc6 }

#define DFCI_MENU_INIT_QUESTION_ID          0x1100
#define DFCI_MENU_UPDATE_NOW_QUESTION_ID    0x1200

// These are the VFR compiler generated data representing our VFR data.
//
extern UINT8  DfciMenuVfrBin[];


// Grid class Start delimeter (GUID opcode).
//
#define SURFACE_GRID_START_OPCODE_GUID                                             \
  {                                                                                \
    0xc0b6e247, 0xe140, 0x4b4d, { 0xa6, 0x4, 0xc3, 0xae, 0x1f, 0xa6, 0xcc, 0x12 }  \
  }

// Grid class End delimeter (GUID opcode).
//
#define SURFACE_GRID_END_OPCODE_GUID                                               \
  {                                                                                \
    0x30879de9, 0x7e69, 0x4f1b, { 0xb5, 0xa5, 0xda, 0x15, 0xbf, 0x6, 0x25, 0xce }  \
  }

// Grid class select cell location (GUID opcode).
//
#define SURFACE_GRID_SELECT_CELL_OPCODE_GUID                                       \
  {                                                                                \
    0x3147b040, 0xeac3, 0x4b9f, { 0xb5, 0xec, 0xc2, 0xe2, 0x88, 0x45, 0x17, 0x4e } \
  }


#endif // __DFCI_MENU_H__

