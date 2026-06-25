/** @file
  SEC implementation of the Advanced Logger library for Aarch64 platforms.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>

#include <AdvancedLoggerInternal.h>

#include <Library/AdvancedLoggerLib.h>
#include <Library/AdvancedLoggerHdwPortLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/SynchronizationLib.h>

#include "../AdvancedLoggerCommon.h"

/**
  Validate Info Blocks

  The address of the ADVANCE_LOGGER_INFO block pointer is captured during the first debug print.
  Offsets LogBufferOffset, LogCurrentOffset, and LogBufferSize, could be written to by untrusted code.  Here,
  we check that the pointers are within the allocated mLoggerInfo space, and that LogBufferSize, which
  is used in multiple places to see if a new message will fit into the log buffer, is valid.

  @param          LoggerInfo  Logger information pointer needs to be validated.

  @return         BOOLEAN     TRUE = mLoggerInfo Block passes security checks
  @return         BOOLEAN     FALSE= mLoggerInfo Block failed security checks

**/
STATIC
BOOLEAN
ValidateInfoBlock (
  IN  ADVANCED_LOGGER_INFO  *LoggerInfo
  )
{
  if (LoggerInfo == NULL) {
    return FALSE;
  }

  if (LoggerInfo->Signature != ADVANCED_LOGGER_SIGNATURE) {
    return FALSE;
  }

  if (LoggerInfo->LogBufferOffset != EXPECTED_LOG_BUFFER_OFFSET (LoggerInfo)) {
    return FALSE;
  }

  if ((LoggerInfo->LogCurrentOffset > TOTAL_LOG_SIZE_WITH_ALI (LoggerInfo)) ||
      (LoggerInfo->LogCurrentOffset < LoggerInfo->LogBufferOffset))
  {
    return FALSE;
  }

  return TRUE;
}

EFI_STATUS
EFIAPI
AdvancedLoggerLibConstructor (
  VOID
  )
{
  ADVANCED_LOGGER_PTR   *LogPtr;
  ADVANCED_LOGGER_INFO  *LoggerInfo;

  // Initialize the fixed memory LogPtr structure to no address, with a signature.
  UINTN  LogBufferSize;

  LogBufferSize = EFI_PAGES_TO_SIZE (FixedPcdGet64 (PcdAdvancedLoggerPages));

  LoggerInfo = ALI_FROM_PA (FixedPcdGet64 (PcdAdvancedLoggerBase));
  //
  // Buffer must be large enough to hold the header plus some payload.
  //
  if ((LoggerInfo != NULL) && (LogBufferSize > sizeof (ADVANCED_LOGGER_INFO))) {
    // Check if we need to initialize the buffer or if pre-UEFI did
    if (!ValidateInfoBlock (LoggerInfo)) {
      ZeroMem ((VOID *)LoggerInfo, sizeof (ADVANCED_LOGGER_INFO));
      LoggerInfo->Signature        = ADVANCED_LOGGER_SIGNATURE;
      LoggerInfo->Version          = ADVANCED_LOGGER_INFO_VER;
      LoggerInfo->LogBufferSize    = (UINT32)(LogBufferSize - sizeof (ADVANCED_LOGGER_INFO));
      LoggerInfo->LogBufferOffset  = EXPECTED_LOG_BUFFER_OFFSET (LoggerInfo);
      LoggerInfo->LogCurrentOffset = LoggerInfo->LogBufferOffset;
    }

    // Update these to have the UEFI settings
    LoggerInfo->HdwPortInitialized = TRUE;
    LoggerInfo->HwPrintLevel       = FixedPcdGet32 (PcdAdvancedLoggerHdwPortDebugPrintErrorLevel);
    LoggerInfo->InPermanentRAM     = TRUE;
    AdvancedLoggerHdwPortInitialize ();
    DEBUG ((DEBUG_INFO, "%a: Advanced Logger initialized at fixed RAM address %p\n", __func__, LoggerInfo));

    // Dump the header contents for debug
    DEBUG ((DEBUG_INFO, "%a: Logger Info Header:\n", __FUNCTION__));
    DEBUG ((DEBUG_INFO, "  Signature:          0x%08X\n", LoggerInfo->Signature));
    DEBUG ((DEBUG_INFO, "  Version:            0x%04X\n", LoggerInfo->Version));
    DEBUG ((DEBUG_INFO, "  LogBufferOffset:    0x%08X\n", LoggerInfo->LogBufferOffset));
    DEBUG ((DEBUG_INFO, "  LogCurrentOffset:   0x%08X\n", LoggerInfo->LogCurrentOffset));
    DEBUG ((DEBUG_INFO, "  LogBufferSize:      0x%08X\n", LoggerInfo->LogBufferSize));

    // Create the hob here so that DXE core can find it.
    LogPtr            = BuildGuidHob (&gAdvancedLoggerHobGuid, sizeof (ADVANCED_LOGGER_INFO));
    LogPtr->Signature = ADVANCED_LOGGER_PTR_SIGNATURE;
    LogPtr->LogBuffer = PA_FROM_PTR (LoggerInfo);
  }

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
