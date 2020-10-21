# Incompatible Pci Devices

## About

There are times when a PCI devices misbehaves.  This library class allows you to specify what to
do when you need to support a PCI device that doesn't follow the spec, or if the platform wants
to disable a feature.

The only Incompatible Pci device behavior currently supported is to disable processing of the
Option Rom.
It is expected for supported PCI devices to have their rom built into the system firmware.

---

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent
