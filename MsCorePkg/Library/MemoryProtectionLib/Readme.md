# MemoryProtectionLib

## About

MemoryProtectionLib enables the altering of setting(s) for memory
protections without the need to rebuild firmware.
These protection settings have usually been completely controlled by fixed at build PCDs in
[MdeModulePackage](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/MdeModulePkg.dec).

![MemoryProtectionLib Flow](mem_prot_flow_mu.jpg)

## Implementation

The library contains a set of logic common across phases and
phase-specific modules which implement functionality based on
respective phase restrictions.

**PeiMemoryProtectionLib** is responsible for creating the HOB entry used by all subsequent checks to
IsMemoryProtectionGlobalToggleEnabled(). The HOB entry can be populated two ways:

1. The first call to IsMemoryProtectionGlobalToggleEnabled() will create the entry if it has not already been created.
See the flow chart to understand the logic behind how the HOB entry is populated in this case.

2. A call to SetMemoryProtectionGlobalToggle() is allowed in this phase **IF**
IsMemoryProtectionGlobalToggleEnabled() has not yet been called elsewhere in PEI and
SetMemoryProtectionGlobalToggle() memory hasn't yet been discovered.
Doing this will allow you to force the toggle to be on/off.

**DxeSmmMemoryProtectionLib** is capable of checking the settings held in the MEM_PROT_SETTINGS struct in the
HOB entry. For faster querying and to fix a potential complication where SmmReadyToLock blocks access to the HOB,
the value of the struct held in the HOB is cached.
The call to set the memory protection global toggle is not allowed in this phase.

## MemoryProtectionExceptionLib

MemoryProtectionExceptionHandlerLib is a companion to this library and
primarily responsible for logic to turn off the memory protection
global toggle if a page fault has occurred. This functionality is reliant on some form of early store which
can be written to in an exception context.

## MemoryProtectionExceptionHandlerLib

Another companion to MemoryProtectionLib. MemoryProtectionLib uses the
MemoryProtectionExceptionLib API call
MemoryProtectionExceptionOverrideCheck() to see if a setting override was created on a previous
boot due to a memory
protection violation. This override always takes priority when creating the HOB unless the platform
explicitly set the
global toggle during PEI (see PeiMemoryProtectionLib under [Implementation](#Implementation)).
MemoryProtectionExceptionOverrideClear() clears the override store.

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent
