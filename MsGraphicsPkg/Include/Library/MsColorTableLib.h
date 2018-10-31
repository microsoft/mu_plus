/**
  Color Table structure and instance definition
  
  Copyright (c) 2018,  Microsoft Corporation.

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

#ifndef _MS_COLOR_TABLE_LIB_H_
#define _MS_COLOR_TABLE_LIB_H_

#include <Protocol/GraphicsOutput.h>

typedef struct MS_COLOR_TABLE_T_DEF {
  //// Color definitions per view
  //  Label
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   LabelTextNormalColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   LabelTextLargeColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   LabelTextRedColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   LabelTextGrayoutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   LabelTextBackgroundColor;

  //  ListBox
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ListBoxNormalColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ListBoxHoverColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ListBoxSelectColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ListBoxGrayoutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ListBoxHighlightBoundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ListBoxSelectFGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ListBoxNormalFGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ListBoxGrayoutFGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ListBoxCheckBoxBackgroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ListBoxCheckBoxBoundGrayoutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ListBoxCheckBoxSelectBGGrayoutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ListBoxCheckBoxNormalBGGrayoutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ListBoxTranshanGrayoutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ListBoxTranshanSelectColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ListBoxTranshanNormalColor;

  //  EditBox
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   EditBoxNormalColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   EditBoxSelectColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   EditBoxGrayoutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   EditBoxTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   EditBoxTextGrayoutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   EditBoxHighlightBGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   EditBoxHighlightBoundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   EditBoxWaterMarkFGColor;

  //  Button
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ButtonNormalColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ButtonHoverColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ButtonSelectColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ButtonGrayoutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ButtonRingColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ButtonTextNormalColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ButtonTextSelectColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ButtonHighlightBoundColor;

  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ButtonLinkNormalColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ButtonLinkHoverColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ButtonLinkSelectColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ButtonLinkGrayoutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ButtonLinkRingColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ButtonLinkTextNormalColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ButtonLinkTextSelectColor;

  //  ToggleSwitch
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ToggleSwitchOnColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ToggleSwitchOffColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ToggleSwitchHoverColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ToggleSwitchGrayoutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ToggleSwitchTextFGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ToggleSwitchTextBGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ToggleSwitchHighlightBGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ToggleSwitchBackgroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   ToggleSwitchCircleGrayoutColor;

  //  Message Box
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MessageBoxTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MessageBoxTitleBarTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MessageBoxBackgroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MessageBoxBackgroundAlert1Color;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MessageBoxBackgroundAlert2Color;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MessageBoxDialogFrameColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MessageBoxButtonHoverColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MessageBoxButtonSelectColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MessageBoxButtonSelectAlert1Color;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MessageBoxButtonGrayoutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MessageBoxButtonRingColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MessageBoxButtonTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MessageBoxButtonSelectTextColor;

  //  Password 
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogTitleBarTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogErrorTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogEditBoxBackgroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogEditBoxTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogEditBoxGrayoutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogEditBoxGrayoutTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogTextSelectColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogBackGroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogFrameColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogButtonHoverColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogButtonSelectColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogButtonGrayOutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogButtonRingColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogButtonTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   PasswordDialogButtonSelectTextColor;

  //  SemmUserAuth
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthDialogTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthTitleBarTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthErrorTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthEditBoxBackGroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthEditBoxTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthEditBoxGrayOutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthEditBoxGrayOutTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthDialogTextSelectColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthDialogBackGroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthDialogFrameColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthDialogButtonHoverColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthDialogButtonSelectColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthDialogButtonGrayOutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthDialogButtonRingColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthDialogButtonTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   UserAuthDialogButtonSelectTextColor;

  //  Default
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultDialogTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultTitleBarTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultErrorTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultEditBoxBackGroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultEditBoxTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultEditBoxGrayOutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultEditBoxGrayOutTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultDialogTextSelectColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultDialogBackGroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultDialogFrameColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultDialogButtonHoverColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultDialogButtonSelectColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultDialogButtonGrayOutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultDialogButtonRingColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultDialogButtonTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   DefaultDialogButtonSelectTextColor;

  //  Single Select
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   SingleSelectDialogTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   SingleSelectDialogTitleBarTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   SingleSelectDialogDialogBackGroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   SingleSelectDialogDialogFrameColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   SingleSelectDialogButtonHoverColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   SingleSelectDialogButtonSelectColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   SingleSelectDialogButtonGrayoutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   SingleSelectDialogButtonRingColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   SingleSelectDialogButtonTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   SingleSelectDialogButtonSelectTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   SingleSelectDialogListBoxGreyoutColor;

  //  Test Background Colors
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   TestBackground1Color;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   TestBackground2Color;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   TestBackground3Color;

  //  On Screen Keyboard
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyLabelColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyShiftnNavFillColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyDefaultFillColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardSizeChangeBackgroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardDocknCloseForegroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardDocknCloseBackgroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardShiftStateKeyColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardShiftStateFGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardShiftStateBGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardCapsLockStateKeyColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardCapsLockStateFGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardCapsLockStateBGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardNumSymStateKeyColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardNumSymStateFGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardNumSymA0StateFGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardNumSymA0StateBGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardFunctionStateKeyColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardFunctionStateFGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardFunctionStateBGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardSelectedStateKeyColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardSelectedStateFGColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   KeyboardSelectedStateBGColor;

  //  Display Engine
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   TitleBarBackgroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   TitleBarTextColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MasterFrameBackgroundColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MasterFrameCellNormalColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MasterFrameCellHoverColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MasterFrameCellSelectColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   MasterFrameCellGrayoutColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   FormCanvasBackgroundColor;

} MS_COLOR_TABLE;

///
/// Contains instance to the Ms Color Table
///
extern MS_COLOR_TABLE  gMsColorTable;

#endif  // _MS_COLOR_TABLE_LIB_H_
