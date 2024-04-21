/** @file
  MM_CORE Arm implementation of Advanced Logger Library.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>

#include <AdvancedLoggerInternal.h>

#include <Library/AdvancedLoggerLib.h>
#include <Library/DebugLib.h>
#include <Library/SynchronizationLib.h>

#include "../AdvancedLoggerCommon.h"

STATIC ADVANCED_LOGGER_INFO  *mLoggerInfo = NULL;
STATIC UINT32                mBufferSize  = 0;
STATIC EFI_PHYSICAL_ADDRESS  mMaxAddress  = 0;
STATIC BOOLEAN               mInitialized = FALSE;

/**
  Validate Info Blocks

  The address of the ADVANCE_LOGGER_INFO block pointer is captured during the first DEBUG print.
  LogBufferOffset, LogCurrentOffset, and LogBufferSize could be written to by untrusted code.  Here,
  we check that the offsets are within the allocated mLoggerInfo space, and that LogBufferSize, which
  is used in multiple places to see if a new message will fit into the log buffer, is valid.

  @param          NONE

  @return         BOOLEAN     TRUE = mLoggerInfo Block passes security checks
  @return         BOOLEAN     FALSE= mLoggerInfo Block failed security checks

**/
STATIC
BOOLEAN
ValidateInfoBlock (
  VOID
  )
{
  if (mLoggerInfo == NULL) {
    return FALSE;
  }

  if (mLoggerInfo->Signature != ADVANCED_LOGGER_SIGNATURE) {
    return FALSE;
  }

  if (mLoggerInfo->LogBufferOffset != EXPECTED_LOG_BUFFER_OFFSET (mLoggerInfo)) {
    return FALSE;
  }

  if (PA_FROM_PTR (LOG_CURRENT_FROM_ALI (mLoggerInfo)) > mMaxAddress) ||
      (mLoggerInfo->LogCurrentOffset < mLoggerInfo->LogBufferOffset))
  {
    return FALSE;
  }

  if (mBufferSize == 0) {
    mBufferSize = mLoggerInfo->LogBufferSize;
  } else if (mLoggerInfo->LogBufferSize != mBufferSize) {
    return FALSE;
  }

  return TRUE;
}

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

  NOTE:  A DEBUG(()) statements here will cause recursion.  Insure that the recursion will be
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
  if (!mInitialized) {
    mInitialized = TRUE;            // Only allow initialization once
    mLoggerInfo  = (ADVANCED_LOGGER_INFO *)(VOID *)FixedPcdGet64 (PcdAdvancedLoggerBase);
    ASSERT (mLoggerInfo != NULL);
    if (mLoggerInfo == NULL) {
      return NULL;
    }

    mMaxAddress = LOG_MAX_ADDRESS (mLoggerInfo);
    mBufferSize = mLoggerInfo->LogBufferSize;
  }

  if (((mLoggerInfo) != NULL) && !ValidateInfoBlock ()) {
    mLoggerInfo = NULL;
    DEBUG ((DEBUG_ERROR, "%a: LoggerInfo marked invalid\n", __FUNCTION__));
  }

  return mLoggerInfo;
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
  return ADVANCED_LOGGER_PHASE_MM;
}
