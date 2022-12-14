# MP Management Tests

## Copyright

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

## About This Test

This is set of functionality and unit tests to ensure that individual cores can be powered on/off and suspended/resumed
as expected.

It consists of:

- An DXE driver
- A Shell-based test app

The Shell-based app may be built at any time and run from Shell. The app can use the DXE driver installed protocol to
preform AP power state exercise if the DXE driver is properly installed.

It is not the intention of this test to include the driver in production systems. They should only be used for purpose-built
test images.
