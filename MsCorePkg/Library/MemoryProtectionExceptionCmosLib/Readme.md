# MemoryProtectionLib

## About

MemoryProtectionExceptionHandlerLib registers a page fault exception handler which toggles off the global
memory protection toggle handled by MemoryProtectionLib. If the toggle is already off, this library does nothing.

MemoryProtectionExceptionLib implements calls to MemoryProtectionExceptionOverrideCheck() and
MemoryProtectionExceptionOverrideClear(). MemoryProtectionExceptionOverrideCheck() returns the value held in early
store (if it exists due to an exception) and MemoryProtectionExceptionOverrideClear() clears the early
store. MemoryProtectionDidSystemHitException() returns TRUE if an exception was hit on a previous boot.
The validity of the data
stored in CMOS is verified through the use of a two-byte checksum. Checks will
first evaluate the checksum and return an EFI_ERROR if it is invalid.

See the MemoryProtectionLib for more information on how this library is used in
conjunction with the memory protection global toggle logic.

## Usage

To use this library, make MemoryProtectionExceptionHandlerLib a null library for a
library such as CpuDxe:

```C
UefiCpuPkg\CpuDxe\CpuDxe.inf {
<LibraryClasses>
NULL|MsCorePkg/Library/MemoryProtectionExceptionCmosLibMemoryProtectionExceptionHandlerLib.inf
}
```

CpuDxe is preferable because the page fault exception handler is only registered after
gEfiCpuArchProtocolGuid has been installed.

MemoryProtectionExceptionLib can be included under [LibraryClasses].

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent
