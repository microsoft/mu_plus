/** @file

  Implements internal Simple UI Toolkit library features.

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

#ifndef _SIMPLE_UI_TOOLKIT_INTERNAL_H_
#define _SIMPLE_UI_TOOLKIT_INTERNAL_H_


#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MsUiThemeLib.h>
#include <Library/MsColorTableLib.h>
#include <Library/MathLib.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleWindowManager.h>
#include <Protocol/HiiFont.h>
#include <Protocol/OnScreenKeyboard.h>

#include <UIToolKit/SimpleUIToolKit.h>


// Protocols.
//
extern EFI_GRAPHICS_OUTPUT_PROTOCOL         *mUITGop;
extern EFI_HII_FONT_PROTOCOL                *mUITFont;
extern MS_SIMPLE_WINDOW_MANAGER_PROTOCOL    *mUITSWM;
extern EFI_HANDLE                           mClientImageHandle;


#endif  // _SIMPLE_UI_TOOLKIT_INTERNAL_H_.
