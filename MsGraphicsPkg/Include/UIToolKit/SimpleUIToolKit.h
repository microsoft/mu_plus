/** @file

  Implements common Simple UI Toolkit features.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SIMPLE_UI_TOOLKIT_H_
#define _SIMPLE_UI_TOOLKIT_H_

// Common control states.
//
typedef enum
{
    NORMAL,     // Normal state.
    HOVER,      // Hover state (mouse pointer hovers over contol).
    SELECT,     // Select state (object selected).
    GRAYED,     // Grayed state (object disabled).
    KEYFOCUS,   // Key focus state (object has input focus).
    KEYDEFAULT  // Key default state (object gets key input if the highlighted control doesn't claim it).
} OBJECT_STATE;

typedef enum
{
    BITMAP,
    BUTTON,
    CANVAS,
    EDITBOX,
    GRID,
    LABEL,
    LISTBOX,
    TOGGLESWITCH
} OBJECT_TYPE;


// Control classes.
//
#include <UIToolKit/ControlBase.h>
#include <UIToolKit/Bitmap.h>
#include <UIToolKit/Button.h>
#include <UIToolKit/Canvas.h>
#include <UIToolKit/EditBox.h>
#include <UIToolKit/Grid.h>
#include <UIToolKit/Label.h>
#include <UIToolKit/ListBox.h>
#include <UIToolKit/ToggleSwitch.h>
#include <UIToolKit/Utilities.h>


// Function prototypes.
//
EFI_STATUS
InitializeUIToolKit (IN EFI_HANDLE ImageHandle);

#endif  // _SIMPLE_UI_TOOLKIT_H_.
