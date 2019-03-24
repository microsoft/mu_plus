/** @file

  Implements a simple label control for displaying text.

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

#ifndef _UIT_LABEL_H_
#define _UIT_LABEL_H_


//
typedef struct _LabelDisplayInfo
{
    SWM_RECT                        LabelBoundsLimit;       // Asbolute maximum label bounds allowed.
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
