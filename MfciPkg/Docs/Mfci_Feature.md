# **DRAFT:** Manufacturer Firmware Configuration Interface (MFCI)

## **NOTE**

The following is prerelease documentation for an upcoming feature.  Details are subject to change.

## Overview

### In a Sentence

Manufacturer Firmware Configuration Interface (MFCI) is a UEFI BIOS feature that provides a secure
mechanism for an authorized agent, such as an OEM or ODM, to modify firmware security properties
of a specific device, typically for the purposes of remanufacturing or refurbishment.  

### Background

Manufacturing and remanufacturing (refurbishment) of devices requires configuration of settings
related to safety (batteries, thermals), compliance (radio calibration, anti-theft), licensing
(OA3, serial numbers), & security.  These sensitive settings are typically secured by the
OEM / ODM at end of manufacturing to ensure that they are not modified with malicious intent.
During initial manufacturing, a device is typically not secured until the final provisioning
station, after calibration and other secure data have been provisioned.  After a device is secured,
product demand may deviate from forecasts and a device may need to be reprovisioned for a different
region, requiring modification of secured settings.  Remanufacturing of devices requires executing
tools and workflows that bypass this security, allowing modification of these sensitive settings,
and performing potentially destructive diagnostics.  The Windows 10X Compatibility
Requirements prohibit unauthorized execution of these dangerous tools. The MFCI feature provides a
secure path to enable remanufacturing while maintaining the Windows 10X security promise.

### Building an End-to-End MFCI Solution

An MFCI-based solution requires:

1. Devices with MFCI integrated into their UEFI BIOS
1. An authority that produces signed MFCI policy blobs, for example, Microsoft's cloud MFCI service from Hardware Dev Center
1. A manufacturer process that connects the MFCI-enabled device to an MFCI policy service

The Project Mu MfciPkg provides the reference code to enable device-side UEFI BIOS, and includes examples of signing
authorities & processes needed to implement a solution.
  
## A Remanufacturing Example

### Conceptual Workflow

1. Determine the desired 64-bit MFCI policy to be applied to the device based upon the type of
remanufacturing to be performed (see [MFCI Structures](Mfci_Structures.md) for more information)
2. Read targeting information stored in OS-visible UEFI variables from the device
3. Combine the 64-bit MFCI policy and targeting information to construct an unsigned MFCI blob
4. Digitally sign the MFCI blob using the specified digital signing format and trusted signing keys
5. Write the signed MFCI Policy blob to the "next" policy blob mailbox (a UEFI variable) on the target device
6. Reboot the target device to trigger an installation attempt  
   Prior to OS launch, UEFI attempts to verify the digital signature and targeting information  
    * If verification fails, the policy is deleted from the "next" policy blob mailbox, and
    the device proceeds with boot to the OS
    * If verification succeeds, MFCI policy is applied:
        1. Callbacks are notified to perform any 1-time actions
        2. Designated action bit ranges are cleared from the policy
        3. If the policy contains persistent states, it is moved to the "current" policy blob mailbox, otherwise it is
            deleted from the "next" policy blob mailbox
        4. The device reboots for the new policy to take effect
        5. The device's UEFI code should now modify its behavior based upon the persistent state bits or device changes
            resulting from the 1-time actions
7. If a persistent MFCI Policy was installed, it is removed by deleting the "current" policy blob mailbox

### Example Workflow Using Microsoft Tools & Services

1. Boot device to FFU mode
2. Use Microsoft-supplied tool ImageUtility.exe to read the targeting variables from the device
3. Use Microsoft Hardware Dev Center (web portal or REST APIs) to generate the signed policy
    1. Authenticate from an authorized account
    2. Supply the targeting and policy parameters
    3. Receive the signed MFCI Policy blob for your device
4. Use the Microsoft-supplied tool ImageUtility.exe to set the signed policy blob to a mailbox
5. Reboot, and the new policy should take effect
6. OEM performs customer service or re-manufacturing as needed. Note that if the device make, model, serial
number, or related UEFI variables change or are deleted, the policy may no longer be valid on subsequent boots.
7. After re-manufacturing, remove the policy using MS-supplied tool ImageUtility.exe to delete the policy

## Deep Dives

* [MFCI Structures](Mfci_Structures.md)
* [UEFI Integration Guide](Mfci_Integration_Guide.md)
* [End to End Testing Guide](TODO)
