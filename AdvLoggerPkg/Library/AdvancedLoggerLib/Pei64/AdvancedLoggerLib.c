/** @file
  PEI X64 Library Instance of the Advanced Logger library.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <AdvancedLoggerInternal.h>

#include <Library/AdvancedLoggerLib.h>
#include <Library/PcdLib.h>
#include <Library/SerialPortLib.h>

#include "../AdvancedLoggerCommon.h"

/**
  Get the Logger Information block

 **/
ADVANCED_LOGGER_INFO *
EFIAPI
AdvancedLoggerGetLoggerInfo (
    VOID
) {
    ADVANCED_LOGGER_INFO       *LoggerInfoSec;
    ADVANCED_LOGGER_PTR        *LogPtr;

    //
    // Locate the SEC Logger Information block.  The Ppi is not accessible in X64.PEIM.
    //
    // The PCD AdvancedLoggerBase MAY be a 64 bit address.  However, it is
    // trimmed to be a pointer the size of the actual platform - and the Pcd is expected
    // to be set accordingly.
    LogPtr = (ADVANCED_LOGGER_PTR *) (VOID *) (UINTN) FixedPcdGet64 (PcdAdvancedLoggerBase);
    LoggerInfoSec = NULL;
    if (LogPtr != NULL) {
        LoggerInfoSec = ALI_FROM_PA(LogPtr->LoggerInfo);
    }

    return LoggerInfoSec;
}
