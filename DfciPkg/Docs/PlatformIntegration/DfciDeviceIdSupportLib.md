# DfciDeviceIdSupportLib

DfciDeviceIdSupportLib provides DFCI with three platform strings:

1. Manufacturer name
2. Product name
3. Serial number

## Restrictions on Device Identifier strings

1. Null terminated CHAR8 strings
2. Maximum of 64 CHAR8 values plus a NULL terminator, for a maximum size of 65 bytes.
3. The following five characters are not allowed **& ' " < >**
4. UTF-8, as per [Wikipedia UTF-8](https://en.wikipedia.org/wiki/UTF-8), are allowed within the
   64 CHAR limit and the character set limitations.

## Interfaces

| Interface                      | Function |
| ---                            | --- |
| DfciIdSupportV1GetSerialNumber | DEPRECATED.  Always return 0. Will be removed. |
| DfciIdSupportGetManufacturer   | Returns an allocated buffer with the system manufacturer name. |
| DfciIdSupportGetProductName    | Return an allocated buffer with the system product name. |
| DfciIdSupportGetSerialNumber   | Return an allocated buffer with the system serial number. |

## Additional Details

These fields and their values are critical to the security of DFCI.
These values should not be user configurable and should be protected from tampering.

---

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent
