/** @file
  Default AdvancedLoggerPrintToHwPort implementation, shared by the Advanced Logger library
  instances that do not override it for special hardware port handling.

  Decides whether a message should be written to the hardware port based on the logger info
  block and the configured hardware port debug level.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>

#include <AdvancedLoggerInternal.h>

#include "AdvancedLoggerCommon.h"

/**
  Returns whether the given message should be written to the hardware port for this
  Advanced Logger instance.

  @param  LoggerInfo  The logger info block, or NULL if it is not available.
  @param  DebugLevel  The debug level of the message being logged.

  @retval TRUE   The message should be written to the hardware port.
  @retval FALSE  The message should not be written to the hardware port.
**/
BOOLEAN
EFIAPI
AdvancedLoggerPrintToHwPort (
  IN ADVANCED_LOGGER_INFO  *LoggerInfo,
  IN UINTN                 DebugLevel
  )
{
  if ((LoggerInfo != NULL) && (LoggerInfo->HdwPortDisabled)) {
    return FALSE;
  }

  return AdvancedLoggerHwPortLevelEnabled (LoggerInfo, DebugLevel);
}
