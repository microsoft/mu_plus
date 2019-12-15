/** @file
PrintScreenLogger.h

PrintScreen logger to capture UEFI menus into a BMP written to a USB key

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PRINTSCREEN_LOGGER_H__
#define __PRINTSCREEN_LOGGER_H__

#include <Uefi.h>

#include <IndustryStandard/Bmp.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/SimpleTextInEx.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#define PRINT_SCREEN_ENABLE_FILENAME   L"PrintScreenEnable.txt"
#define MAX_PRINT_SCREEN_FILES         512
#define PRINT_SCREEN_DEBUG_WARNING     32

//
// Print Screen Delay.  There appears to be no way to see the difference between
// PrtScn key down and key up.  So, we get called twice.  Also, the PrtScn key appears
// to have repeat enabled.  To prevent duplicate screen captures, this code ignores
// PrtScn keys for 3 seconds after completing a PrtScn.
//
// 3 seconds in 100ns intervals = 3 * ms in 1 second * us in 1 ms * 100ns in 1us
#define PRINT_SCREEN_DELAY       (3 * 1000           * 1000       * 10)

#endif  // __PRINTSCREEN_LOGGER_H__
