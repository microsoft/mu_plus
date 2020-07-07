=================================
Project Mu Common Plus Repository
=================================

.. |build_status_windows| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/mu_plus%20PR%20gate?branchName=release/202002

|build_status_windows| Current build status for release/202002


This repository is part of Project Mu.  Please see Project Mu for details https://microsoft.github.io/mu

Branch Status - dev/202002
==========================

Status:
  Stabilized

Stabilized:
  2020/07/06


Branch Changes - release/202002
===============================

Breaking Changes
----------------

- SharedCrypto is still preserved for now, but doesn't build against the new core.
- Some identifiers and types were modified in the DeviceSpecificBusInfoLib interface that may break
  the build for library consumers. Users should take note that:
  1. The "MinimumGenSpeed" field in DEVICE_PCI_INFO is now called "MinimumLinkSpeed"
  2. The "MinimumLinkSpeed" field type is changed from a UINTN to an enum of type PCIE_LINK_SPEED
  3. The "PCIE_LINK_SPEED_GENx" macros are removed from the library header
  4. A new data structure is introduced of type DEVICE_PCI_CHECK_RESULT that is used in the function
     prototype for a new function ProcessPciDeviceResults (). The user should implement this function
     if their platform needs to take custom actions based on device check results.

Main Changes
------------

- None

Bug Fixes
---------

- None

Code of Conduct
===============

This project has adopted the Microsoft Open Source Code of Conduct https://opensource.microsoft.com/codeofconduct/

For more information see the Code of Conduct FAQ https://opensource.microsoft.com/codeofconduct/faq/
or contact `opencode@microsoft.com <mailto:opencode@microsoft.com>`_. with any additional questions or comments.

Contributions
=============

Contributions are always welcome and encouraged!
Please open any issues in the Project Mu GitHub tracker and read https://microsoft.github.io/mu/How/contributing/


Copyright & License
===================

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
