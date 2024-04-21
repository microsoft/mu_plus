/** @file
  STANDALONE_MM_CORE and STANDALONE_MM implementation of Advanced Logger Library.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <AdvancedLoggerInternal.h>

#include <Library/AdvancedLoggerLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/SynchronizationLib.h>

#include "../AdvancedLoggerCommon.h"

STATIC ADVANCED_LOGGER_INFO  *mLoggerInfo = NULL;
STATIC UINT32                mBufferSize  = 0;
STATIC EFI_PHYSICAL_ADDRESS  mMaxAddress  = 0;

/**
  Validate Info Blocks

  The address of the ADVANCE_LOGGER_INFO block pointer is captured during the Constructor.  The
  pointers LogBuffer and LogCurrent, and LogBufferSize, could be written to by untrusted code.  Here, we check that
  the pointers are within the allocated mLoggerInfo space, and that LogBufferSize, which is used in multiple places
  to see if a new message will fit into the log buffer, is valid.

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

  if (PA_FROM_PTR (LOG_CURRENT_FROM_ALI (mLoggerInfo)) > mMaxAddress ||
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
  Get the Logger Information Block published by PeiCore.

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
  EFI_HOB_GUID_TYPE    *GuidHob;
  ADVANCED_LOGGER_PTR  *LogPtr;
  STATIC BOOLEAN       Initialized = FALSE;

  if (!Initialized) {
    Initialized = TRUE;   // Only one attempt at getting the logger info block.

    //
    // Locate the Logger Information block.
    //
    GuidHob = GetFirstGuidHob (&gAdvancedLoggerHobGuid);
    if (GuidHob == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Advanced Logger Hob not found\n", __FUNCTION__));
    } else {
      LogPtr      = (ADVANCED_LOGGER_PTR *)GET_GUID_HOB_DATA (GuidHob);
      mLoggerInfo = ALI_FROM_PA (LogPtr->LogBuffer);
      if (mLoggerInfo != NULL) {
        mMaxAddress = LOG_MAX_ADDRESS (mLoggerInfo);
      }

      //
      // If mLoggerInfo is NULL at this point, there is no Advanced Logger.
      //
      DEBUG ((DEBUG_INFO, "%a: LoggerInfo=%p\n", __FUNCTION__, mLoggerInfo));
    }
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
  return ADVANCED_LOGGER_PHASE_MM_CORE;
}

/**
  The constructor function initializes Logger Information pointer to ensure that the
  pointer is initialized when MM is loaded, either by the constructor, or the first DEBUG message.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
MmAdvancedLoggerLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  AdvancedLoggerGetLoggerInfo ();

  ASSERT (mLoggerInfo != NULL);

  return EFI_SUCCESS;
}
