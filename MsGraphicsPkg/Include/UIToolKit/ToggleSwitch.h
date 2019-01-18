/** @file

  Implements a simple toggle switch control.

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

#ifndef _UIT_TOGGLE_SWITCH_H_
#define _UIT_TOGGLE_SWITCH_H_


// Toggle switch display context.
//
typedef struct _ToggleSwitchDisplayInfo
{
    SWM_RECT                        ToggleSwitchBounds;         // Toggle switch bounding rectangle.
    SWM_RECT                        ToggleSwitchTextBounds;     // Toggle switch text bounding rectangle.
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *SwitchOnBitmap;            // Toggle switch "On" bitmap.
    CHAR16                          *pToggleSwitchOnText;       // Toggle switch "On" text label.
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *SwitchOffBitmap;           // Toggle switch "Off" bitmap.
    CHAR16                          *pToggleSwitchOffText;      // Toggle switch "Off" text label.
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *GrayedSwitchOnBitmap;      // Grayed Out Toggle switch "On" bitmap.
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *GrayedSwitchOffBitmap;     // Grayed Out Toggle switch "Off" bitmap.
    UINT32                          SwitchBitmapWidth;          // Toggle switch bitmap width (in pixels).
    UINT32                          SwitchBitmapHeight;         // Toggle switch bitmap height (in pixels).
    OBJECT_STATE                    State;                      // Toggle switch object state.

} ToggleSwitchDisplayInfo;


//////////////////////////////////////////////////////////////////////////////
// ToggleSwitch Class Definition
//
typedef struct _ToggleSwitch
{
    // *** Base Class ***
    //
    ControlBase                     Base;

    // *** Member variables ***
    //
    EFI_FONT_INFO                  *m_FontInfo;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_OnColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_OffColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_HoverColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_GrayOutColor;

    ToggleSwitchDisplayInfo         *m_pToggleSwitch;
    BOOLEAN                         m_CurrentState;
    VOID                            *m_pSelectionContext;

    // *** Functions ***
    //
    VOID
    (*Ctor)(IN struct _ToggleSwitch           *this,
            IN SWM_RECT                       ToggleSwitchBox,
            IN EFI_FONT_INFO                  *FontInfo,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  OnColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  OffColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  HoverColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  GrayOutColor,
            IN CHAR16                         *pToggleSwitchOnText,
            IN CHAR16                         *pToggleSwitchOffText,
            IN BOOLEAN                        InitialState,
            IN VOID                           *pSelectionContext);

} ToggleSwitch;


//////////////////////////////////////////////////////////////////////////////
// Public
//
ToggleSwitch *new_ToggleSwitch(IN UINT32                            OrigX,
                               IN UINT32                            OrigY,
                               IN UINT32                            ToggleSwitchWidth,
                               IN UINT32                            ToggleSwitchHeight,
                               IN EFI_FONT_INFO                     *FontInfo,
                               IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     OnColor,
                               IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     OffColor,
                               IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     HoverColor,
                               IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     GrayOutColor,
                               IN CHAR16                            *pToggleSwitchOnText,
                               IN CHAR16                            *pToggleSwitchOffText,
                               IN BOOLEAN                           InitialState,
                               IN VOID                              *pSelectionContext);

VOID delete_ToggleSwitch(IN ToggleSwitch *this);


#endif  // _UIT_TOGGLE_SWITCH_H_.
