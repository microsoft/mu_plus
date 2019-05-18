=================================
Project Mu Common Plus Repository
=================================

.. |build_status_windows| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/mu_plus%20PR%20gate?branchName=release/201903

|build_status_windows| Current build status for release/201903


This repository is part of Project Mu.  Please see Project Mu for details https://microsoft.github.io/mu

Branch Changes - release/201903
===============================

Breaking Changes-dev
--------------------

- None

Main Changes-dev
----------------

- Added Color Bars for DEVICE_STATE_PLATFORM_MODE_[2|3]. (indigo and brown, respectively). Removed display for DEVICE_STATE_UNDEFINED.
- Added SharedCryptoPkg that includes support for having BaseCryptLib over a protocol
- Added the MuVarPolicyFoundationDxe driver to work with the new Variable Policy infrastructure to install a couple of foundational policies that can be used by multiple drivers. See the Feature_MuVarPolicyFoundationDxe_Readme.md document for details.

Bug Fixes-dev
-------------

- None

1903_CIBuild Changes
--------------------

Source Commit from 201811: 235936c9ab85fe859ddbdb8576b7d4e5cd711052

- Update DisplayEngineDxe fingerprint. Should this even still be an override?

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

Copyright (c) 2018, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
