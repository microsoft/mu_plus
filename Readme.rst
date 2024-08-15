=================================
Project Mu Common Plus Repository
=================================

============================= ================= =============== ===================
 Host Type & Toolchain        Build Status      Test Status     Code Coverage
============================= ================= =============== ===================
Windows_VS2022_               |WindowsCiBuild|  |WindowsCiTest| |WindowsCiCoverage|
Ubuntu_GCC5_                  |UbuntuCiBuild|   |UbuntuCiTest|  |UbuntuCiCoverage|
============================= ================= =============== ===================

This repository is part of Project Mu.  Please see Project Mu for details https://microsoft.github.io/mu

Branch Status - release/202405
==============================

:Status:
  In Development

:Entered Development:
  2023/11/24

:Anticipated Stabilization:
  Nov 2024

Branch Changes - release/202405
===============================

202405 is a larger deviation than previous releases. As part of upstreaming changes to EDK2, the commits were reviewed, squashed, and some were dropped.
Due to these changes, there maybe more work minor work required to bring an existing platforms up to 202405 compatibility. 

Breaking Changes-dev
--------------------
- Nothing

Main Changes-dev
----------------
- AdvancedFileLogger triggers on gEfiEventBeforeExitBootServicesGuid due to removal of gMuEventPreExitBootServicesGuid.
- OnScreenKeyboard triggers on gEfiEventBeforeExitBootServicesGuid due to removal of gMuEventPreExitBootServicesGuid.
- RenderingEngine triggers on gEfiEventBeforeExitBootServicesGuid due to removal of gMuEventPreExitBootServicesGuid.
- MsCorePkg/Library/MuArmGicExLib/MuArmGicExLib.inf exists to contain the functionality previously in ArmPkg/Drivers/ArmGic/ArmGicLib.inf.

Platform Integration Reference
----------------
Reference platforms which consume release/202405 are available in [mu_tiano_platforms](https://github.com/microsoft/mu_tiano_platforms).

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

.. _Windows_VS2022: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=49&&branchName=release%2F202405
.. |WindowsCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Plus%20CI%20VS2022?branchName=release%2F202405
.. |WindowsCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/49.svg
.. |WindowsCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. _Ubuntu_GCC5: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=50&&branchName=release%2F202405
.. |UbuntuCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Plus%20CI%20Ubuntu%20GCC5?branchName=release%2F202405
.. |UbuntuCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/50.svg
.. |UbuntuCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue
