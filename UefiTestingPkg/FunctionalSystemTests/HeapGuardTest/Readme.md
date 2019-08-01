# Heap Guard Tests

## &#x1F539; Copyright
Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

## About This Test

This is set of tests to ensure that heap guard, stack guard, null pointer detection, and nx protections are working properly.

It consists of:

- An SMM driver
- A Shell-based test app

The Shell-based app may be built at any time and run from Shell. The app can use the SMM driver to preform SMM tests if the SMM driver is installed.

It is not the intention of this test to include the driver in production systems. They should only be used for purpose-built test images.