# MemoryProtectionLib

## About

MemoryProtection lib supplies libraries for the PEI and DXE phases to get and set the setting(s) for memory
protections. The settings have previously been completely controlled by regular and fixed at build PCDs in
[MdeModulePackage](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/MdeModulePkg.dec). This library
provides functionality for dynamically toggling these settings.

![MemoryProtectionLib Flow](mem_prot_flow_mu.jpg)

## Implementation

The library is split into two main components: MemoryProtectionCommon provides common functionality and
the phase-specific modules implement functionality based on respective phase restrictions.

**PeiMemoryProtectionLib** is responsible for creating the HOB entry used by all subsequent checks to
IsMemoryProtectionGlobalToggleEnabled() (and, more generally, any theoretical call checking the values
held in the MEM_PROT_SETTINGS struct). See the flow chart to understand the logic behind how the HOB entry is
populated. The call to set the memory protection global toggle is not allowed in this phase.

**DxeSmmMemoryProtectionLib** is capable of checking the settings held in the MEM_PROT_SETTINGS struct in the
HOB entry. For faster querying and to fix a potential complication where SmmReadyToLock blocks access to the HOB,
the value of the struct held in the HOB is cached in the constructor and potentially in the first check to get a
memory protection setting. The call to set the memory protection global toggle is not allowed in this phase.

**UefiMemoryProtectionLib** is primarily for the frontpage interface. It is the only phase capable of manually
changing the memory protection global toggle. The **MemoryProtectionExceptionLib** will also change the toggle in
the case of a page fault.

## MemoryProtectionExceptionLib and MemoryProtectionExceptionHandlerLib

MemoryProtectionExceptionHandlerLib is primarily responsible for functionality to turn off the memory protection
global toggle if a page fault has occurred. This functionality is reliant on some form of early store which
can be written to at
the highest priority level. This (MemoryProtectionLib) library uses the MemoryProtectionExceptionLib API call
MemoryProtectionExceptionOverrideCheck() to see if an exception has occurred and ensure the HOB entry is created
according to the value returned instead of the UEFI variable or default PCD values.
ClearMemoryProtectionExceptionOverride() clears the override and is used when the value of the variable is manually
updated in UefiMemoryProtectionLib to ensure the manually set value is used next boot.

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent
