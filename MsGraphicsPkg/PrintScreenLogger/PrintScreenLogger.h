/** @file
PrintScreenLogger.h

PrintScreen logger to capture UEFI menus into a BMP written to a USB key

Copyright (c) 2018, Microsoft Corporation

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

#ifndef __PRINTSCREEN_LOGGER_H__
#define __PRINTSCREEN_LOGGER_H__

#include <Uefi.h>
#include <Uefi\UefiInternalFormRepresentation.h>

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
