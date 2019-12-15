/** @file

  This file contains include headers for MsEarlyGraphics

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
