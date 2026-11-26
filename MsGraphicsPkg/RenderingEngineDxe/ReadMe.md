# Rendering Engine Driver

## About

The Rendering Engine is responsible for maintaining the supported windows, displaying popups,
and restopring windows when popups are dismissed.
The Rendering Engine opens the Graphics Output Protocol produced by the display driver, publishes a proxy
Graphics Output Protocol and the Rendering Engine protocol on a child handle, and hooks the parent's Blt function
so callers that use the parent directly also go through it. Console drivers attach to the child handle.

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
