/** @file

  Implements a simple toggle switch control.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
