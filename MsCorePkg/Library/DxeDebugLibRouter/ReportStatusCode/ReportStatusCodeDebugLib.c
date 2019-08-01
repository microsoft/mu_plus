/** @file
  Debug Library based on report status code library.

  Note that if the debug message length is larger than the maximum allowable
  record length, then the debug message will be ignored directly.

Copyright (C) Microsoft Corporation. All rights reserved.

  Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ReportStatusCodeLib.h>
#include <Library/DebugLib.h>
#include <Library/DebugPrintErrorLevelLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/StatusCodeDataTypeDebug.h>

/**
  Prints a debug message to the debug output device if the specified error level is enabled.

  If any bit in ErrorLevel is also set in DebugPrintErrorLevelLib function 
  GetDebugPrintErrorLevel (), then print the message specified by Format and the 
  associated variable argument list to the debug output device.

  If Format is NULL, then ASSERT().

  If the length of the message string specificed by Format is larger than the maximum allowable
  record length, then directly return and not print it.

  @param  ErrorLevel  The error level of the debug message.
  @param  Format      Format string for the debug message to print.
  @param  Marker      Variable argument list forwarded from DebugLibRouter

**/
VOID
EFIAPI
ReportStatusCodeDebugPrint (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  IN  VA_LIST      VaListMarker
  )
{
  UINT64          Buffer[(EFI_STATUS_CODE_DATA_MAX_SIZE / sizeof (UINT64)) + 1];
  EFI_DEBUG_INFO  *DebugInfo;
  UINTN           TotalSize;
  UINTN           DestBufferSize;
  BASE_LIST       BaseListMarker;
  CHAR8           *FormatString;
  BOOLEAN         Long;

  //
  // If Format is NULL, then ASSERT().
  //
  ASSERT (Format != NULL);

  //
  // Check driver Debug Level value and global debug level
  //
  if ((ErrorLevel & GetDebugPrintErrorLevel ()) == 0) {
    return;
  }

  //
  // Compute the total size of the record.
  // Note that the passing-in format string and variable parameters will be constructed to 
  // the following layout:
  //
  //         Buffer->|------------------------|
  //                 |         Padding        | 4 bytes
  //      DebugInfo->|------------------------|
  //                 |      EFI_DEBUG_INFO    | sizeof(EFI_DEBUG_INFO)
  // BaseListMarker->|------------------------|
  //                 |           ...          |
  //                 |   variable arguments   | 12 * sizeof (UINT64)
  //                 |           ...          |
  //                 |------------------------|
  //                 |       Format String    |
  //                 |------------------------|<- (UINT8 *)Buffer + sizeof(Buffer)
  //
  TotalSize = 4 + sizeof (EFI_DEBUG_INFO) + 12 * sizeof (UINT64) + AsciiStrSize (Format);

  //
  // If the TotalSize is larger than the maximum record size, then return
  //
  if (TotalSize > sizeof (Buffer)) {
    return;
  }

  //
  // Fill in EFI_DEBUG_INFO
  //
  // Here we skip the first 4 bytes of Buffer, because we must ensure BaseListMarker is
  // 64-bit aligned, otherwise retrieving 64-bit parameter from BaseListMarker will cause
  // exception on IPF. Buffer starts at 64-bit aligned address, so skipping 4 types (sizeof(EFI_DEBUG_INFO))
  // just makes address of BaseListMarker, which follows DebugInfo, 64-bit aligned.
  //
  DebugInfo             = (EFI_DEBUG_INFO *)(Buffer) + 1;
  DebugInfo->ErrorLevel = (UINT32)ErrorLevel;
  BaseListMarker        = (BASE_LIST)(DebugInfo + 1);
  FormatString          = (CHAR8 *)((UINT64 *)(DebugInfo + 1) + 12);

  //
  // Copy the Format string into the record
  //
  // According to the content structure of Buffer shown above, the size of
  // the FormatString buffer is the size of Buffer minus the Padding
  // (4 bytes), minus the size of EFI_DEBUG_INFO, minus the size of
  // variable arguments (12 * sizeof (UINT64)).
  //
  DestBufferSize = sizeof (Buffer) - 4 - sizeof (EFI_DEBUG_INFO) - 12 * sizeof (UINT64);
  AsciiStrCpyS (FormatString, DestBufferSize / sizeof (CHAR8), Format);

  //
  // The first 12 * sizeof (UINT64) bytes following EFI_DEBUG_INFO are for variable arguments
  // of format in DEBUG string, which is followed by the DEBUG format string.
  // Here we will process the variable arguments and pack them in this area.
  //
  for (; *Format != '\0'; Format++) {
    //
    // Only format with prefix % is processed.
    //
    if (*Format != '%') {
      continue;
    }
    Long = FALSE;
    //
    // Parse Flags and Width
    //
    for (Format++; TRUE; Format++) {
      if (*Format == '.' || *Format == '-' || *Format == '+' || *Format == ' ') {
        //
        // These characters in format field are omitted.
        //
        continue;
      }
      if (*Format >= '0' && *Format <= '9') {
        //
        // These characters in format field are omitted.
        //
        continue;
      }
      if (*Format == 'L' || *Format == 'l') {
        //
        // 'L" or "l" in format field means the number being printed is a UINT64
        //
        Long = TRUE;
        continue;
      }
      if (*Format == '*') {
        //
        // '*' in format field means the precision of the field is specified by
        // a UINTN argument in the argument list.
        //
        BASE_ARG (BaseListMarker, UINTN) = VA_ARG (VaListMarker, UINTN);
        continue;
      }
      if (*Format == '\0') {
        //
        // Make no output if Format string terminates unexpectedly when
        // looking up for flag, width, precision and type. 
        //
        Format--;
      }
      //
      // When valid argument type detected or format string terminates unexpectedly,
      // the inner loop is done.
      //
      break;
    }
    
    //
    // Pack variable arguments into the storage area following EFI_DEBUG_INFO.
    //
    if ((*Format == 'p') && (sizeof (VOID *) > 4)) {
      Long = TRUE;
    }
    if (*Format == 'p' || *Format == 'X' || *Format == 'x' || *Format == 'd' || *Format == 'u') {
      if (Long) {
        BASE_ARG (BaseListMarker, INT64) = VA_ARG (VaListMarker, INT64);
      } else {
        BASE_ARG (BaseListMarker, int) = VA_ARG (VaListMarker, int);
      }
    } else if (*Format == 's' || *Format == 'S' || *Format == 'a' || *Format == 'g' || *Format == 't') {
      BASE_ARG (BaseListMarker, VOID *) = VA_ARG (VaListMarker, VOID *);
    } else if (*Format == 'c') {
      BASE_ARG (BaseListMarker, UINTN) = VA_ARG (VaListMarker, UINTN);
    } else if (*Format == 'r') {
      BASE_ARG (BaseListMarker, RETURN_STATUS) = VA_ARG (VaListMarker, RETURN_STATUS);
    }

    //
    // If the converted BASE_LIST is larger than the 12 * sizeof (UINT64) allocated bytes, then ASSERT()
    // This indicates that the DEBUG() macro is passing in more argument than can be handled by 
    // the EFI_DEBUG_INFO record
    //
    ASSERT ((CHAR8 *)BaseListMarker <= FormatString);

    //
    // If the converted BASE_LIST is larger than the 12 * sizeof (UINT64) allocated bytes, then return
    //
    if ((CHAR8 *)BaseListMarker > FormatString) {
      return;
    }
  }

  //
  // Send the DebugInfo record
  //
  REPORT_STATUS_CODE_EX (
    EFI_DEBUG_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_DC_UNSPECIFIED),
    0,
    NULL,
    &gEfiStatusCodeDataTypeDebugGuid,
    DebugInfo,
    TotalSize
    );
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

  @param  FileName     Pointer to the name of the source file that generated the assert condition.
  @param  LineNumber   The line number in the source file that generated the assert condition
  @param  Description  Pointer to the description of the assert condition.

**/
VOID
EFIAPI
ReportStatusCodeDebugAssert (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  )
{
  UINT64                 Buffer[EFI_STATUS_CODE_DATA_MAX_SIZE / sizeof(UINT64)];
  EFI_DEBUG_ASSERT_DATA  *AssertData;
  UINTN                  HeaderSize;
  UINTN                  TotalSize;
  CHAR8                  *Temp;
  UINTN                  ModuleNameSize;
  UINTN                  FileNameSize;
  UINTN                  DescriptionSize;

  //
  // Get string size
  //
  HeaderSize       = sizeof (EFI_DEBUG_ASSERT_DATA);
  //
  // Compute string size of module name enclosed by []
  //
  ModuleNameSize   = 2 + AsciiStrSize (gEfiCallerBaseName);
  FileNameSize     = AsciiStrSize (FileName);
  DescriptionSize  = AsciiStrSize (Description);

  //
  // Make sure it will all fit in the passed in buffer.
  //
  if (HeaderSize + ModuleNameSize + FileNameSize + DescriptionSize > sizeof (Buffer)) {
    //
    // remove module name if it's too long to be filled into buffer
    //
    ModuleNameSize = 0;
    if (HeaderSize + FileNameSize + DescriptionSize > sizeof (Buffer)) {
      //
      // FileName + Description is too long to be filled into buffer.
      //
      if (HeaderSize + FileNameSize < sizeof (Buffer)) {
        //
        // Description has enough buffer to be truncated.
        //
        DescriptionSize = sizeof (Buffer) - HeaderSize - FileNameSize;
      } else {
        //
        // FileName is too long to be filled into buffer.
        // FileName will be truncated. Reserved one byte for Description NULL terminator.
        //
        DescriptionSize = 1;
        FileNameSize    = sizeof (Buffer) - HeaderSize - DescriptionSize;
      }
    }
  }
  //
  // Fill in EFI_DEBUG_ASSERT_DATA
  //
  AssertData = (EFI_DEBUG_ASSERT_DATA *)Buffer;
  AssertData->LineNumber = (UINT32)LineNumber;
  TotalSize  = sizeof (EFI_DEBUG_ASSERT_DATA);

  Temp = (CHAR8 *)(AssertData + 1);

  //
  // Copy Ascii [ModuleName].
  //
  if (ModuleNameSize != 0) {
    CopyMem(Temp, "[", 1);
    CopyMem(Temp + 1, gEfiCallerBaseName, ModuleNameSize - 3);
    CopyMem(Temp + ModuleNameSize - 2, "] ", 2);
  }

  //
  // Copy Ascii FileName including NULL terminator.
  //
  Temp = CopyMem (Temp + ModuleNameSize, FileName, FileNameSize);
  Temp[FileNameSize - 1] = 0;
  TotalSize += (ModuleNameSize + FileNameSize);

  //
  // Copy Ascii Description include NULL terminator.
  //
  Temp = CopyMem (Temp + FileNameSize, Description, DescriptionSize);
  Temp[DescriptionSize - 1] = 0;
  TotalSize += DescriptionSize;

  REPORT_STATUS_CODE_EX (
    (EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED),
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_EC_ILLEGAL_SOFTWARE_STATE),
    0,
    NULL,
    NULL,
    AssertData,
    TotalSize
    );

  //
  // Generate an Assertion Break, Breakpoint, DeadLoop, or NOP based on PCD settings
  //
  if ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_ASSERT_BREAKASSERT_ENABLED) != 0) {
    CpuBreakAssert ();
  }
  
  if ((PcdGet8 (PcdDebugPropertyMask) & DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED) != 0) {
    CpuBreakpoint ();
  }
  
  if ((PcdGet8 (PcdDebugPropertyMask) & DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED) != 0) {
    CpuDeadLoop ();
  }
}