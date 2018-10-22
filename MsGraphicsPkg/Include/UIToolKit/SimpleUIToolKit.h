/** @file

  Implements common Simple UI Toolkit features.

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
