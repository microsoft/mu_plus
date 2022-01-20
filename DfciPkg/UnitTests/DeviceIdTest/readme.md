# Verify DfciDeviceIdLib library functionality

The library DfciDeviceIdLib provided Dfci with platform information that Dfci needs.  This include the manufacturer
name, product name, and serial number.  Dfci has limit on the characters supported, and the length of the strings returned.

Device Id Library rules:

1. The following five characters are not allowed: `" ' < > &`
2. The maximum string length is 64 characters plus a terminating '\0'
3. '\0' is a required terminator.  The interfaces return
      the string and the size of the string (including the '\0').
4. The string is a valid UTF-8 string (ie, no 8-bit ASCII)

## About

These tests verify that the DeviceIdLib Library functions properly.

## DeviceIdIdTestApp

This application consumes the DfciDeviceIdLib executed test cases for the verification of the Device Id Strings.

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
