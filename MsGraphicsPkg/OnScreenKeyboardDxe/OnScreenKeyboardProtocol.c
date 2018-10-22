/** @file

  Implements a simple on-screen virtual keyboard protocol.

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

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/OnScreenKeyboard.h>

#include "OnScreenKeyboard.h"


EFI_STATUS
OSKShowIcon (IN MS_ONSCREEN_KEYBOARD_PROTOCOL   *This,
             IN BOOLEAN                         bShowIcon)
{
    EFI_STATUS Status = EFI_SUCCESS;

    Status = ShowKeyboardIcon(bShowIcon);

    return Status;
}


EFI_STATUS
OSKShowDockAndCloseButtons (IN MS_ONSCREEN_KEYBOARD_PROTOCOL   *This,
                            IN BOOLEAN                         bShowDockAndCloseButtons)
{
    EFI_STATUS Status = EFI_SUCCESS;

    // Set the display state of the (un)dock and close buttons.
    //
    mOSK.bShowDockAndCloseButtons = bShowDockAndCloseButtons;

    return Status;
}


EFI_STATUS
OSKSetIconPosition (IN MS_ONSCREEN_KEYBOARD_PROTOCOL   *This,
                    IN SCREEN_POSITION                 Position)
{
    EFI_STATUS Status = EFI_SUCCESS;


    // Set the position
    //
    Status = SetKeyboardIconPosition(Position);

    return Status;
}


EFI_STATUS
OSKSetKeyboardPosition (IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
                        IN SCREEN_POSITION                  Position,
                        IN OSK_DOCKED_STATE                 DockedState)
{
    EFI_STATUS Status = EFI_SUCCESS;

    // Set the keyboard position, docked state, and force restoration of the underlying screen capture when repositioning.
    //
    Status = SetKeyboardPosition(Position, DockedState);

    return Status;
}


EFI_STATUS
OSKSetKeyboardRotationAngle (IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
                             IN SCREEN_ANGLE                     KeyboardAngle)
{
    EFI_STATUS Status = EFI_SUCCESS;

    // Rotate the keyboard to the specified angle about the z-axis.
    //
    Status = RotateKeyboard(KeyboardAngle);

    return Status;
}


EFI_STATUS
OSKGetKeyboardMode (IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
                    IN UINT32                           *ModeBitfield)
{
    EFI_STATUS Status = EFI_SUCCESS;


    // Retrieve keyboard operating mode(s).
    //
    Status = GetKeyboardMode (ModeBitfield);

    return Status;
}


EFI_STATUS
OSKSetKeyboardMode (IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
                    IN UINT32                           ModeBitfield)
{
    EFI_STATUS Status = EFI_SUCCESS;


    // Set keyboard operating mode.
    //
    Status = SetKeyboardMode (ModeBitfield);

    return Status;
}


EFI_STATUS
OSKSetKeyboardSize (IN MS_ONSCREEN_KEYBOARD_PROTOCOL    *This,
                    IN UINTN                             PercentOfScreenWidth)
{
    EFI_STATUS Status = EFI_SUCCESS;
    float Percent = (((float)PercentOfScreenWidth) / 100);

    // Set the keyboard size
    //
    Status = SetKeyboardSize(Percent);

    return Status;
}


EFI_STATUS
OSKShowKeyboard (IN MS_ONSCREEN_KEYBOARD_PROTOCOL   *This,
                 IN BOOLEAN                         bShowKeyboard)
{
    EFI_STATUS  Status = EFI_SUCCESS;

    // Show or hide the keyboard as requested
    //
    if (TRUE == bShowKeyboard)
    {
        // Show keyboard.
        //
        Status = ShowKeyboard(TRUE);
    }
    else
    {
        // Deselect any key currently being pressed.
        //
        mOSK.DeselectKey = mOSK.SelectedKey;
        mOSK.SelectedKey = NUMBER_OF_KEYS;

        // Hide keyboard.
        //
        Status = ShowKeyboard(FALSE);
    }

    return Status;
}


EFI_STATUS
OSKGetKeyboardBounds (IN  MS_ONSCREEN_KEYBOARD_PROTOCOL   *This,
                      OUT SWM_RECT                        *FrameRect)
{
    EFI_STATUS  Status = EFI_SUCCESS;


    GetKeyboardBoundingRect(FrameRect);

    return Status;
}

