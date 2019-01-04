/** @file

  This file contains include headers for MsEarlyGraphics

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

#ifndef _MS_EARLY_GRAPHICS_PEI_H_
#define _MS_EARLY_GRAPHICS_PEI_H_

#include <Uefi.h>

#include <Protocol/GraphicsOutput.h>
#include <Ppi/Graphics.h>
#include <Guid/MsEarlyGraphicsHob.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MsPlatformEarlyGraphicsLib.h>
#include <Library/MsUiThemeLib.h>

#include "../MsEarlyGraphicsCommon.h"

#endif  // _MS_EARLY_GRAPHICS_PEI_H_
