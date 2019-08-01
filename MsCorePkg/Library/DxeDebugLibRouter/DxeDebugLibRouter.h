/** @file
Provides the function pointers used to route Debug messages
to the correct DebugLib

Copyright (C) Microsoft Corporation. All rights reserved.
Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

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