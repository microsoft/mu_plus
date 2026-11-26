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
- The driver must load before network consumers initialize SNP. It wraps each installed SNP's `Initialize` method
  and programs the emulated address synchronously after successful initialization, holding TPL_CALLBACK across both
  operations. `PlatformMacEmulationSnpCheck` therefore runs at TPL_CALLBACK and must not lower the TPL.
- MAC programming failures are logged without changing the original initialization result. Such failures can still
  allow transmission with the original MAC address.
- This prototype remains resident and retains wrapper contexts until boot services end.
