/** @file

  Implements common structures and constants for a simple on-screen virtual keyboard.

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

#ifndef _ONSCREEN_KEYBOARD_H_
#define _ONSCREEN_KEYBOARD_H_

#include <Protocol/HiiFont.h>
#include <Protocol/SimpleWindowManager.h>
#include "DisplayTypes.h"


#define NUMBER_OF_KEYS              41                  // Total number of uniques keys across all keyboard pages.
#define KEYBOARD_INPUT_QUEUE_SIZE   20                  // Maximum depth of keyboard input queue.

#define DEFAULT_OSK_ICON_LOCATION       TopLeft         // Default keyboard icon screen position.
#define DEFAULT_OSK_LOCATION            TopLeft         // Default keyboard screen position.
#define DEFAULT_OSK_ANGLE               Angle_0         // Default keyboard rotation angle.
#define DEFAULT_OSK_SIZE                0.70f            // Default keyboard size (percent of screen width).

#define PERIODIC_CHKINPUT_INTERVAL  (  5 * 10 * 1000)   // Check for pointer events: 5ms in 100ns units
#define PERIODIC_REFRESH_INTERVAL   (  5 * 10 * 1000)   // Check for paint events: 5ms in 100ns units
#define INITIAL_KEYREPEAT_INTERVAL  (500 * 10 * 1000)   // Initial Key Repeat: 500ms in 100ns units
#define STEADYST_KEYREPEAT_INTERVAL ( 33 * 10 * 1000)   // Steady-State Key Repeat: 33ms in 100ns units

#define SMALL_ASSET_MAX_SCREEN_WIDTH    1280            // Maximum screen resolution that still supports "small" keyboard icon & button bitmaps.

// Reference keyboard information - note that the geometry may be scaled/transformed for rendering.
//
// Standard key        = 145x120
// Backspace key       = 305x120
// Enter key           = 265x120
// Space key           = 940x120
// Key spacing         = 14  (9.655172% of standard key width)
// Left spacing        = 120 (82.758620% of standard key width)
// Left spacing indent = 160 (110.344827% of standard key width)
// Right spacing       = 120 (82.758620% of standard key width)
// Top border height   = 84  (70% of standard key height)
//
#define DEFAULT_MIN_SCALE           (float)0.1
#define STANDARD_KEY_WIDTH          (float)145.0
#define STANDARD_KEY_HEIGHT         (float)120.0
#define BKSP_KEY_WIDTH_PERCENT      (float)2.103448
#define ENTER_KEY_WIDTH_PERCENT     (float)1.827586
#define SPACE_KEY_WIDTH_PERCENT     (float)6.482758

#define KEY_SPACING_PERCENT         (float)0.09655172
#define INDENT_SPACING_PERCENT      (float)0.82758620
#define INDENT2_SPACING_PERCENT     (float)1.10344820
#define RIGHT_SPACING_PERCENT       (float)0.82758620
#define TOP_BORDER_HEIGHT_PERCENT   (float)0.70000000

#define DOCK_BUTTON_X_PERCENT       (float)0.920000     // X position is 92% of keyboard width
#define CLOSE_BUTTON_X_PERCENT      (float)0.970000     // X position is 97% of keyboard width


// Function prototypes
//
EFI_STATUS
EFIAPI
OSKDriverSupported (IN EFI_DRIVER_BINDING_PROTOCOL  *This,
                    IN EFI_HANDLE                   Controller,
                    IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath);

EFI_STATUS
EFIAPI
OSKDriverStart (IN EFI_DRIVER_BINDING_PROTOCOL      *This,
                IN EFI_HANDLE                       Controller,
                IN EFI_DEVICE_PATH_PROTOCOL         *RemainingDevicePath);

EFI_STATUS
EFIAPI
OSKDriverStop (IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
               IN  EFI_HANDLE                      Controller,
               IN  UINTN                           NumberOfChildren,
               IN  EFI_HANDLE                      *ChildHandleBuffer);

EFI_STATUS
ShowKeyboard (IN BOOLEAN bShowKeyboard);

EFI_STATUS
ShowKeyboardIcon (IN BOOLEAN bShowKeyboard);

EFI_STATUS
SetKeyboardIconPosition (IN SCREEN_POSITION      Position);

EFI_STATUS
SetKeyboardPosition (IN SCREEN_POSITION      Position,
                     IN OSK_DOCKED_STATE     DockedState);

EFI_STATUS
SetKeyboardSize (IN float    PercentOfScreenWidth);

EFI_STATUS
GetKeyboardMode (IN UINT32   *ModeBitfield);

EFI_STATUS
SetKeyboardMode (IN UINT32   ModeBitfield);

EFI_STATUS
RotateKeyboard(IN SCREEN_ANGLE    Angle);

VOID
GetKeyboardBoundingRect (OUT SWM_RECT *pRect);


// Current keyboard modifier state (i.e., shift, caps lock, numsym lock, etc.)
//
typedef enum
{
    Normal = 0,
    Shift,
    CapsLock,
    NumSym,
    Function
} OSK_KEY_MODIFIER_STATE;


// Key mapping structure
//
typedef struct _OSK_KEY_MAPPING_tag
{
    EFI_KEY     EfiKey;             // Key identifier
    CHAR16      Unicode;            // Key Unicode value to be provided through console-in protocols
    UINT16      ScanCode;           // Key scancode value to be provided through console-in protocols
    EFI_STRING  KeyLabel;           // Key label to be displayed on the key when rendering
    UINTN       KeyLabelWidth;      // Key label width (pixels) based on current font
    UINTN       KeyLabelHeight;     // Key label height (pixels) based on current font
} OSK_KEY_MAPPING;


// Individual key information structure
//
typedef struct _KEYINFO_tag
{

    // Key fill and text colors
    //
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pKeyFillColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pKeyLabelColor;

    // Raw key bounding rectangle (floating-point 3D space) used for transformations
    //
    RECT3D *pKeyBoundingRect;

    // Key bounding rectangle (as integer) used for display and touch/mouse hit detection
    //
    struct
    {
        UINTN Left;
        UINTN Top;
        UINTN Right;
        UINTN Bottom;
    } KeyDisplayHitRect;

} KEY_INFO;


// Icon and special button bitmap information.
//
typedef struct _BITMAP_INFO_tag
{
    const UINT32 *pBitmap;
    UINTN Width;
    UINTN Height;
} BITMAP_INFO;


// Keyboard context
//
typedef struct _KEYBOARDINFO_tag
{
    // Keyboard icon & keybard position, angle.
    //
    SCREEN_POSITION     KeyboardIconPosition;
    SCREEN_POSITION     KeyboardPosition;
    SCREEN_ANGLE        KeyboardAngle;

    // Docked state
    //
    OSK_DOCKED_STATE    DockedState;

    // Keyboard modifier state
    //
    OSK_KEY_MODIFIER_STATE   KeyModifierState;

    // Active key mapping table (swapped depending on modifier state)
    //
    OSK_KEY_MAPPING *pKeyMap;

    // Key press input queue (holds key press data until consumer reads them out, FIFO)
    //
    BOOLEAN bQueueEmpty;
    UINTN QueueInputPosition;
    UINTN QueueOutputPosition;
    EFI_INPUT_KEY KeyPressQueue[KEYBOARD_INPUT_QUEUE_SIZE];

    // Simple Input and Simple Input Extended protocols
    //
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL    SimpleTextIn;
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL SimpleTextInEx;

    // OSK protocol
    //
    MS_ONSCREEN_KEYBOARD_PROTOCOL       OSKProtocol;

    // Screen resolution used to compute keyboard asset locations and dimensions.
    //
    UINTN ScreenResolutionWidth;
    UINTN ScreenResolutionHeight;

    // Maximum keyboard (screen) dimensions
    //
    float PercentOfScreenWidth;
    UINTN KeyboardMaxWidth;
    UINTN KeyboardMaxHeight;

    BOOLEAN bKeyboardMoving;                // TRUE == keyboard is in the process of being moved (dragged by user)
    UINTN KeyboardDragOrigX;                // Starting/sampled keyboard X position during drag operation (used to compute dx, dy offset)
    UINTN KeyboardDragOrigY;                // Starting/sampled keyboard Y position during drag operation (used to compute dx, dy offset)

    // Preferred font display information (adapted to current video mode).
    // NOTE: Unless space is allocated for a FontName directly after the EFI_FONT_INFO
    //       structure, a font name can not be specified. The EFI_FONT_INFO structure only allocates
    //       space for a single CHAR16 (the terminating NULL).
    EFI_FONT_INFO           PreferredFontInfo;

    // Memory buffers used to render the keyboard and to maintain screen contents
    // when the keyboard is dismissed
    //
    BOOLEAN bKeyboardIconAutoEnable;
    BOOLEAN bKeyboardSelfRefresh;
    BOOLEAN bDisplayKeyboardIcon;
    BITMAP_INFO KeyboardIcon;               // Keyboard icon.
    BOOLEAN bDisplayKeyboard;
    BOOLEAN bKeyboardStateChanged;          // Keyboard has changed (but not dimensionally), for example key selection highlighting
    BOOLEAN bKeyboardSizeChanged;           // If size has changed, we need to refresh keyboard background
    BOOLEAN bShowDockAndCloseButtons;       // Whether to show (enable) the (un)dock and close buttons
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pBackBuffer;
    EFI_IMAGE_OUTPUT              *pKeyTextBltBuffer;

    // Individual key information (references key geometries below for hit detection)
    //
    UINTN    SelectedKey;
    UINTN    DeselectKey;
    KEY_INFO KeyList[NUMBER_OF_KEYS];

    // Individual key geometries - original and screen-transformed pointsets
    //
    RECT3D  KeyRectOriginal[NUMBER_OF_KEYS];    // Original rects computed based on keyboard geometry.
    RECT3D  KeyRectDisplay[NUMBER_OF_KEYS];     // Original rects adjusted for keyboard rotation (i.e., mapped to screen coordinate space).
    RECT3D  KeyRectXformed[NUMBER_OF_KEYS];     // Display rects transformed by current transform matrix.

    // Outer keyboard bounding rectangle - original and screen-transformed pointsets
    //
    RECT3D  KeyboardRectOriginal;               // Original rect computed based on keyboard geometry.
    RECT3D  KeyboardRectDisplay;                // Original rect adjusted for keyboard rotation (i.e. mapped to screen coordinate space).
    RECT3D  KeyboardRectXformed;                // Display rect transformed by current transform matrix.

    // Dock-Undock Button centerpoint
    //
    BITMAP_INFO KeyboardDockButton;             // Dock button icon.
    BITMAP_INFO KeyboardUndockButton;           // Undock button icon.
    POINT3D DockingButtonOriginal;              // Original rect computed based on keyboard geometry.
    POINT3D DockingButtonDisplay;               // Original rect adjusted for keyboard rotation (i.e. mapped to screen coordinate space).
    POINT3D DockingButtonXformed;               // Display rect transformed by current transform matrix.

    // Close Button centerpoint
    //
    BITMAP_INFO KeyboardCloseButton;            // Close button icon.
    POINT3D CloseButtonOriginal;                // Original rect computed based on keyboard geometry.
    POINT3D CloseButtonDisplay;                 // Original rect adjusted for keyboard rotation (i.e. mapped to screen coordinate space).
    POINT3D CloseButtonXformed;                 // Display rect transformed by current transform matrix.

} KEYBOARD_CONTEXT;


// External definitions
//
extern MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *mSWMProtocol;
extern KEYBOARD_CONTEXT                    mOSK;
extern EFI_HANDLE                          mImageHandle;

#endif  // _ONSCREEN_KEYBOARD_H_
