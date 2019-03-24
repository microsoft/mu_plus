/** @file

  Implements a simple editbox control.

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

#ifndef _UIT_EDIT_BOX_H_
#define _UIT_EDIT_BOX_H_

#define UIT_EDITBOX_MAX_STRING_LENGTH       128


// EditBox types.
//
typedef enum
{
    UIT_EDITBOX_TYPE_SELECTABLE,
    UIT_EDITBOX_TYPE_NORMAL,
    UIT_EDITBOX_TYPE_PASSWORD
} UIT_EDITBOX_TYPE;


//////////////////////////////////////////////////////////////////////////////
// EditBox Class Definition
//
typedef struct _EditBox
{
    // *** Base Class ***
    //
    ControlBase                     Base;

    // *** Member variables ***
    //
    UINT32                          m_CurrentPosition;
    UINT32                          m_DisplayStartPosition;
    UINT32                          m_CharWidth;
    UIT_EDITBOX_TYPE                m_Type;
    SWM_RECT                        m_EditBoxBounds;
    UINT32                          m_MaxDisplayChars;
    CHAR16                          m_EditBoxText[UIT_EDITBOX_MAX_STRING_LENGTH + 1];           // Include NULL terminator.
    CHAR16                          m_EditBoxDisplayText[UIT_EDITBOX_MAX_STRING_LENGTH + 1];    // Include NULL terminator.
    CHAR16                          m_EditBoxWatermarkText[UIT_EDITBOX_MAX_STRING_LENGTH + 1];  // Include NULL terminator.
    SWM_RECT                        m_EditBoxTextBounds;
    OBJECT_STATE                    m_State;
    EFI_EVENT                       m_HidePasswordEvent;
    BOOLEAN                         m_KeyboardEnabled;

    EFI_FONT_INFO                  *m_FontInfo;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_NormalColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_NormalTextColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_GrayOutColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_GrayOutTextColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_SelectTextColor;

    VOID                            *m_pSelectionContext;

    // *** Functions ***
    //
    VOID
    (*Ctor)(IN struct _EditBox                *this,
            IN UINT32                         OrigX,
            IN UINT32                         OrigY,
            IN UINT32                         MaxDisplayChars,
            IN UIT_EDITBOX_TYPE               Type,
            IN EFI_FONT_INFO                  *FontInfo,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pNormalColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pNormalTextColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pGrayOutColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pGrayOutTextColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pSelectTextColor,
            IN CHAR16                         *pWatermarkText,
            IN VOID                           *pSelectionContext);

    EFI_STATUS
    (*ClearEditBox)(IN  struct _EditBox     *this);

    EFI_STATUS
    (*WipeBuffer)(IN  struct _EditBox     *this);

    CHAR16 *
    (*GetCurrentTextString)(IN  struct _EditBox     *this);

    EFI_STATUS
    (*SetCurrentTextString)(IN  struct _EditBox     *this,
                            IN  CHAR16              *NewTextString);

} EditBox;


//////////////////////////////////////////////////////////////////////////////
// Public
//
EditBox *new_EditBox(IN UINT32                              OrigX,
                     IN UINT32                              OrigY,
                     IN UINT32                              MaxDisplayChars,
                     IN UIT_EDITBOX_TYPE                    Type,
                     IN EFI_FONT_INFO                       *FontInfo,
                     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *pNormalColor,
                     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *pNormalTextColor,
                     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *pGrayOutColor,
                     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *pGrayOutTextColor,
                     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *pSelectTextColor,
                     IN CHAR16                              *pWatermarkText,
                     IN VOID                                *pSelectionContext);

VOID delete_EditBox(IN EditBox *this);


#endif  // _UIT_EDIT_BOX_H_.
