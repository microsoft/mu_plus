/** @file

This include file is shared between the Simple FrontPage and our custom Forms Display Engine.  It
defines the master UI layout (something that should be replaced with a XAML-like implementation in the
future) as well as shared structures for communicating and coordinating user input events between the
two subsystems.

Copyright (c) 2015 - 2018, Microsoft Corporation.

All rights reserved.
Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#ifndef _MS_DISPLAY_ENGINE_H_
#define _MS_DISPLAY_ENGINE_H_

#include <Protocol/SimpleWindowManager.h>

// ****************************************************************************
// **                  Simple UI Element Master Layout                       **
// **                                                                        **
// ** The following set of constants represent coordinates in percentage of  **
// ** screen size values for nearly all displayable Simple UI elements and   **
// ** are used for layout of our FrontPage, Dialog, and related screens.     **
// **                                                                        **
// ** NOTE: This should all be replaced with a XAML-like implementation in   **
// **       the future.  For now hopefully this quick-and-dirty              **
// **       implementation is sufficient.                                    **
// ****************************************************************************
//

#define MS_DEFAULT_FONT_SIZE                     MsUiGetStandardFontHeight ()     // Default font size
 
// FrontPage TitleBar (NOTE: Y origins are based on vertically centering the element in the TitleBar).
#define FP_TBAR_HEIGHT_PERCENT                   8                      // TitleBar height is 8% the height of the screen.
#define FP_TBAR_MSLOGO_X_PERCENT                 4                      // TitleBar: Microsoft Logo x origin starts at 4% of *Master Frame* width.
#define FP_TBAR_TEXT_X_PERCENT                   25                     // TitleBar: Title text x origin starts at 25% of the *Master Frame* width.
#define FP_TBAR_TEXT_FONT_HEIGHT                 MsUiGetLargeFontHeight ()  // TitleBar: Title text font height.
#define FP_TBAR_ENTRY_INDICATOR_X_PERCENT        96                     // TitleBar: Entry icon location upper right corner

// FrontPage Master Frame
#define FP_MFRAME_WIDTH_PERCENT                  25                     // Master Frame is 25% the width of the screen.
#define FP_MFRAME_MENU_TEXT_OFFSET_PERCENT       4                      // Master Frame: Indent menu text 4% of the Master Frame width.
#define FP_MFRAME_MENU_CELL_HEIGHT_PERCENT       6                      // Master Frame: Menu cell height is 6% of the Master Frame height.
#define FP_MFRAME_MENU_TEXT_FONT_HEIGHT          MsUiGetStandardFontHeight ()  // Master Frame: Menu text font height.
#define FP_MFRAME_DIVIDER_LINE_WIDTH_PIXELS      MsUiScaleByTheme (3)   // Master Frame: Divider line between Master Frame and form canvas is 3 pixel.

// FrontPage Form Canvas
#define FP_FCANVAS_BORDER_PAD_WIDTH_PERCENT      8                      // Form Canvas: Left & Right canvas border padding is 8% the width of the screen.
#define FP_FCANVAS_BORDER_PAD_HEIGHT_PERCENT     4                      // Form Canvas: Top & Bottom canvas border padding is 4% the height of the screen.

// Grid class Start delimeter (GUID opcode).
//
#define GRID_START_OPCODE_GUID                                                     \
  {                                                                                \
    0xc0b6e247, 0xe140, 0x4b4d, { 0xa6, 0x4, 0xc3, 0xae, 0x1f, 0xa6, 0xcc, 0x12 }  \
  }

// Grid class End delimeter (GUID opcode).
//
#define GRID_END_OPCODE_GUID                                                       \
  {                                                                                \
    0x30879de9, 0x7e69, 0x4f1b, { 0xb5, 0xa5, 0xda, 0x15, 0xbf, 0x6, 0x25, 0xce }  \
  }

// Grid class select cell location (GUID opcode).
//
#define GRID_SELECT_CELL_OPCODE_GUID                                               \
  {                                                                                \
    0x3147b040, 0xeac3, 0x4b9f, { 0xb5, 0xec, 0xc2, 0xe2, 0x88, 0x45, 0x17, 0x4e } \
  }

// Bitmap class definition (GUID opcode).
//
#define BITMAP_OPCODE_GUID                                                         \
  {                                                                                \
    0xefbdb196, 0x91d7, 0x4e04, { 0xb7, 0xef, 0xa4, 0x4c, 0x5f, 0xba, 0x2e, 0xb5 } \
  }

// Simple Refresh Formset GUID
//
#define REFRESH_FORMSET_GUID                                                       \
  {                                                                                \
    0x2166d685, 0x70a0, 0x4cd8, { 0x89, 0x50, 0x82, 0x9e, 0x4d, 0xc1, 0x05, 0x5a } \
  }


// Shared FrontPage - Display Engine notification types.
//
typedef enum
{
    NONE = 0,   // No action to be taken.
    REDRAW,     // Redraw the Top Menu.
    USERINPUT   // User input provided.
} FPDE_SHARED_NOTIFY_TYPE;

// ****** Structure Definitions ******
//

// Custom structure for sharing user event and operating state information between the Simple FrontPage
// and our custom display engine.
//
typedef struct _DISPLAY_ENGINE_SHARED_STATE_
{
    BOOLEAN                 CloseFormRequest;       // Request from FrontPage to display engine (forms browser) to close the current form.
    BOOLEAN                 ShowTopMenuHighlight;   // Indicates whether the Top Menu should show keyboard tab highlight.
    FPDE_SHARED_NOTIFY_TYPE NotificationType;       // FrontPage notification type.
    SWM_INPUT_STATE         InputState;             // User input (i.e., keyboard event, touch/mouse event, etc.).

} DISPLAY_ENGINE_SHARED_STATE;

#endif  // _MS_DISPLAY_ENGINE_H_.
