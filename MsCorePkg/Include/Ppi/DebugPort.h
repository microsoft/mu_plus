/** @file

Debug Port PEI PPI header

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DEBUG_PORT_PPI_H_
#define _DEBUG_PORT_PPI_H_

/**
  Function pointer for PPI routing to correct DebugPrint function

  @param  ErrorLevel  The error level of the debug message.
  @param  Format      Format string for the debug message to print.
  @param  VaListMarker  VA_LIST marker for the variable argument list.

**/
typedef
VOID
(EFIAPI *DEBUG_PORT_PRINT)(
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  VA_LIST          VaListMarker
  );

/**
  Function pointer for PPI routing to correct DebugAssert function

  @param  FileName     The pointer to the name of the source file that generated the assert condition.
  @param  LineNumber   The line number in the source file that generated the assert condition
  @param  Description  The pointer to the description of the assert condition.

**/
typedef
VOID
(EFIAPI *DEBUG_PORT_ASSERT)(
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  );

/**
  Function pointer for PPI routing to correct DebugDumpMemory function

  @param  Address      The address of the memory to dump.
  @param  Length       The length of the region to dump.
  @param  Flags        PrintAddress, PrintOffset etc

**/
typedef
VOID
(EFIAPI *DEBUG_PORT_DUMP_MEMORY)(
  IN UINTN        ErrorLevel,
  IN CONST VOID   *Address,
  IN UINTN        Length,
  IN UINT32       Flags
  );
  
typedef struct {
  DEBUG_PORT_PRINT            DebugPortPrint;
  DEBUG_PORT_ASSERT           DebugPortAssert;
  DEBUG_PORT_DUMP_MEMORY      DebugPortDumpMemory;
} DEBUG_PORT_PPI;

extern EFI_GUID gDebugPortPpiGuid;

#endif
