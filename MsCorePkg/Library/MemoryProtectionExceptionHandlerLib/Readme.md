# MemoryProtectionExceptionHandlerLib

## About

If memory protections are on, MemoryProtectionExceptionHandlerLib registers an exception handler which
uses platform early store to log the page fault. It also registers the exception handler which will handle
stack cookie check failures.

## Usage

To use this library, make MemoryProtectionExceptionHandlerLib a null library for a
module such as CpuDxe:

```C
UefiCpuPkg\CpuDxe\CpuDxe.inf {
<LibraryClasses>
NULL|MsCorePkg/Library/MemoryProtectionExceptionHandlerLib/MemoryProtectionExceptionHandlerLibX64.inf
}
```

or

```C
ArmPkg\Drivers\CpuDxe\CpuDxe.inf {
<LibraryClasses>
NULL|MsCorePkg/Library/MemoryProtectionExceptionHandlerLib/MemoryProtectionExceptionHandlerLibAArch64.inf
}
```

CpuDxe is preferable because the exception handlers can only be registered after
gEfiCpuArchProtocolGuid has been installed, and attaching to the CPU driver allows
the exception handlers to be registered as early as possible.
