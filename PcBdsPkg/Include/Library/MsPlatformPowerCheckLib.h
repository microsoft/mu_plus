/** @file
Library interface allowing Platform code to configure CPU Power Limits 

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MS_PLATFORM_POWER_CHECK_LIB_H__
#define __MS_PLATFORM_POWER_CHECK_LIB_H__

/**
Set CPU power limits based on Internal Battery RSoC

**/
VOID
EFIAPI
PlatformPowerLevelCheck();

#endif