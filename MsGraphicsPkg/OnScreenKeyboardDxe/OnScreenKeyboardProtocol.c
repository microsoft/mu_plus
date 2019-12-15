/** @file

  Implements a simple on-screen virtual keyboard protocol.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
EFIAPI
OSKShowIcon (IN MS_ONSCREEN_KEYBOARD_PROTOCOL   *This,
             IN BOOLEAN                         bShowIcon)
{
    EFI_STATUS Status = EFI_SUCCESS;

    Status = ShowKeyboardIcon(bShowIcon);

    return Status;
}

EFI_STATUS
EFIAPI
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
EFIAPI
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
EFIAPI
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
EFIAPI
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
EFIAPI
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
EFIAPI
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
EFIAPI
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
EFIAPI
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
EFIAPI
OSKGetKeyboardBounds (IN  MS_ONSCREEN_KEYBOARD_PROTOCOL   *This,
                      OUT SWM_RECT                        *FrameRect)
{
    EFI_STATUS  Status = EFI_SUCCESS;


    GetKeyboardBoundingRect(FrameRect);

    return Status;
}

