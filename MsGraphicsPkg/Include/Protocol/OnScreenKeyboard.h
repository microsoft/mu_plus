/** @file
  Defines the On-Screen Keyboard (OSK) protocol.

  Copyright (c) 2014 - 2018, Microsoft Corporation.

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

#ifndef _OSK_PROTOCOL_H_
#define _OSK_PROTOCOL_H_

#include <Protocol/SimpleWindowManager.h>


// Global ID for the On-Screen Keyboard Protocol
//
#define MS_ONSCREEN_KEYBOARD_PROTOCOL_GUID                                          \
  {                                                                                 \
    0x3c4ca20d, 0xc95a, 0x4b8b, { 0x81, 0xaf, 0x94, 0xa9, 0x83, 0x9, 0x23, 0xe2 }   \
  }

typedef struct  _MS_ONSCREEN_KEYBOARD_PROTOCOL  MS_ONSCREEN_KEYBOARD_PROTOCOL;

// Keyboard mode values (used in mode bitfield since multiple can be set at once).
//
#define OSK_MODE_AUTOENABLEICON 0x00000001      // Auto-Enable mode causes OSK icon to be displayed when client waits on input.
#define OSK_MODE_NUMERIC        0x00000002      // Numeric mode causes OSK to switch to numeric input page.
#define OSK_MODE_SELF_REFRESH   0x00000004      // Keyboard self-refresh mode (periodically redraws itself).


// Screen position values - used for keyboard & icon placement
//
typedef enum
{
    BottomRight = 0,
    BottomCenter,
    BottomLeft,
    LeftCenter,
    TopRight,
    TopCenter,
    TopLeft,
    RightCenter
} SCREEN_POSITION;


// Screen fixed rotation angles - used for keyboard rotation angle
//
typedef enum
{
    Angle_0 = 0,
    Angle_90,
    Angle_180,
    Angle_270
} SCREEN_ANGLE;


// Current keyboard docked state
//
typedef enum
{
    Docked = 0,
    Undocked
} OSK_DOCKED_STATE;

/**
  Shows or Hides the OSK icon

  @param  This                 Protocol instance pointer.
  @param  bShowIcon            TRUE == Show icon, FALSE == Hide icon

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could not be reset.

**/
typedef
EFI_STATUS
(EFIAPI *MS_OSK_SHOW_KEYBOARD_ICON)(
    IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
    IN BOOLEAN                          bShowIcon
);

/**
  Selects the OSK icon position

  @param  This                 Protocol instance pointer.
  @param  Position             Location where the OSK icon should appear

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could not be reset.

**/
typedef
EFI_STATUS
(EFIAPI *MS_OSK_SET_KEYBOARD_ICON_POSITION)(
    IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
    IN SCREEN_POSITION                  Position
);

/**
  Selects the OSK position

  @param  This                 Protocol instance pointer.
  @param  Position             Location where the OSK should appear
  @param  DockedState          OSK docked state

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could not be reset.

**/
typedef
EFI_STATUS
(EFIAPI *MS_OSK_SET_KEYBOARD_POSITION)(
    IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
    IN SCREEN_POSITION                  Position,
    IN OSK_DOCKED_STATE                 DockedState
);

/**
  Sets the OSK size

  @param  This                 Protocol instance pointer.
  @param  PercentOfScreenSize  Keyboard scale specified in % of screen width (1-100)

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could not be reset.

**/
typedef
EFI_STATUS
(EFIAPI *MS_OSK_SET_KEYBOARD_SIZE)(
    IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
    IN UINTN                            PercentOfScreenWidth
);

/**
  Sets the OSK rotation angle

  @param  This                 Protocol instance pointer.
  @param  KeyboardAngle        Angle keyboard is rendered on the screen.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could not be reset.

**/
typedef
EFI_STATUS
(EFIAPI *MS_OSK_SET_KEYBOARD_ANGLE)(
    IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
    IN SCREEN_ANGLE                     KeyboardAngle
);


/**
  Retrieves the current OSK mode

  @param  This                 Protocol instance pointer.
  @param  ModeBitfield         Bitfield representing currently-enabled mode(s).

  @retval EFI_SUCCESS          Device mode was successfully retrieved.
          !EFI_SUCCESS         Failure.
**/
typedef
EFI_STATUS
(EFIAPI *MS_OSK_GET_KEYBOARD_MODE)(
    IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
    IN UINT32                           *ModeBitfield
);


/**
  Sets the OSK mode

  @param  This                 Protocol instance pointer.
  @param  ModeBitfield         Bitfield representing the mode(s) to be enabled.

  @retval EFI_SUCCESS          The device mode was successfully applied.
          !EFI_SUCCESS         Failure.
**/
typedef
EFI_STATUS
(EFIAPI *MS_OSK_SET_KEYBOARD_MODE)(
    IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
    IN UINT32                           ModeBitfield
);

/**
  Shows or Hides the OSK

  @param  This                 Protocol instance pointer.
  @param  bShowKeyboard        TRUE == Show keyboard, FALSE == Hide keyboard

  @retval EFI_SUCCESS          The request was successful.

**/
typedef
EFI_STATUS
(EFIAPI *MS_OSK_SHOW_KEYBOARD)(
    IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
    IN BOOLEAN                          bShowKeyboard
);


/**
  Shows or Hides the OSK's (Un)docking and Close buttons, thereby disabling those features.

  @param  This                          Protocol instance pointer.
  @param  bShowDockAndCloseButtons      TRUE == Show buttons, FALSE == Hide buttons

  @retval EFI_SUCCESS          The request was successful.

**/
typedef
EFI_STATUS
(EFIAPI *MS_OSK_SHOW_DOCKANDCLOSE_BUTTONS)(
    IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
    IN BOOLEAN                          bShowDockAndCloseButtons
);


/**
  Gets the current OSK outer bounding frame (position and size).

  @param  This              Protocol instance pointer.
  @param  FrameRect         OSK frame rectangle.             

  @retval EFI_SUCCESS       The request was successful.

**/
typedef
EFI_STATUS
(EFIAPI *MS_OSK_GET_KEYBOARD_BOUNDS)(
    IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
    IN SWM_RECT                         *FrameRect
);


// OSK protocol structure
//
struct _MS_ONSCREEN_KEYBOARD_PROTOCOL
{
    MS_OSK_SHOW_KEYBOARD_ICON           ShowKeyboardIcon;
    MS_OSK_SHOW_KEYBOARD                ShowKeyboard;
    MS_OSK_SHOW_DOCKANDCLOSE_BUTTONS    ShowDockAndCloseButtons;
    MS_OSK_SET_KEYBOARD_ICON_POSITION   SetKeyboardIconPosition;
    MS_OSK_SET_KEYBOARD_POSITION        SetKeyboardPosition;
    MS_OSK_SET_KEYBOARD_ANGLE           SetKeyboardRotationAngle;
    MS_OSK_SET_KEYBOARD_SIZE            SetKeyboardSize;
    MS_OSK_GET_KEYBOARD_MODE            GetKeyboardMode;
    MS_OSK_SET_KEYBOARD_MODE            SetKeyboardMode;
    MS_OSK_GET_KEYBOARD_BOUNDS          GetKeyboardBounds;
};


extern EFI_GUID     gMsOSKProtocolGuid;

#endif      // _OSK_PROTOCOL_H_
