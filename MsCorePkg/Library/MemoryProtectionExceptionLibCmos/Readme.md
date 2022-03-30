# MemoryProtectionExceptionLib

## About

MemoryProtectionExceptionLib implements functionality for getting, setting, and clearing memory protection
override values from the platform-specific early store. All functions return an EFI_ERROR if the function was
unsuccessful. The validity of the data stored in CMOS is verified through the use of a two-byte checksum.
Sets/Gets/Clears will first evaluate the checksum and return an EFI_ERROR if it is invalid. Sets/Gets/Clears will
also write and read a test value to CMOS to make sure the library is working as expected which also should catch
instances where the library was linked improperly.

## Usage

MemoryProtectionExceptionLib is a BASE library and can be included under [LibraryClasses].

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent
