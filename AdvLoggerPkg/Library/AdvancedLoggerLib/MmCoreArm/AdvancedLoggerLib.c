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
#include <Library/ArmMmuLib.h>

#include "../AdvancedLoggerCommon.h"

#define ADV_LOGGER_MIN_SIZE  (65536)

STATIC ADVANCED_LOGGER_INFO  *mLoggerInfo = NULL;
STATIC UINT32                mBufferSize  = 0;
STATIC EFI_PHYSICAL_ADDRESS  mMaxAddress  = 0;
STATIC BOOLEAN               mInitialized = FALSE;

/**
  Validate Info Blocks

  The address of the ADVANCE_LOGGER_INFO block pointer is captured during the first debug print.  The
  pointers LogBuffer and LogCurrent, and LogBufferSize, could be written to by untrusted code.  Here,
  we check that the pointers are within the allocated mLoggerInfo space, and that LogBufferSize, which
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

  if (mLoggerInfo->LogBuffer != (PA_FROM_PTR (mLoggerInfo + 1))) {
    return FALSE;
  }

  if ((mLoggerInfo->LogCurrent > mMaxAddress) ||
      (mLoggerInfo->LogCurrent < mLoggerInfo->LogBuffer))
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

  NOTE:  A debug statement here will cause recursion.  Insure that the recursion will be
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
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Address;

  ASSERT (FeaturePcdGet (PcdAdvancedLoggerFixedInRAM));
  if (!FeaturePcdGet (PcdAdvancedLoggerFixedInRAM)) {
    return NULL;
  }

  if (!mInitialized) {
    // If this is the first time for MM core to get here, the memory attributes of this module
    // may not be fully set yet. Thus set the memory for global variables attributes to RW first.
    Address = ALIGN_VALUE ((EFI_PHYSICAL_ADDRESS)(UINTN)&mInitialized - EFI_PAGE_SIZE + 1, EFI_PAGE_SIZE);
    Status  = ArmSetMemoryRegionNoExec (Address, EFI_PAGE_SIZE);
    Status  = ArmClearMemoryRegionReadOnly (Address, EFI_PAGE_SIZE);

    Address = ALIGN_VALUE ((EFI_PHYSICAL_ADDRESS)(UINTN)&mLoggerInfo - EFI_PAGE_SIZE + 1, EFI_PAGE_SIZE);
    Status  = ArmSetMemoryRegionNoExec (Address, EFI_PAGE_SIZE);
    Status  = ArmClearMemoryRegionReadOnly (Address, EFI_PAGE_SIZE);

    mInitialized = TRUE;            // Only allow initialization once
    mLoggerInfo  = (ADVANCED_LOGGER_INFO *)(VOID *)FixedPcdGet64 (PcdAdvancedLoggerBase);
    ASSERT (mLoggerInfo != NULL);
    if (mLoggerInfo == NULL) {
      return NULL;
    }

    mLoggerInfo->HwPrintLevel = FixedPcdGet32 (PcdAdvancedLoggerHdwPortDebugPrintErrorLevel);

    mMaxAddress = mLoggerInfo->LogBuffer + mLoggerInfo->LogBufferSize;
    mBufferSize = mLoggerInfo->LogBufferSize;
  }

  if (((mLoggerInfo) != NULL) && !ValidateInfoBlock ()) {
    mLoggerInfo = NULL;
    DEBUG ((DEBUG_ERROR, "%a: LoggerInfo marked invalid\n", __FUNCTION__));
  }

  return mLoggerInfo;
}

/**
  Helper function to return the string prefix for each message.
  This function is intended to be used to distinguish between
  various types of modules.

  @param[out]   MessagePrefixSize  The size of the prefix string in bytes,
                excluding NULL terminator.

  @return       Pointer to the prefix string. NULL if no prefix is available.
**/
CONST CHAR8 *
EFIAPI
AdvancedLoggerGetStringPrefix (
  IN UINTN  *MessagePrefixSize
  )
{
  if (MessagePrefixSize == NULL) {
    return NULL;
  }

  *MessagePrefixSize = FixedPcdGetSize (PcdAdvancedLoggerMessageMmCoreArmPrefix) - 1;
  return (CHAR8 *)FixedPcdGetPtr (PcdAdvancedLoggerMessageMmCoreArmPrefix);
}
