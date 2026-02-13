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

Branch Status - release/202511
==============================

:Status:
  In Development

:Entered Development:
  2026/11/22 (Date Edk2 started accepting changes which were not in a previous release)

:Anticipated Stabilization:
  May 2026

Branch Changes - release/202511
===============================

Breaking Changes-dev
--------------------

Main Changes-dev
----------------

- gEfiEventPostReadyToBootGuid was replaced with EDK2's gEfiEventAfterReadyToBootGuid
- gMemoryProtectionNonstopModeProtocolGuid has been depreciated and associated code removed. 
- The changes in basecore which allowed removing STATIC defines were dropped. Associated unit tests which relied on this capability were dropped. 

Platform Integration Reference
------------------------------
Reference platforms which consume release/202511 are available in [mu_tiano_platforms](https://github.com/microsoft/mu_tiano_platforms).

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

| Copyright (c) Microsoft Corporation
| SPDX-License-Identifier: BSD-2-Clause-Patent

.. ===================================================================
.. This is a bunch of directives to make the README file more readable
.. ===================================================================

.. CoreCI

.. _Windows_VS2022: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=49&&branchName=release%2F202511
.. |WindowsCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Plus%20CI%20VS2022?branchName=release%2F202511
.. |WindowsCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/49.svg
.. |WindowsCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. _Ubuntu_GCC5: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=50&&branchName=release%2F202511
.. |UbuntuCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Plus%20CI%20Ubuntu%20GCC5?branchName=release%2F202511
.. |UbuntuCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/50.svg
.. |UbuntuCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue
