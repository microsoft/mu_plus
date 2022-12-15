# MacAddressEmulationDxe

This driver provides UEFI support for MAC Address Emulation.

## Feature of this driver

Support for updating the station address (i.e. the MAC address) for UEFI SNP
instances that are used for PXE boot.  This ensures that the MAC address is
emulated in preboot.

## Usage

To use this feature, platforms must do the following:

1. Provide an implementation of MacAddressEmulationPlatformLib.

2. Update the platform DSC/FDF to build and include the driver

3. Update the platform FDF to include the driver in the ROM

### Notes on platform implementation

- `GetMacEmulationAddress` and `PlatformMacEmulationEnable` are called during driver entrypoint.
- `PlatformMacEmulationSnpCheck` is called during a callback at TPL_NOTIFY, so any code here must be aware of this
   restriction. It is not recommended to lower the TPL during this function if the network stack has already been
   started as some packets may be transmitted before the mac is programmed to the emulated address.
