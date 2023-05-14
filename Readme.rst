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

Branch Status - release/202302
==============================

:Status:
  In Development

:Entered Development:
  May 2023

:Anticipated Stabilization:
  Aug 2023

Branch Changes - release/202302
===============================

Breaking Changes-dev
--------------------

- None

Main Changes-dev
----------------

- Refactored memory test app
- Added AARCH64 RWX Test to the DxePagingAuditApp
- Added MemoryAttributeProtocolFuncTestApp
- Pkg refactors to remove unnecessary libraries
- Added Image Protection Test
- Updated Stack Cookie implementation

Bug Fixes-dev
-------------

- Fixed memory map overlap check in MemmapAndMatTestApp

Branched from 202208
--------------------

Original sync Commit: b9335ebbc2bb143ffe5f6f941a806482aedd0a98


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

.. _Windows_VS2022: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=49&&branchName=release%2F202302
.. |WindowsCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Plus%20CI%20VS2022?branchName=release%2F202302
.. |WindowsCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/49.svg
.. |WindowsCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. _Ubuntu_GCC5: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=50&&branchName=release%2F202302
.. |UbuntuCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Plus%20CI%20Ubuntu%20GCC5?branchName=release%2F202302
.. |UbuntuCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/50.svg
.. |UbuntuCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue
