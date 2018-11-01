/** @file
  Base Debug library instance base on Serial Port library.
  It uses PrintLib to send debug messages to serial port device.
  
  NOTE: If the Serial Port library enables hardware flow control, then a call 
  to DebugPrint() or DebugAssert() may hang if writes to the serial port are 
  being blocked.  This may occur if a key(s) are pressed in a terminal emulator
  used to monitor the DEBUG() and ASSERT() messages. 

  Copyright (c) 2018, Microsoft Corporation

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php.                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <Library/SerialPortLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/DebugPrintErrorLevelLib.h>
#include <Library/BaseLib.h>

//
// Define the maximum debug and assert message length that this library supports 
//
#define MAX_DEBUG_MESSAGE_LENGTH  0x100

/**
  Prints a debug message to the debug output device if the specified error level is enabled.

  If any bit in ErrorLevel is also set in DebugPrintErrorLevelLib function 
  GetDebugPrintErrorLevel (), then print the message specified by Format and the 
  associated variable argument list to the debug output device.

  If Format is NULL, then ASSERT().

  @param  ErrorLevel  The error level of the debug message.
  @param  Format      Format string for the debug message to print.
  @param  Marker      Variable argument list forwarded from DebugLibRouter

**/
VOID
EFIAPI
SerialDebugPrint (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  IN  VA_LIST      Marker
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
  AsciiVSPrint (Buffer, sizeof (Buffer), Format, Marker);
  //
  // Send the print string to a Serial Port 
  //
  SerialPortWrite ((UINT8 *)Buffer, AsciiStrLen (Buffer));
}

/**
  Prints an assert message containing a filename, line number, and description.  
  This may be followed by a breakpoint or a dead loop.

  Print a message of the form "ASSERT <FileName>(<LineNumber>): <Description>\n"
  to the debug output device.  If DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED bit of 
  PcdDebugProperyMask is set then CpuBreakpoint() is called. Otherwise, if 
  DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED bit of PcdDebugProperyMask is set then 
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
SerialDebugAssert (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  )
{
  CHAR8  Buffer[MAX_DEBUG_MESSAGE_LENGTH];

  //
  // Generate the ASSERT() message in Ascii format
  //
  AsciiSPrint (Buffer, sizeof (Buffer), "ASSERT [%a] %a(%d): %a\n", gEfiCallerBaseName, FileName, LineNumber, Description);

  //
  // Send the print string to the Console Output device
  //
  SerialPortWrite ((UINT8 *)Buffer, AsciiStrLen (Buffer));

  //
  // Generate an Assertion Break, Breakpoint, DeadLoop, or NOP based on PCD settings
  //
  if ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_ASSERT_BREAKASSERT_ENABLED) != 0) {
    CpuBreakAssert ();
  }

  if ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED) != 0) {
    CpuBreakpoint ();
  }

  if ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED) != 0) {
    CpuDeadLoop ();
  }
}