# SMM Paging Protections Test

## &#x1F539; Copyright
Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

## About This Test

This is a [currently] small test to prove that certain SMM paging protections have been applied.

It consists of:

- An SMM driver
- A DXE driver
- A Shell-based test app

In order to use this test, the SMM and DXE drivers must be built and included in your FW image to be dispatched at boot time. The Shell-based app may be built at any time and run from Shell. The app will ask the DXE driver to pass a message to the SMM driver to invoke a particular test.

It is not the intention of this test to include the two drivers in production systems. They should only be used for purpose-built test images.