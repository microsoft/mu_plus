# Incompatible Pci Devices - No Option Roms Allowed

## About

Some system plaforms do not want PCI devices to run their option ROMS.  In this case, the
platform provides the PCI Option Rom in the system firmware image, and this driver prevents
the PCI drivers from loading the option rom from the device.
