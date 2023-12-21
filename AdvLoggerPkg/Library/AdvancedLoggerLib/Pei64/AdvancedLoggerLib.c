/** @file
  PEI X64 Library Instance of the Advanced Logger library.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>

#include <AdvancedLoggerInternal.h>

#include <Library/AdvancedLoggerLib.h>
#include <Library/AdvancedLoggerHdwPortLib.h>
#include <Library/PcdLib.h>

#include "../AdvancedLoggerCommon.h"

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

  //
  // Locate the SEC Logger Information block.  The Ppi is not accessible in X64.PEIM.
  //
  // The PCD AdvancedLoggerBase MAY be a 64 bit address.  However, it is
  // trimmed to be a pointer the size of the actual platform - and the Pcd is expected
  // to be set accordingly.
  LogPtr        = (ADVANCED_LOGGER_PTR *)(UINTN)FixedPcdGet64 (PcdAdvancedLoggerBase);
  LoggerInfoSec = NULL;
  if ((LogPtr != NULL) &&
      (LogPtr->Signature == ADVANCED_LOGGER_PTR_SIGNATURE) &&
      (LogPtr->LogBuffer != 0ULL))
  {
    LoggerInfoSec = ALI_FROM_PA (LogPtr->LogBuffer);
    if (!LoggerInfoSec->HdwPortInitialized) {
      AdvancedLoggerHdwPortInitialize ();
      LoggerInfoSec->HdwPortInitialized = TRUE;
    }
  }

  return LoggerInfoSec;
}
