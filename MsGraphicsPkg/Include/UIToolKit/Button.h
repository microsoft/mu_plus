/** @file

  Implements a simple button control.

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

#ifndef _UIT_BUTTON_H_
#define _UIT_BUTTON_H_


// Button display context information.
//
typedef struct _ButtonDisplayInfo
{
    SWM_RECT        ButtonBounds;
    CHAR16          *pButtonText;
    SWM_RECT        ButtonTextBounds;
    OBJECT_STATE    State;
} ButtonDisplayInfo;


//////////////////////////////////////////////////////////////////////////////
// Button Class Definition
//
typedef struct _Button
{
    // *** Base Class ***
    //
    ControlBase                     Base;

    // *** Member variables ***
    //
    EFI_FONT_INFO                  *m_FontInfo;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_NormalColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_SelectColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_HoverColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_GrayOutTextColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_NormalTextColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_SelectTextColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_ButtonRingColor;
    BOOLEAN                         m_ButtonDown;
    ButtonDisplayInfo              *m_pButton;
    VOID                           *m_pSelectionContext;

    // *** Functions ***
    //
    VOID
    (*Ctor)(IN struct _Button                *this,
            IN SWM_RECT                       ButtonBox,
            IN EFI_FONT_INFO                  *FontInfo,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pNormalColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pHoverColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pSelectColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pGrayOutTextColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pButtonRingColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pNormalTextColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pSelectTextColor,
            IN CHAR16                         *pButtonText,
            IN VOID                           *pSelectionContext);

} Button;


//////////////////////////////////////////////////////////////////////////////
// Public
//

// A flag to indicate that one or both of the button dimensions should be determined by the size of the text.
#define SUI_BUTTON_AUTO_SIZE        0

#define SUI_BUTTON_HIGHLIGHT_X_PAD  MsUiScaleByTheme (20)
#define SUI_BUTTON_HIGHLIGHT_Y_PAD  MsUiScaleByTheme (26)

Button *new_Button(IN UINT32                            OrigX,
                   IN UINT32                            OrigY,
                   IN UINT32                            ButtonWidth,
                   IN UINT32                            ButtonHeight,
                   IN EFI_FONT_INFO                     *FontInfo,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *pNormalColor,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *pHoverColor,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *pSelectColor,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *pGrayOutTextColor,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *pButtonRingColor,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *pNormalTextColor,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *pSelectTextColor,
                   IN CHAR16                            *pButtonText,
                   IN VOID                              *pSelectionContext);

VOID delete_Button(IN Button *this);


#endif  // _UIT_BUTTON_H_.
