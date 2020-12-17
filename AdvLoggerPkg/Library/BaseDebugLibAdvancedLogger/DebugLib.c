/** @file
  Base Debug library instance for Advanced Logger.
  It uses PrintLib to format debug messages to log in the in memory buffer.

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/AdvancedLoggerLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugPrintErrorLevelLib.h>
#include <Library/MuTelemetryHelperLib.h>

/**
  Prints a debug message to the debug output device if the specified
  error level is enabled.

  If any bit in ErrorLevel is also set in DebugPrintErrorLevelLib function
  GetDebugPrintErrorLevel (), then print the message specified by Format and
  the associated variable argument list to the debug output device.

  If Format is NULL, then ASSERT().

  @param  ErrorLevel    The error level of the debug message.
  @param  Format        Format string for the debug message to print.
  @param  VaListMarker  VA_LIST marker for the variable argument list.

**/
VOID
EFIAPI
DebugVPrint (
  IN  UINTN         ErrorLevel,
  IN  CONST CHAR8   *Format,
  IN  VA_LIST       VaListMarker
  );


//
// Define the maximum debug and assert message length that this library supports
//
#define MAX_DEBUG_MESSAGE_LENGTH  0x100

//
// VA_LIST can not initialize to NULL for all compiler, so we use this to
// indicate a null VA_LIST
//
VA_LIST     mVaListNull;

/**
MS_CHANGE_?
MS_CHANGE - To split the DebugPrint into two one taking va_list and one with var args
Prints a debug message to the debug output device if the specified error level is enabled.

If any bit in ErrorLevel is also set in DebugPrintErrorLevelLib function
GetDebugPrintErrorLevel (), then print the message specified by Format and the
associated variable argument list to the debug output device.

If Format is NULL, then ASSERT().

If the length of the message string specificed by Format is larger than the maximum allowable
record length, then directly return and not print it.

@param  ErrorLevel    The error level of the debug message.
@param  Format        Format string for the debug message to print.
@param  VaListMarker  VA_LIST marker for the variable argument list.

**/
VOID
EFIAPI
DebugPrintValist(
IN  UINTN        ErrorLevel,
IN  CONST CHAR8  *Format,
VA_LIST          VaListMarker
) {

    DebugVPrint (ErrorLevel, Format, VaListMarker);
}
// END

/**
  Prints a debug message to the debug output device if the specified error level is enabled.

  If any bit in ErrorLevel is also set in DebugPrintErrorLevelLib function
  GetDebugPrintErrorLevel (), then print the message specified by Format and the
  associated variable argument list to the debug output device.

  If Format is NULL, then ASSERT().

  @param  ErrorLevel  The error level of the debug message.
  @param  Format      Format string for the debug message to print.
  @param  ...         Variable argument list whose contents are accessed
                      based on the format string specified by Format.

**/
VOID
EFIAPI
DebugPrint (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  ...
  )
{
  VA_LIST  Marker;

  VA_START (Marker, Format);
  DebugVPrint (ErrorLevel, Format, Marker);
  VA_END (Marker);
}


/**
  Prints a debug message to the debug output device if the specified
  error level is enabled base on Null-terminated format string and a
  VA_LIST argument list or a BASE_LIST argument list.

  If any bit in ErrorLevel is also set in DebugPrintErrorLevelLib function
  GetDebugPrintErrorLevel (), then print the message specified by Format and
  the associated variable argument list to the debug output device.

  If Format is NULL, then ASSERT().

  @param  ErrorLevel      The error level of the debug message.
  @param  Format          Format string for the debug message to print.
  @param  VaListMarker    VA_LIST marker for the variable argument list.
  @param  BaseListMarker  BASE_LIST marker for the variable argument list.

**/
VOID
DebugPrintMarker (
  IN  UINTN         ErrorLevel,
  IN  CONST CHAR8   *Format,
  IN  VA_LIST       VaListMarker,
  IN  BASE_LIST     BaseListMarker
  )
{
  CHAR8    Buffer[MAX_DEBUG_MESSAGE_LENGTH];

  //
  // If Format is NULL, then ASSERT().
  //
  ASSERT (Format != NULL);

  //
  // Check driver debug mask value and global mask
  //
  if ((ErrorLevel & GetDebugPrintErrorLevel ()) == 0) {
    return;
  }

  //
  // Convert the DEBUG() message to an ASCII String
  //
  if (BaseListMarker == NULL) {
    AsciiVSPrint (Buffer, sizeof (Buffer), Format, VaListMarker);
  } else {
    AsciiBSPrint (Buffer, sizeof (Buffer), Format, BaseListMarker);
  }

  //
  // Send the print string to the Advanced Logger
  //
  AdvancedLoggerWrite (ErrorLevel, Buffer, AsciiStrLen (Buffer));
}


/**
  Prints a debug message to the debug output device if the specified
  error level is enabled.

  If any bit in ErrorLevel is also set in DebugPrintErrorLevelLib function
  GetDebugPrintErrorLevel (), then print the message specified by Format and
  the associated variable argument list to the debug output device.

  If Format is NULL, then ASSERT().

  @param  ErrorLevel    The error level of the debug message.
  @param  Format        Format string for the debug message to print.
  @param  VaListMarker  VA_LIST marker for the variable argument list.

**/
VOID
EFIAPI
DebugVPrint (
  IN  UINTN         ErrorLevel,
  IN  CONST CHAR8   *Format,
  IN  VA_LIST       VaListMarker
  )
{
  DebugPrintMarker (ErrorLevel, Format, VaListMarker, NULL);
}


/**
  Prints a debug message to the debug output device if the specified
  error level is enabled.
  This function use BASE_LIST which would provide a more compatible
  service than VA_LIST.

  If any bit in ErrorLevel is also set in DebugPrintErrorLevelLib function
  GetDebugPrintErrorLevel (), then print the message specified by Format and
  the associated variable argument list to the debug output device.

  If Format is NULL, then ASSERT().

  @param  ErrorLevel      The error level of the debug message.
  @param  Format          Format string for the debug message to print.
  @param  BaseListMarker  BASE_LIST marker for the variable argument list.

**/
VOID
EFIAPI
DebugBPrint (
  IN  UINTN         ErrorLevel,
  IN  CONST CHAR8   *Format,
  IN  BASE_LIST     BaseListMarker
  )
{
  DebugPrintMarker (ErrorLevel, Format, mVaListNull, BaseListMarker);
}


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
  AdvancedLoggerWrite (MAX_UINT32, Buffer, AsciiStrLen (Buffer));

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
  Fills a target buffer with PcdDebugClearMemoryValue, and returns the target buffer.

  This function fills Length bytes of Buffer with the value specified by
  PcdDebugClearMemoryValue, and returns Buffer.

  If Buffer is NULL, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param   Buffer  The pointer to the target buffer to be filled with PcdDebugClearMemoryValue.
  @param   Length  The number of bytes in Buffer to fill with zeros PcdDebugClearMemoryValue.

  @return  Buffer  The pointer to the target buffer filled with PcdDebugClearMemoryValue.

**/
VOID *
EFIAPI
DebugClearMemory (
  OUT VOID  *Buffer,
  IN UINTN  Length
  )
{
  //
  // If Buffer is NULL, then ASSERT().
  //
  ASSERT (Buffer != NULL);

  //
  // SetMem() checks for the the ASSERT() condition on Length and returns Buffer
  //
  return SetMem (Buffer, Length, PcdGet8(PcdDebugClearMemoryValue));
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


/**
  Returns TRUE if DEBUG() macros are enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of
  PcdDebugPropertyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of PcdDebugPropertyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of PcdDebugPropertyMask is clear.

**/
BOOLEAN
EFIAPI
DebugPrintEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_PRINT_ENABLED) != 0);
}


/**
  Returns TRUE if DEBUG_CODE() macros are enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of
  PcdDebugPropertyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of PcdDebugPropertyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of PcdDebugPropertyMask is clear.

**/
BOOLEAN
EFIAPI
DebugCodeEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_CODE_ENABLED) != 0);
}


/**
  Returns TRUE if DEBUG_CLEAR_MEMORY() macro is enabled.

  This function returns TRUE if the DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED bit of
  PcdDebugPropertyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED bit of PcdDebugPropertyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED bit of PcdDebugPropertyMask is clear.

**/
BOOLEAN
EFIAPI
DebugClearMemoryEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED) != 0);
}

/**
  Returns TRUE if any one of the bit is set both in ErrorLevel and PcdFixedDebugPrintErrorLevel.

  This function compares the bit mask of ErrorLevel and PcdFixedDebugPrintErrorLevel.

  @retval  TRUE    Current ErrorLevel is supported.
  @retval  FALSE   Current ErrorLevel is not supported.

**/
BOOLEAN
EFIAPI
DebugPrintLevelEnabled (
  IN  CONST UINTN        ErrorLevel
  )
{
  return (BOOLEAN) ((ErrorLevel & PcdGet32(PcdFixedDebugPrintErrorLevel)) != 0);
}

