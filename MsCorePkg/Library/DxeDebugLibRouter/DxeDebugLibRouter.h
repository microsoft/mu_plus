/** @file
Provides the function pointers used to route Debug messages
to the correct DebugLib

Copyright (c) 2018, Microsoft Corporation

Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

/**
    DebugPrint Protocol Shim used to route
    function calls to the correct library
**/
typedef VOID (EFIAPI DEBUG_PRINT)(
    IN  UINTN        ErrorLevel,
    IN  CONST CHAR8  *Format,
    IN  VA_LIST      VaListMarker
    );

/**
    DebugAssert Protocol Shim used to route
    function calls to the correct library
**/
typedef VOID (EFIAPI DEBUG_ASSERT)(
    IN CONST CHAR8  *FileName,
    IN UINTN        LineNumber,
    IN CONST CHAR8  *Description
    );


/**
    DebugPrint functions implemented
**/
VOID
EFIAPI
ReportStatusCodeDebugPrint (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  IN  VA_LIST      VaListMarker
  );

VOID
EFIAPI
SerialDebugPrint (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  IN  VA_LIST      Marker
  );

/**
    DebugAssert functions implemented
**/
VOID
EFIAPI
ReportStatusCodeDebugAssert (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  );

VOID
EFIAPI
SerialDebugAssert (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  );