/** @file DebugFileLogger.h

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  This file contains functions for logging debug print messages to a file.

**/

#ifndef _COMMON_DEBUGFILE_LOGGER_H
#define _COMMON_DEBUGFILE_LOGGER_H

#include <Library/ReportStatusCodeLib.h>

#define PEI_BUFFER_SIZE_DEBUG_FILE_LOGGING  (6 * EFI_PAGE_SIZE)

#define EFI_DEBUG_FILE_LOGGER_OVERFLOW 0x80000000

typedef struct {

   UINT32 BytesWritten;

} EFI_DEBUG_FILELOGGING_HEADER;

UINTN
WriteStatusCodeToBuffer (
  IN EFI_STATUS_CODE_TYPE           CodeType,
  IN EFI_STATUS_CODE_VALUE          Value,
  IN UINT32                         Instance,
  IN CONST EFI_GUID                 *CallerId,
  IN CONST EFI_STATUS_CODE_DATA     *Data OPTIONAL,
  OUT CHAR8                         *Buffer,
  IN UINTN                          BufferSize
);

#endif  // _COMMON_DEBUGFILE_LOGGER_H
