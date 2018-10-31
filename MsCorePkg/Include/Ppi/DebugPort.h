/** @file

Debug Port PEI PPI header

Copyright (c) 2018, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
