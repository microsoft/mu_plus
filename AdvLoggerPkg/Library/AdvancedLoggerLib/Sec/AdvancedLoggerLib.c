/** @file
  SEC implementation of the Advanced Logger library.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>

#include <AdvancedLoggerInternal.h>

#include <Library/AdvancedLoggerLib.h>
#include <Library/AdvancedLoggerHdwPortLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/SynchronizationLib.h>

#include "../AdvancedLoggerCommon.h"

EFI_STATUS
EFIAPI
AdvancedLoggerLibConstructor (
  VOID
  )
{
  ADVANCED_LOGGER_PTR  *LogPtr;

  // Initialize the fixed memory LogPtr structure to no address, with a signature.

  LogPtr = (ADVANCED_LOGGER_PTR *)(UINTN)FixedPcdGet64 (PcdAdvancedLoggerBase);
  if (LogPtr != NULL) {
    LogPtr->LogBuffer = 0ULL;
    LogPtr->Signature = ADVANCED_LOGGER_PTR_SIGNATURE;
  }

  AdvancedLoggerHdwPortInitialize ();
  return EFI_SUCCESS;
}

/**
  Get the Logger Information block

 **/
ADVANCED_LOGGER_INFO *
EFIAPI
AdvancedLoggerGetLoggerInfo (
  VOID
  )
{
  ADVANCED_LOGGER_INFO  *LoggerInfoSec;
  ADVANCED_LOGGER_PTR   *LogPtr;

  // The SEC implementation requires a priori knowledge of an address in the heap to
  // use for the Logger Info block.

  // The PCD AdvancedLoggerBase MAY be a 64 bit address.  However, it is
  // trimmed to be a pointer the size of the actual platform SEC pointer - and
  // the Pcd is expected to be set properly for the platform.

  LoggerInfoSec = NULL;
  LogPtr        = (ADVANCED_LOGGER_PTR *)(UINTN)FixedPcdGet64 (PcdAdvancedLoggerBase);

  if ((LogPtr != NULL) &&
      (LogPtr->Signature == ADVANCED_LOGGER_PTR_SIGNATURE) &&
      (LogPtr->LogBuffer != 0ULL))
  {
    LoggerInfoSec = ALI_FROM_PA (LogPtr->LogBuffer);
  }

  return LoggerInfoSec;
}

/**
  Helper function to return the log phase for each message.

  This function is intended to be used to distinguish between
  various types of modules.

  @return       Phase of current advanced logger instance.
**/
UINT16
EFIAPI
AdvancedLoggerGetPhase (
  VOID
  )
{
  return ADVANCED_LOGGER_PHASE_SEC;
}

/**
  Returns whether the given message should be written to the hardware port for this SEC
  Advanced Logger instance.

  SEC instances use only the static hardware port debug level and do not consult the logger
  info block's dynamic hardware port level.

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

  return (BOOLEAN)((DebugLevel & PcdGet32 (PcdAdvancedLoggerHdwPortDebugPrintErrorLevel)) != 0);
}
