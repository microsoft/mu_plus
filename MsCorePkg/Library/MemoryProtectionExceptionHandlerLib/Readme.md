# MemoryProtectionExceptionHandlerLib

## About

MemoryProtectionExceptionHandlerLib registers a page fault exception handler which toggles off the global
memory protection toggle handled by MemoryProtectionLib. If the toggle is already off, this library does nothing.

## Usage

To use this library, make MemoryProtectionExceptionHandlerLib a null library for a
module such as CpuDxe:

```C
UefiCpuPkg\CpuDxe\CpuDxe.inf {
<LibraryClasses>
NULL|MsCorePkg/Library/MemoryProtectionExceptionHandlerLib/MemoryProtectionExceptionHandlerLib.inf
}
```

CpuDxe is preferable because the page fault exception handler is only registered after
gEfiCpuArchProtocolGuid has been installed.
