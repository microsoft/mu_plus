/** @file
  Base Assert library instance with telemetry
  Proviced the Assert functionality for DebugLib

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/AdvancedLoggerLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/MuTelemetryHelperLib.h>


//
// Define the maximum debug and assert message length that this library supports
//
#define MAX_DEBUG_MESSAGE_LENGTH  0x100

/**
  Prints an assert message containing a filename, line number, and description.
  This may be followed by a breakpoint or a dead loop.

  Print a message of the form "ASSERT <FileName>(<LineNumber>): <Description>\n"
  to the debug output device.  If DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED bit of
  PcdDebugPropertyMask is set then CpuBreakpoint() is called. Otherwise, if
  DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED bit of PcdDebugPropertyMask is set then
  CpuDeadLoop() is called.  If neither of these bits are set, then this function
  returns immediately after the message is printed to the debug output device.
  DebugAssert() must actively prevent recursion.  If DebugAssert() is called while
  processing another DebugAssert(), then DebugAssert() must return immediately.

  If FileName is NULL, then a <FileName> string of "(NULL) Filename" is printed.
  If Description is NULL, then a <Description> string of "(NULL) Description" is printed.

  @param  FileName     The pointer to the name of the source file that generated the assert condition.
  @param  LineNumber   The line number in the source file that generated the assert condition
  @param  Description  The pointer to the description of the assert condition.

**/
VOID
EFIAPI
DebugAssert (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  )
{
  CHAR8   Buffer[MAX_DEBUG_MESSAGE_LENGTH];

  // BEGIN LOGTELEMETRY
  // We use the first two bytes of Data1 to record the line number where the assert occurred, and the
  // remaining 14 bytes in both fields to record the last 14 characters of the file name minus the
  // extension. The file name includes the directory, so it should never be less than 14 characters
  // but we retain the logic to handle it if it occurs.
  // For example, if we were to hit the ASSERT on line 402 of MdeModulePkg\Bus\Pci\PciHostBridgeDxe\PciHostBridge.c,
  // we would have Data1 = 0x6F486963505C0192, and Data2 = 0x6567646972427473. This would give us a
  // filename string of "\PciHostBridge", and a line number of 0x0192 (402 in decimal).
  UINTN   FileNameLength  = 0;
  UINT64  Data1           = 0xFFFFFFFFFFFFFFFF;
  UINT64  Data2           = 0xFFFFFFFFFFFFFFFF;

  // Check to make sure the file name is valid and at least two characters long (which would be just the extension)
  if ((FileName != NULL) &&
      (AsciiStrLen (FileName) >= 2))
  {
    FileNameLength = AsciiStrLen (FileName) - (2 * sizeof (CHAR8)); // We don't care about the extension
  }
  // END LOGTELEMETRY

  //
  // Generate the ASSERT() message in ASCII format
  //
  AsciiSPrint (Buffer, sizeof (Buffer), "ASSERT [%a] %a(%d): %a\n", gEfiCallerBaseName, FileName, LineNumber, Description);

  //
  // Send the print string to the Logging device device
  //
  AdvancedLoggerWrite (DEBUG_ERROR, Buffer, AsciiStrLen (Buffer));

  //
  // Generate a Breakpoint, DeadLoop, or Telemetry based on PCD settings
  //
  // BEGIN LOGTELEMETRY
  if ((PcdGet8 (PcdDebugPropertyMask) & DEBUG_PROPERTY_ASSERT_TELEMETRY_ENABLED) != 0) {
    CopyMem (&Data1, (UINT16*)(&LineNumber), sizeof(UINT16)); // Use the first two bytes of Data1 for the line number
    if (FileNameLength <= 6) { // We can fit everything into Data1
      CopyMem ((UINT8*)&Data1 + 2, FileName, FileNameLength);
    } else if (FileNameLength > 6 && FileNameLength <= 14) { // Use all of Data1 and some of Data2
      CopyMem ((UINT8*)&Data1 + 2, FileName, 6);
      CopyMem (&Data2, FileName + 6, FileNameLength - 6);
    } else { // Take the last 14 characters of the file name
      CopyMem ((UINT8*)&Data1 + 2, FileName + (FileNameLength - 14), 6);
      CopyMem (&Data2, FileName + (FileNameLength - 8), 8);
    }
    LogTelemetry (TRUE,
      NULL,
      (EFI_SOFTWARE_UNSPECIFIED | EFI_SW_EC_RELEASE_ASSERT),
      NULL,
      NULL,
      Data1,
      Data2
    );
  }
  // END LOGTELEMETRY
  if ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED) != 0) {
    CpuBreakpoint ();
  }
  if ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED) != 0) {
    CpuDeadLoop ();
  }
}


/**
  Returns TRUE if ASSERT() macros are enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of
  PcdDebugPropertyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugPropertyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugPropertyMask is clear.

**/
BOOLEAN
EFIAPI
DebugAssertEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED) != 0);
}
