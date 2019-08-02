/** @file

  Implements common structures and constants for a simple on-screen virtual keyboard protocol.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _OSK_PROTOCOL_IMPL_H_
#define _OSK_PROTOCOL_IMPL_H_

#include <Protocol/OnScreenKeyboard.h>


EFI_STATUS
OSKShowIcon (IN MS_ONSCREEN_KEYBOARD_PROTOCOL   *This,
             IN BOOLEAN                         bShowIcon);

EFI_STATUS
OSKShowDockAndCloseButtons (IN MS_ONSCREEN_KEYBOARD_PROTOCOL   *This,
                            IN BOOLEAN                         bShowDockAndCloseButtons);

EFI_STATUS
OSKSetIconPosition (IN MS_ONSCREEN_KEYBOARD_PROTOCOL   *This,
                    IN SCREEN_POSITION                 Position);

EFI_STATUS
OSKSetKeyboardPosition (IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
                        IN SCREEN_POSITION                  Position,
                        IN OSK_DOCKED_STATE                 DockedState);

EFI_STATUS
OSKSetKeyboardRotationAngle (IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
                             IN SCREEN_ANGLE                     KeyboardAngle);

EFI_STATUS
OSKSetKeyboardSize (IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
                    IN UINTN                             PercentOfScreenWidth);

EFI_STATUS
OSKGetKeyboardMode (IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
                    IN UINT32                           *ModeBitfield);

EFI_STATUS
OSKSetKeyboardMode (IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
                    IN UINT32                           ModeBitfield);

EFI_STATUS
OSKShowKeyboard (IN MS_ONSCREEN_KEYBOARD_PROTOCOL   *This,
                 IN BOOLEAN                         bShowKeyboard);

EFI_STATUS
OSKGetKeyboardBounds (IN  MS_ONSCREEN_KEYBOARD_PROTOCOL   *This,
                      OUT SWM_RECT                        *FrameRect);

#endif  // _OSK_PROTOCOL_IMPL_H_
