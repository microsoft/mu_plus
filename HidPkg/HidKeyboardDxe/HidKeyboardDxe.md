
# Description

This driver produces an instances of SIMPLE_TEXT_INPUT/SIMPLE_TEXT_INPUT_EX for keyboard support in UEFI

It registers a callback with devices exposing the HID_KEYBOARD_PROTOCOL to receive Keyboard HID reports,
which are used to satisfy the contract of SIMPLE_TEXT_INPUT/SIMPLE_TEXT_INPUT_EX.

# Provides

SIMPLE_TEXT_INPUT/SIMPLE_TEXT_INPUT_EX instance for consumption by UEFI console.

# Dependencies

HID_KEYBOARD_PROTOCOL to register for and receive HID Keyboard Reports.

# Application

Used to enable PreBoot Keyboard Support.
