# ACPI RGRT

## About

The Regulatory Graphics Resource Table is an entry into the ACPI table. It provides a way to
publish a PNG with the regulatory information of a given product.
This could include FCC id, UL, Model number, or CMIIT ID just to name a few.
This driver publishes that entry in the ACPI table to be picked up by the OS later to display
for regulatory reasons.

## The table definition

The format of the table is defined by a pending addition to the ACPI spec here:

![ACPI RGRT Spec](spec_mu.png "ACPI RGRT Specification")

and consists of a standard ACPI header
along with an RGRT payload.
At time of writing, the only image format supported by RGRT is PNG.

## Using in your project

Using this is as simple as including the INF in both your DSC and FDF.
You will also need to define the PCD you use in the FDF file in your DEC file.

DSC:

```inf
[Components]
  MsCorePkg/AcpiRGRT/AcpiRgrt.inf
```

FDF:

```inf
[FV.FVDXE]

# Regulatory Graphic Driver
INF MsCorePkg/AcpiRGRT/AcpiRgrt.inf

FILE FREEFORM = PCD(gMsCorePkgTokenSpaceGuid.PcdRegulatoryGraphicFileGuid) {
    SECTION RAW = $(GRAPHICS_RESOURCES)/RGRT.png
}
```

---

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent
