/** @file

  Implements a simple label control for displaying text.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UIT_LABEL_H_
#define _UIT_LABEL_H_


//
typedef struct _LabelDisplayInfo
{
    SWM_RECT                        LabelBoundsLimit;       // absolute maximum label bounds allowed.
    SWM_RECT                        LabelBoundsCurrent;     // Actual label bounds required for the current string.
    CHAR16                          *pLabelText;            // Label text string.
} LabelDisplayInfo;

//////////////////////////////////////////////////////////////////////////////
// Label Class Definition
//
typedef struct _Label
{
    // *** Base Class ***
    //
    ControlBase                     Base;

    // *** Member variables ***
    //
    EFI_FONT_INFO                  *m_FontInfo;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_TextColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_BackgroundColor;

    LabelDisplayInfo                *m_pLabel;

    // *** Functions ***
    //
    VOID
    (*Ctor)(IN struct _Label                  *this,
            IN SWM_RECT                       *LabelBox,
            IN EFI_FONT_INFO                  *FontInfo,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pTextColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pBackgroundColor,
            IN CHAR16                         *pLabelText);

    EFI_STATUS
    (*UpdateLabelText)(IN struct _Label           *this,
                       IN CHAR16                  *pLabelText);

} Label;


//////////////////////////////////////////////////////////////////////////////
// Public
//
Label *new_Label(IN UINT32                          OrigX,
                 IN UINT32                          OrigY,
                 IN UINT32                          LabelWidth,
                 IN UINT32                          LabelHeight,
                 IN EFI_FONT_INFO                   *FontInfo,
                 IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *pTextColor,
                 IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *pBackgroundColor,
                 IN CHAR16                          *pLabelText);

VOID delete_Label(IN Label *this);


#endif  // _UIT_LABEL_H_.
