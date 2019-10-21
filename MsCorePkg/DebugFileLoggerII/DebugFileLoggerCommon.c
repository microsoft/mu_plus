/** @file DebugFileLoggerCommon.c

    Copyright (C) Microsoft Corporation. All rights reserved.
    SPDX-License-Identifier: BSD-2-Clause-Patent

    This file contains common functions (i.e., between PEI and DXE drivers) to support debug logging.

**/

#include <Guid/StatusCodeDataTypeId.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/SafeIntLib.h>
#include <Library/SynchronizationLib.h>

#include "DebugFileLoggerCommon.h"

/**
    Convert status code value and extended data to readable ASCII string and write it into the caller-provided buffer

    @param    CodeType    Indicates the type of status code being reported.
    @param    Value       Describes the current status of a hardware or software entity.
                          This included information about the class and subclass that is used to
                          classify the entity as well as an operation.
    @param    Instance    The enumeration of a hardware or software entity within
                          the system. Valid instance numbers start with 1.
    @param    CallerId    This optional parameter may be used to identify the caller.
                          This parameter allows the status code driver to apply different rules to
                          different callers.
    @param    Data        This optional parameter may be used to pass additional data.
    @param    Buffer      Pointer to a buffer that receives ASCII-encoded StatusCode information.
    @param    BufferSize  Size of Buffer.

    @retval   Number of characters written into Buffer (zero indicates error).

    NOTE: This routine is equivalent to SerialStatusCodeReportWorker but without the serial print.
**/

UINTN
WriteStatusCodeToBuffer (
    IN EFI_STATUS_CODE_TYPE           CodeType,
    IN EFI_STATUS_CODE_VALUE          Value,
    IN UINT32                         Instance,
    IN CONST EFI_GUID                *CallerId,
    IN CONST EFI_STATUS_CODE_DATA    *Data OPTIONAL,
    OUT CHAR8                        *Buffer,
    IN UINTN                          BufferSize
    )
{
    UINTN           CharCount = 0;
    CHAR8          *Description;
    UINT32          ErrorLevel;
    CHAR8          *Filename;
    CHAR8          *Format;
    UINT32          LineNumber = 0;
    BASE_LIST       Marker;
    UINTN           Result = 0;

    if (Data != NULL &&
        ReportStatusCodeExtractAssertInfo (CodeType, Value, Data, &Filename, &Description, &LineNumber)) {
        //
        // Print ASSERT() information into output buffer.
        //
        CharCount = AsciiSPrint (Buffer,
                                 BufferSize,
                                 "\n\rDXE_ASSERT!: %a (%d): %a\n\r",
                                 Filename,
                                 LineNumber,
                                 Description);
    } else if (Data != NULL &&
               ReportStatusCodeExtractDebugInfo (Data, &ErrorLevel, &Marker, &Format)) {
        //
        // Print DEBUG() information into output buffer.
        //
        CharCount = AsciiBSPrint (Buffer,
                                  BufferSize,
                                  Format,
                                  Marker);
    } else if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_ERROR_CODE) {
        //
        // Print ERROR information into output buffer.
        //
        CharCount = AsciiSPrint (Buffer,
                                 BufferSize,
                                 "ERROR: C%x:V%x I%x",
                                 CodeType,
                                 Value,
                                 Instance);
        if (CallerId != NULL) {
            if (SafeUintnSub (BufferSize, (sizeof (*Buffer) * CharCount), &Result) == RETURN_SUCCESS) {
                CharCount += AsciiSPrint (&Buffer[CharCount],
                                           Result,
                                           " %g",
                                           CallerId);
            }
        }

        if (Data != NULL) {
            if (SafeUintnSub (BufferSize, (sizeof (*Buffer) * CharCount), &Result) == RETURN_SUCCESS) {
                CharCount += AsciiSPrint(&Buffer[CharCount],
                                          Result,
                                          " %x",
                                          Data);
            }
        }

        if (SafeUintnSub (BufferSize, (sizeof (*Buffer) * CharCount), &Result) == RETURN_SUCCESS) {
            CharCount += AsciiSPrint (&Buffer[CharCount],
                                       Result,
                                       "\n\r");
        }
    } else if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_PROGRESS_CODE) {
        //
        // Print PROGRESS information into output buffer.
        //
        CharCount = AsciiSPrint (Buffer,
                                 BufferSize,
                                 "PROGRESS CODE: V%x I%x\n\r",
                                 Value,
                                 Instance);
    } else if (Data != NULL &&
               CompareGuid (&Data->Type, &gEfiStatusCodeDataTypeStringGuid) &&
               ((EFI_STATUS_CODE_STRING_DATA *) Data)->StringType == EfiStringAscii) {
        //
        // EFI_STATUS_CODE_STRING_DATA
        //
        CharCount = AsciiSPrint (Buffer,
                                 BufferSize,
                                 "%a\n\r",
                                 ((EFI_STATUS_CODE_STRING_DATA *) Data)->String.Ascii);
    } else {
        //
        // Code type is not defined.
        //
        CharCount = AsciiSPrint (Buffer,
                                 BufferSize,
                                 "Undefined: C%x:V%x I%x\n\r",
                                 CodeType,
                                 Value,
                                 Instance);
    }

    return CharCount;
}
