/** @file

  Implements common structures and constants for a simple on-screen virtual keyboard protocol.

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
