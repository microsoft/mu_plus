/** @file

  Implements internal Simple UI Toolkit library features.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
