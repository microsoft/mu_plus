/** @file
  MM_CORE Arm implementation of Advanced Logger Library.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <AdvancedLoggerInternal.h>

#include <Library/AdvancedLoggerLib.h>
#include <Library/AdvancedLoggerHdwPortLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/SynchronizationLib.h>

#include "../AdvancedLoggerCommon.h"

#define ADV_LOGGER_MIN_SIZE  (65536)

//
// NO GLOBALS! This routine may run before data sections are writable, and so
// cannot presume globals will be available.
//

/**
  The logger Information Block is carved from the Trust Zone at a specific fixed address.

  This address is obtained from the PcdAdvancedLoggerBase.  The size of the Advanced Logger
  buffer is obtained from PcdAdvancedLoggerPages.

  The following PCD settings are assumed:

  PcdAdvancedLoggerPeiInRAM    -- TRUE
  PcdAdvancedLoggerBase        -- NOT NULL and pointer to memory to be used
  PcdAdvancedLoggerPages       -- > 64KB of pages
  PcdAdvancedLoggerCarBase     -- NOT USED, leave at default
  PcdAdvancedLoggerPreMemPages -- NOT USED, leave at default

  NOTE:  A debug statement here will cause recursion. Ensure that the recursion will be
         a straight path just to return the existing mLoggerInfo.

  @param       - None

  @returns     - NULL - No valid Advanced Logger Info block available
               - Pointer to a Valid Advanced Logger Info block.

 **/
ADVANCED_LOGGER_INFO *
EFIAPI
AdvancedLoggerGetLoggerInfo (
  VOID
  )
{
  ADVANCED_LOGGER_INFO  *LoggerInfo;
  EFI_PHYSICAL_ADDRESS  MaxAddress;
  UINT32                HwPrintLevel;

  ASSERT (FeaturePcdGet (PcdAdvancedLoggerFixedInRAM));
  if (!FeaturePcdGet (PcdAdvancedLoggerFixedInRAM)) {
    return NULL;
  }

  LoggerInfo = (ADVANCED_LOGGER_INFO *)(VOID *)FixedPcdGet64 (PcdAdvancedLoggerBase);
  if (LoggerInfo == NULL) {
    return NULL;
  }

  // Initialize HdwPrintLevel if needed.
  HwPrintLevel = FixedPcdGet32 (PcdAdvancedLoggerHdwPortDebugPrintErrorLevel);
  if (LoggerInfo->HwPrintLevel != HwPrintLevel) {
    LoggerInfo->HwPrintLevel = HwPrintLevel;
  }

  //
  // The pointers LogBuffer and LogCurrent, and LogBufferSize, could be written
  // to by untrusted code.  Here, we check that the pointers are within the
  // allocated LoggerInfo space, and that LogBufferSize, which is used in
  // multiple places to see if a new message will fit into the log buffer, is
  // valid.
  //

  if (LoggerInfo->Signature != ADVANCED_LOGGER_SIGNATURE) {
    return NULL;
  }

  // Ensure the start of the log is in the correct location.
  if (LoggerInfo->LogBuffer != (PA_FROM_PTR (LoggerInfo + 1))) {
    return NULL;
  }

  // Make sure the size of the buffer does not overrun it's fixed size.
  if ((LoggerInfo->LogBuffer + LoggerInfo->LogBufferSize) >
      (FixedPcdGet32 (PcdAdvancedLoggerPages) * EFI_PAGE_SIZE))
  {
    return FALSE;
  }

  // Ensure the current pointer does not overrun.
  MaxAddress = LoggerInfo->LogBuffer + LoggerInfo->LogBufferSize;
  if ((LoggerInfo->LogCurrent > MaxAddress) ||
      (LoggerInfo->LogCurrent < LoggerInfo->LogBuffer))
  {
    return NULL;
  }

  return LoggerInfo;
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
  return ADVANCED_LOGGER_PHASE_MM_CORE;
}
