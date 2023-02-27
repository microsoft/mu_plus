# MemoryProtectionExceptionHandlerLib

## About

If memory protections are on, MemoryProtectionExceptionHandlerLib registers an exception handler which
uses platform early store to log the page fault.

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

BEEBE TODO: Rewrite this doc to fit the changes