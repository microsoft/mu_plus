# ACPI RGRT

## What is it?

The Regulatory Graphics Resource Table is an entry into the ACPI table. It provides a way to publish a PNG with the regulatory information of a given product. This could include FCC id, UL, Model number, or CMIIT ID just to name a few. This driver publishes that entry in the ACPI table to be picked up by the OS later to display for regulatory reasons.

## The table definition

The format of the table is defined by spec.png and consists of a standard ACPI header along with an RGRT payload. At time of writing, the only image format supported by RGRT is PNG.

![ACPI RGRT Spec](spec_mu.png "ACPI RGRT Specification")

## Using in your project

Using this is as simple as including the INF in both your DSC and INF. You will also need to define the file for the PCD in your FDF.

DSC:
```
[Components]
  MsCorePkg/AcpiRGRT/AcpiRgrt.inf
```

FDF:
```
[FV.FVDXE]

# Regulatory Graphic Driver
INF MsCorePkg/AcpiRGRT/AcpiRgrt.inf

FILE FREEFORM = PCD(gMsCorePkgTokenSpaceGuid.PcdRegulatoryGraphicFileGuid) {
    SECTION RAW = $(GRAPHICS_RESOURCES)/RGRT.png
}
```
