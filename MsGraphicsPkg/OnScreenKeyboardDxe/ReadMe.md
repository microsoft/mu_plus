# OnScreenKeyboardDxe

## About

This driver implements an On-Screen Keyboard (OSK) driver.

This driver relies heavily on the SimpleWindowManager, The SimpleUI Toolkit, and the
RenderingEngine to implement a floating on screen keyboard that can be rotated on the display.

OSK participates in the UEFI driver model: it binds `BY_DRIVER` to the Simple Window Manager
protocol and publishes its ConIn device (Simple Text In/Simple Text In Ex) on a child handle
registered `BY_CHILD_CONTROLLER`. This ensures OSK is stopped before the Simple Window Manager
protocol disappears on a graphics disconnect, and lets OSK restart cleanly (a fresh client
registration and ConIn device) on reconnect. The `gMsOSKProtocol` protocol is installed
separately from the driver binding and persists across binds so other drivers can configure OSK's
presentation state before a display is available.

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
