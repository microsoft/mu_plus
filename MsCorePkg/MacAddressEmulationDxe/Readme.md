[//]:# Copyright (c) Microsoft Corporation.
[//]:# SPDX-License-Identifier: BSD-2-Clause-Patent


# MacAddressEmulationDxe

This driver provides UEFI support for MAC Address Emulation. There are two main
features in this driver:

1. Support for updating the station address (i.e. the MAC address) for UEFI SNP
instances that are used for PXE boot.  This ensures that the MAC address is
emulated in preboot.

2. Support for the configuration story for MAC emulation.  This includes a
settings manager that provides access to the MAC emulation setting as well as
code that verifies the proper provisioning of the MAC address to use for
emulation.


# Usage

To use this feature, platforms must do the following:

1. Provide an implementation of MacAddressEmulationPlatformLib.

2. Update the platform DSC/FDF to build and include the driver:

```dsc
MsCorePkg/MacAddressEmulationDxe/MacAddressEmulationDxe.inf {
  <LibraryClasses>
    MacAddressEmulationPlatformLib|<Path to appropriate MsMacEmulationPlatformLib>
}
```

3. Update the platform FDF to include the driver in the ROM

```fdf
INF MsCorePkg/MacAddressEmulationDxe/MacAddressEmulationDxe.inf
```

