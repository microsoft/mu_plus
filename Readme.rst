=================================
Project Mu Common Plus Repository
=================================

============================= ================= =============== ===================
 Host Type & Toolchain        Build Status      Test Status     Code Coverage
============================= ================= =============== ===================
Windows_VS2019_               |WindowsCiBuild|  |WindowsCiTest| |WindowsCiCoverage|
Ubuntu_GCC5_                  |UbuntuCiBuild|   |UbuntuCiTest|  |UbuntuCiCoverage|
============================= ================= =============== ===================

This repository is part of Project Mu.  Please see Project Mu for details https://microsoft.github.io/mu

Branch Status - release/202102
==============================

:Status:
  In Development

:Entered Development:
  2021/03/22

:Anticipated Stabilization:
  May 2021

Branch Changes - release/202102
===============================

Breaking Changes-dev
--------------------

- None

Main Changes-dev
----------------

- MuVarPolicyFoundationDxe is no longer required to get backwards compatibility with EDKII_VARIABLE_LOCK_PROTOCOL
  - It's now provided by Basecore.
- UefiTestingPkg/FunctionalSystemTests/VarPolicyUnitTestApp dropped. It's now in Basecore

Bug Fixes-dev
-------------

- Drop mPeiVariableNotifyList from MfciPei. Seems to be unused.

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

.. _Windows_VS2019: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=49&&branchName=release%2F202102
.. |WindowsCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Plus%20CI%20VS2019?branchName=release%2F202102
.. |WindowsCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/49.svg
.. |WindowsCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. _Ubuntu_GCC5: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=50&&branchName=release%2F202102
.. |UbuntuCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Plus%20CI%20Ubuntu%20GCC5?branchName=release%2F202102
.. |UbuntuCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/50.svg
.. |UbuntuCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue
