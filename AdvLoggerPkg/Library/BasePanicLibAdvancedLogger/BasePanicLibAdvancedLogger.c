/** @file
  An instance of the Panic Library that outputs to advanced logger.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/AdvancedLoggerLib.h>
#include <Library/PanicLib.h>

//
// Define the maximum panic message length that this library supports
//
#define MAX_PANIC_MESSAGE_LENGTH  0x100

/**
  Prints a panic message containing a filename, line number, and description.
  This is always followed by a dead loop.

  Print a message of the form "PANIC <FileName>(<LineNumber>): <Description>\n"
  to the debug output device.  Immediately after that CpuDeadLoop() is called.

  If FileName is NULL, then a <FileName> string of "(NULL) Filename" is printed.
  If Description is NULL, then a <Description> string of "(NULL) Description" is printed.

  @param  FileName     The pointer to the name of the source file that generated the panic condition.
  @param  LineNumber   The line number in the source file that generated the panic condition
  @param  Description  The pointer to the description of the panic condition.

**/
VOID
EFIAPI
PanicReport (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  )
{
  CHAR8  Buffer[MAX_PANIC_MESSAGE_LENGTH];

  if (FileName == NULL) {
    FileName = "(NULL) Filename";
  }

  if (Description == NULL) {
    Description = "(NULL) Description";
  }

  AsciiSPrint (Buffer, sizeof (Buffer), "PANIC [%a] %a(%d): %a\n", gEfiCallerBaseName, FileName, LineNumber, Description);

  // Note: 0x80000000 is used instead of DEBUG_ERROR to avoid a dependency on DebugLib.h just for that value.
  //       The high bit being set for error status codes is defined per the UEFI Specification (2.10 section below).
  //       https://uefi.org/specs/UEFI/2.10/Apx_D_Status_Codes.html#status-codes
  AdvancedLoggerWrite (0x80000000, Buffer, AsciiStrnLenS (Buffer, sizeof (Buffer)));

  //
  // Deadloop since the system is in an unrecoverable state.
  //
  CpuDeadLoop ();
}
