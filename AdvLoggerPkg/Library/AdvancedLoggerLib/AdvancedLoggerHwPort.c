/** @file
  Default AdvancedLoggerPrintToHwPort implementation, shared by the Advanced Logger library
  instances that do not override it for special hardware port handling at OS runtime.

  Permits hardware port writes at boot time, and at OS runtime unless a platform
  opts in to PcdAdvancedLoggerHdwPortOsRuntimeDisable. The DXE runtime instance provides its
  own implementation (it clears its logger info block at ExitBootServices) instead of using
  this file.

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>

#include <AdvancedLoggerInternal.h>

#include <Library/PcdLib.h>

#include "AdvancedLoggerCommon.h"

/**
  Returns whether this Advanced Logger instance permits writing debug output to the
  hardware port.

  Hardware port writes are always permitted unless the platform sets
  PcdAdvancedLoggerHdwPortOsRuntimeDisable, in which case they are suppressed once at OS
  runtime (after ExitBootServices), as reported by the logger info block's AtRuntime field.

  @param  LoggerInfo  The logger info block, or NULL if it is not available.

  @retval TRUE   Hardware port writes are permitted.
  @retval FALSE  Hardware port writes are currently suppressed (OS runtime, opt-in platform).
**/
BOOLEAN
EFIAPI
AdvancedLoggerPrintToHwPort (
  IN ADVANCED_LOGGER_INFO  *LoggerInfo
  )
{
  if (FeaturePcdGet (PcdAdvancedLoggerHdwPortOsRuntimeDisable) &&
      (LoggerInfo != NULL) &&
      (LoggerInfo->AtRuntime))
  {
    return FALSE;
  }

  return TRUE;
}
