/** @file
  SEC implementation of the Advanced Logger library.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

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
    ADVANCED_LOGGER_PTR    *LogPtr;

    // Initialize the fixed memory LogPtr structure to no address, with a signature.

    LogPtr = (ADVANCED_LOGGER_PTR *) FixedPcdGet64 (PcdAdvancedLoggerBase);
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
) {
    ADVANCED_LOGGER_INFO   *LoggerInfoSec;
    ADVANCED_LOGGER_PTR    *LogPtr;

    // The SEC implementation requires a priori knowledge of an address in the heap to
    // use for the Logger Info block.

    // The PCD AdvancedLoggerBase MAY be a 64 bit address.  However, it is
    // trimmed to be a pointer the size of the actual platform SEC pointer - and
    // the Pcd is expected to be set properly for the platform.

    LoggerInfoSec = NULL;
    LogPtr = (ADVANCED_LOGGER_PTR *) FixedPcdGet64 (PcdAdvancedLoggerBase);

    if ((LogPtr != NULL) &&
        (LogPtr->Signature == ADVANCED_LOGGER_PTR_SIGNATURE) &&
        (LogPtr->LogBuffer != 0ULL))
    {
        LoggerInfoSec = ALI_FROM_PA(LogPtr->LogBuffer);
    }

    return LoggerInfoSec;
}
