=================================
Project Mu Common Plus Repository
=================================

============================= ================= =============== ===================
 Host Type & Toolchain        Build Status      Test Status     Code Coverage
============================= ================= =============== ===================
Windows_VS2022_               |WindowsCiBuild|  |WindowsCiTest| |WindowsCiCoverage|
Ubuntu_GCC5_                  |UbuntuCiBuild|   |UbuntuCiTest|  |UbuntuCiCoverage|
============================= ================= =============== ===================

This repository is part of Project Mu.  Please see Project Mu for details https://microsoft.github.io/mu.

Branch Status - release/202311
==============================

:Status:
  In Development

:Entered Development:
  Dec 2023

:Anticipated Stabilization:
  Feb 2024

Branch Changes - release/202311
===============================

Breaking Changes-dev
--------------------

- Incomplete

Main Changes-dev
----------------

- Added HiiKeyboardLayout crate to support UEFI keyboard layouts
- Added Hid KeyboardSupport for UefiHidDxe
- Rework of TpmReplay
- Separated FrameBufferMemDrawLib into DXE and PEI instances
- Added SecureBootKeyStoreLib library implementation
- Split memory protection test app into DXE and SMM versions

Bug Fixes-dev
-------------

- Fixed logic related to the DXE_CORE advanced logger
- Fixed AdvLogger rust deadlock

Branched from 202308
--------------------

Original sync Commit: 61103c3af347f488431507491350b681dd1b462f


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

| Copyright (C) Microsoft Corporation
| SPDX-License-Identifier: BSD-2-Clause-Patent

.. ===================================================================
.. This is a bunch of directives to make the README file more readable
.. ===================================================================

.. CoreCI

.. _Windows_VS2022: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=49&&branchName=release%2F202311
.. |WindowsCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Plus%20CI%20VS2022?branchName=release%2F202311
.. |WindowsCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/49.svg
.. |WindowsCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. _Ubuntu_GCC5: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=50&&branchName=release%2F202311
.. |UbuntuCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Plus%20CI%20Ubuntu%20GCC5?branchName=release%2F202311
.. |UbuntuCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/50.svg
.. |UbuntuCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue
