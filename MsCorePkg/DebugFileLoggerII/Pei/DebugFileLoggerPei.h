/** @file DebugFileLoggerPEI.h

    Copyright (C) Microsoft Corporation. All rights reserved.
    SPDX-License-Identifier: BSD-2-Clause-Patent

    This file contains functions for logging debug print messages to a file.

**/

#ifndef DEBUG_FILE_LOGGER_PEI_H
#define DEBUG_FILE_LOGGER_PEI_H

#include <Ppi/ReportStatusCodeHandler.h>

#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/StatusCodeDataTypeDebug.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PrintLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/SynchronizationLib.h>

#include "../DebugFileLoggerCommon.h"


#endif  // _PEI_DEBUGFILE_LOGGER_H
