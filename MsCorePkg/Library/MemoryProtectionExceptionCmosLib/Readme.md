# MemoryProtectionLib

## About

MemoryProtectionExceptionHandlerLib registers a page fault exception handler which toggles off the global
memory protection toggle handled by MemoryProtectionLib. If the toggle is already off, this library does nothing.

MemoryProtectionExceptionLib implements calls to MemoryProtectionExceptionOverrideCheck() and
ClearMemoryProtectionExceptionOverride(). MemoryProtectionExceptionOverrideCheck() returns the value held in early
store (if it exists due to an exception) and ClearMemoryProtectionExceptionOverride() clears the early
store.

See MemoryProtectionLib for more information on how this library is used in conjunction with the global toggle logic.

## Usage

To use this library, make MemoryProtectionExceptionHandlerLib a null library for a dxe core library such as DxeMain:

```C
MdeModulePkg/Core/Dxe/DxeMain.inf {
<LibraryClasses>
NULL|MsCorePkg/Library/MemoryProtectionExceptionCmosLibMemoryProtectionExceptionHandlerLib.inf
}
```

and MemoryProtectionExceptionLib under [LibraryClasses].

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent
