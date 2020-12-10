# MfciRetirevePolicyLib: NULL edition

## About

Platforms are not generally expected to implement this library, as MFCI carries known implementations,
thus it's header is under MfciPkg/Private/...  
This library abstracts where MfciDxe retrieves the cached, current policy.
On platforms with a PEI phase, it may retrieve the policy from a HOB.
On platforms that lack a PEI phase, it may retrieve the policy from a UEFI variable.

## Copyright

Copyright (c) Microsoft Corporation
SPDX-License-Identifier: BSD-2-Clause-Patent
