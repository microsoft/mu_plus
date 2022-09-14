# Device Firmware Configuration Interface (DFCI) Introduction

## Overview

The Device Firmware Configuration Interface (DFCI) brings new levels of security and usability to
PC configuration management.
It is a new feature of UEFI that enables secure programmatic configuration of hardware settings
that are typically configured within a BIOS menu by a human.
High value configuration can be moved to UEFI BIOS where it is resilient against malware,
rootkits, and non-persistent physical tampering.
Whereas traditional UEFI security implementations required a physical touch, DFCI securely
enables **zero-touch** remote configuration of these settings built upon[Microsoft Intune](
https://docs.microsoft.com/en-us/intune/configuration/device-firmware-configuration-interface-windows)
and authorized by [Windows Autopilot](http://aka.ms/windowsautopilot).
DFCI can provide additional assurance by configuring and locking hardware security features
before launching the OS (e.g. disabling microphones or radios).
Note that for management of servers in a datacenter, DFCI does not presume to be the solution.
[Redfish](https://www.dmtf.org/standards/redfish) may be a more suitable solution for the datacenter.

### Why Zero Touch

Traditional UEFI management solutions were either not secure, allowing malware to control them,
or not scalable, requiring a physical touch by IT or OEM for authentication.
DFCI is zero touch, leveraging the existing Windows Autopilot device registration for DFCI
authorization.

### Why should I configure my UEFI BIOS

PC configuration is typically performed via Active Directory Group Policy, System Center
Configuration Manager (SCCM), or Modern Device Management (MDM) such as [Microsoft Intune](
https://www.microsoft.com/en-us/microsoft-365/enterprise-mobility-security/microsoft-intune).
All of these solutions store their managed configuration in the OS disk partition.
Unfortunately, this configuration can be bypassed by the PCs default ability to boot other
operating system instances via external media (e.g. USB), network (e.g. PXE), & alternate disk
partitions, or by simply re-installing the OS.
Device Firmware Configuration Interface (DFCI) places high value configuration settings into PCs
UEFI BIOS.
UEFI DFCI storage is both visible to all OS instances, persistent, surviving OS reinstalls and
disk reformats, and tamper-resistant, defending itself from malware and rootkits.
UEFI executes before the OS and can disallow booting of specified devices, for example USB or
network PXE.
Further, DFCI can leverage hardware security to enforce some policies with higher assurance than
typical OS configuration.
For example, it could disable power to cameras or radios in a way that they could not be
re-enabled by an OS, malware, or rootkit.

### Popular Usages

* Disabling cameras, microphones, and/or radios in manufacturing and other secure facilities
* Disabling boot to USB and network for single purpose and KIOSK devices
* Disabling local user access to all UEFI settings to maintain the out of box configuration

## OEM Enablement Summary

DFCI enablement is comprised of:

1. UEFI BIOS implementation
2. Windows Autopilot participation

If an OEM or its Partners already participate in the Windows Autopilot program, no additional
Autopilot work is required, the only remaining work should be the UEFI BIOS implementation.

### Windows Autopilot Implementation

The pre-existing [Windows Autopilot device registration workflows](
https://docs.microsoft.com/en-us/windows/deployment/windows-autopilot/add-devices)
remain unchanged for DFCI, no additional work is required.
It should be noted that Autopilot self-registrations are not trusted for the purpose of
DFCI management (e.g. from Intune, Microsoft Store for Business, & Business 365).

### UEFI BIOS Implementation

DFCI enablement in UEFI BIOS requires implementation of DFCI interfaces and semantics, and
inclusion of a public Microsoft certificate.
There is precisely one (1) Microsoft zero-touch certificate that is shared by all DFCI-enabled
systems to authenticate zero-touch provisioning requests.
Thus there is no requirement to inject the certificate at manufacturing, it may simply be
included in the UEFI BIOS image.
The DFCI source code and public certificate are available on GitHub under a permissive open
source license (SPDX-License-Identifier: BSD-2-Clause-Patent).

* <https://github.com/microsoft/mu_plus/tree/dev/201908/DfciPkg>
* <https://github.com/microsoft/mu_plus/tree/dev/201908/ZeroTouchPkg>

There is also an example UEFI BIOS menu that demonstrates how to integrate DFCI:

* <https://github.com/microsoft/mu_oem_sample/search?q=dfci&unscoped_q=dfci>

## Unsigned Settings

Some platforms may choose to implement less secure deployment methods for certain settings.
DFCI allows a platform to supply either an allow list, or a disallow list, of a set of settings
that may be deployed using unsigned packets.
Unsigned settings may only be deployed when no DFCI Owner has been enrolled in the system
unless that DFCI owner has specifically allowed certain settings to be set by unsigned packets.
See the [section on platform integRation](PlatformIntegration/PlatformIntegrationOverview.md) for more information.

## UEFI Implementation Details

1. [**Scenarios:** Building the Microsoft Scenarios with DFCI](Scenarios/DfciScenarios.md)
2. [**Integration:** Integrating DFCI code into your platforms](PlatformIntegration/PlatformIntegrationOverview.md)
3. [**Architecture:** DFCI Code Internals](Internals/DfciInternals.md)

---

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
