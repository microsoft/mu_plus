# MFCI Device ID Support Library: NULL interface

## About

Platforms MUST publish MFCI targeting information prior to EndOfDxe. They may do so by either setting all of the Per-Device
Targeting Variables declared in `MfciVariables.h`.  Alternatively, then can implement this library.
MfciDxe will invoke this library if it cannot find variables just before EndOfDxe.

Refer to `MfciPkg/Include/MfciVariables.h`, "Targeting Variable Names" for additional details on the strings to return.

## Copyright

Copyright (c) Microsoft Corporation
SPDX-License-Identifier: BSD-2-Clause-Patent
