# MemoryProtectionExceptionLib

## About

MemoryProtectionExceptionLib implements functionality for checking, updating, and clearing memory protection
override values from the platform-specific early store. MemoryProtectionExceptionOverrideCheck() returns
the relevant value held in early store (if it exists). MemoryProtectionExceptionOverrideClear() clears the early
store. MemoryProtectionExceptionOccurred() returns TRUE if an exception was hit on a previous boot.
MemoryProtectionExceptionOverrideWrite() writes to the early store.
The validity of the data stored in CMOS is verified through the use of a two-byte checksum. Checks will
first evaluate the checksum and return an EFI_ERROR if it is invalid. The checksum is updated when
MemoryProtectionExceptionOverrideWrite() is called.

## Usage

MemoryProtectionExceptionLib is a BASE library and can be included under [LibraryClasses].

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent
