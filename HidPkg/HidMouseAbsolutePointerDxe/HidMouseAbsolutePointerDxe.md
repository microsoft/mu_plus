
# Description
This driver produces an instance of EFI_ABSOLUTE_POINTER_PROTOCOL for mouse support in UEFI

It registers a callback with the devices exposing HID_POINTER_PROTOCOL to receive Mouse HID reports,
which are used to satisfy the contract of EFI_ABSOLUTE_POINTER_PROTOCOL.

# Provides
EFI_ABSOLUTE_POINTER_PROTOCOL instance for consumption by UEFI console.

# Dependencies
HID_POINTER_PROTOCOL to register for and receive HID Mouse Reports.

# Application
Used to enable PreBoot Mouse Support.