# Simple Window Manager

## About

The Simple Window Manager manages window placement and pop up dialogs.

The Simple Window Manager uses the Rendering Engine to display objects in their display region.

The window manager service (`gMsSWMProtocol` and the aggregate Absolute Pointer input protocol used
for rendering surfaces) is published on the Rendering Engine's graphics controller handle for the
lifetime of that graphics binding. It is removed when graphics is disconnected. This lets dependents
(e.g., OSK) open the protocol `BY_DRIVER` and be stopped deterministically before it disappears,
instead of relying on the order the core happens to visit agents during a reconnect. The base
Absolute Pointer input aggregator used by other drivers (e.g., BDS) is independent of graphics and
stays installed for the life of this driver.

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
