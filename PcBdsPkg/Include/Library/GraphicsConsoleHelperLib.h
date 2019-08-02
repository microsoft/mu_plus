/** @file
This Graphics Console Helper Lib provides a method
for the BDS to change the graphics console/resolution

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _GRAPHICS_CONSOLE_HELPER_LIB_H_
#define _GRAPHICS_CONSOLE_HELPER_LIB_H_


typedef UINT8 GRAPHICS_CONSOLE_MODE;

#define GCM_LOW_RES    (1)
#define GCM_NATIVE_RES (2)

EFI_STATUS
EFIAPI
SetGraphicsConsoleMode(GRAPHICS_CONSOLE_MODE Mode);




#endif
