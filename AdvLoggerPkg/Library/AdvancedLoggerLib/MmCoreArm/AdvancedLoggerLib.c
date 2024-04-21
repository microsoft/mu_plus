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

  NOTE:  A debug statement here will cause recursion. Avoid debug statements
         or calls to external functions that may contain debug prints.

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

  ASSERT (FeaturePcdGet (PcdAdvancedLoggerFixedInRAM));
  if (!FeaturePcdGet (PcdAdvancedLoggerFixedInRAM)) {
    return NULL;
  }

  LoggerInfo = (ADVANCED_LOGGER_INFO *)(VOID *)FixedPcdGet64 (PcdAdvancedLoggerBase);
  if (LoggerInfo == NULL) {
    return NULL;
  }

  //
  // LogBuffer and LogCurrent, and LogBufferSize, could be written
  // to by untrusted code.  Here, we check that the offsets are within the
  // allocated LoggerInfo space, and that LogBufferSize, which is used in
  // multiple places to see if a new message will fit into the log buffer, is
  // valid.
  //

  if (LoggerInfo->Signature != ADVANCED_LOGGER_SIGNATURE) {
    return NULL;
  }

  // Ensure the start of the log is in the correct location.
  if (LoggerInfo->LogBufferOffset != EXPECTED_LOG_BUFFER_OFFSET (LoggerInfo)) {
    return NULL;
  }

  // Make sure the size of the buffer does not overrun it's fixed size.
  MaxAddress = LOG_MAX_ADDRESS (mLoggerInfo);
  if ((MaxAddress - PA_FROM_PTR (LoggerInfo)) >
      (FixedPcdGet32 (PcdAdvancedLoggerPages) * EFI_PAGE_SIZE))
  {
    return NULL;
  }

  // Ensure the current pointer does not overrun.
  if ((LOG_CURRENT_FROM_ALI (LoggerInfo) > MaxAddress) ||
      (LoggerInfo->LogCurrentOffset < LoggerInfo->LogBufferOffset))
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
