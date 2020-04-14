# Microsoft DFCI Scenarios

## Overview

Microsoft [leverages DFCI to provide automated UEFI settings management via Microsoft Intune](https://docs.microsoft.com/en-us/intune/configuration/device-firmware-configuration-interface-windows).  Intune provides the IT manager with abstracted, easy button settings that are generally applicable to all platforms, for example, disable booting USB devices or disable all cameras.  IT managers depend on the UEFI implementation to protect and enforce DFCI configurations such that they cannot be bypassed by an operating system or casual physical attacker.  [Windows Autopilot](https://www.microsoft.com/en-us/microsoft-365/windows/windows-autopilot) provides the trusted device ownership database, mapping devices to Azure Active Directory Tenants.  The "Microsoft Device Management Trust" certificate must be included in UEFI to act as the root of trust for automated UEFI management.  The Autopilot Service (APS) exposes a cloud endpoint to enable recovery from a BIOS menu in case the device can no longer boot due to misconfiguration or disk corruption.

## Microsoft DFCI Scenario Requirements

* PCs must include the DFCI feature in their UEFI
* PCs must be registered to the [Windows Autopilot](https://www.microsoft.com/en-us/microsoft-365/windows/windows-autopilot) service by an OEM or [Microsoft Cloud Solution Provider](https://docs.microsoft.com/en-us/windows/client-management/mdm/uefi-csp)
* PCs must be managed with [Microsoft Intune](https://www.microsoft.com/en-us/microsoft-365/enterprise-mobility-security/microsoft-intune)

### OEMs that support DFCI

[![Microsoft Surface](collateral/SurfaceLogo_mu.png)](https://docs.microsoft.com/en-us/surface/surface-manage-dfci-guide)

More are in the works...

## Lifecycle

The DFCI lifecycle can be viewed as UEFI integration, device registration, profile creation, enrollment, management, retirement, & recovery.

![DFCI Scenarios](collateral/DFCI-Management_mu.png)

| Stage | Description |
| --- | --- |
| [UEFI Integration](../PlatformIntegration/PlatformIntegrationOverview.md) | PCs must first include a UEFI BIOS that integrates the DFCI code and includes the Microsoft Device Management Trust certificate. |
| [Device Registration](https://docs.microsoft.com/en-us/windows/deployment/windows-autopilot/add-devices#registering-devices) | Device ownership must be [registered](https://docs.microsoft.com/en-us/windows/deployment/windows-autopilot/add-devices#registering-devices) via the [Windows Autopilot](https://www.microsoft.com/en-us/microsoft-365/windows/windows-autopilot) program by an OEM or Microsoft [Cloud Solution Provider](https://partner.microsoft.com/en-US/membership/cloud-solution-provider) |
| Profile Creation | An IT administrator [leverages Intune to create DFCI Profiles](https://docs.microsoft.com/en-us/intune/configuration/device-firmware-configuration-interface-windows) for their devices. |
| Enrollment | The DFCI enrollment process is kicked off when a PC is enrolled into Intune and has a matching DFCI Profile.  Enrollment includes Intune requesting enrollment packets from APS, sending the packets to the [Windows UEFI configuration service provider (CSP) endpoints](https://docs.microsoft.com/en-us/windows/client-management/mdm/uefi-csp), the CSP writes the packets to UEFI variables, and triggers an OS reboot to allow UEFI firmware to process the DFCI packets. |
| Management | For day-to-day management, Intune creates device-specific packets, digitally signs them, and sends them through the same [UEFI configuration service provider](https://docs.microsoft.com/en-us/windows/client-management/mdm/uefi-csp), UEFI variable, and reboot process. |
| Retirement | When a device is removed from Windows Autopilot, they are marked as unenrolled in APS.  Intune will attempt to restore permissions (un-grey all settings) and remove its management authority from the device. |
| Recovery | Recovery shall be provided via a pre-boot UEFI menu, always available to a physically-present user, that can refresh DFCI configuration via web, USB, or other. |

## Enrollment Flow

Prior to the time of Enrollment, Microsoft Device Management Trust delegates management to the APS by signing a wildcard enrollment packet (targeting all manufacturer, model, & serial number) that authorizes enrollment of the APS certificate.  At the request of Intune, the APS authorizes enrollment of a device, creates and signs per-device-targeted enrollment packets that enroll the Intune DFCI management certificate.  The APS provides a level of indirection as well as an extra level of recovery via a web recovery service endpoint.  The APS additionally configures recovery settings as well as permissions that deny Intune access to modify them.  
**DEFINED:** A device is considered "enrolled" when `IS_OWNER_IDENTITY_ENROLLED(IdMask)` returns TRUE, unenrolled when FALSE.
Intune creates device-specific packets to provision the Intune authority and configure DFCI settings and permissions as specified in the matching Intune DFCI Profile.  Intune delivers these packets via the [Windows UEFI configuration service provider](https://docs.microsoft.com/en-us/windows/client-management/mdm/uefi-csp) which writes them to UEFI mailboxes, to be processed by DFCI on the following boot.  Device-specific packets include the SMBIOS manufacturer, model, & serial number, along with an anti-rollback count, to be leveraged by the UEFI DFCI code to determine applicability.

## Retirement & Recovery Flows

### Retirement

An IT administrator leverages the Intune console to remove devices from Autopilot.  Intune creates and sends device-specific packets that both restore DFCI permissions (effectively un-managing settings, making them available in the BIOS menu) and remove the Intune authority from DFCI.  Note that this does not restore the settings to default values, they remain as is.  Intune also notifies APS that the device is in the unenrolled state.  If the device owner wants to further remove the APS authority and/or opt-out of DFCI management, they must leverage the Recovery flow.

### Recovery

Recovery is essential because UEFI misconfiguration may prevent booting to an operating system, for example if USB and network boot are disabled and the hard disk becomes corrupted.  When a device is enrolled, UEFI must provide alternative mechanisms for the physically-present user to place packets in the DFCI request mailboxes - this MUST NOT be blocked by a BIOS password or similar.  Note that when a device is not enrolled, a BIOS password _should_ prevent access to DFCI enrollment by a physically-present user until they have entered the correct credential.
The APS keeps track of the enrollment state of devices.  When an administrator removes a device from Autopilot, APS creates signed, device-specific un-enrollment packets and makes them available via a REST endpoint at DFCI_PRIVATE_SETTING_ID__DFCI_RECOVERY_URL.  These packets should delete the Intune and APS certificates and provide the local user with access to all settings (they should no longer be greyed out in BIOS menus).  Note that retirement does not restore visible settings to their default values.

![DFCI Recovery](collateral/DFCI-Recovery_mu.png)

## Standard Settings

The following standard settings are defined for DFCI v1.0, will be exposed by Intune, and Settings Providers for them must be implemented by the UEFI provider.  Note that for device management, the settings apply only to built-in devices, not externally attached devices.

* All cameras
* All audio (microphones & speakers)
* All radios (Wi-Fi, broadband, NFC, BT, ...)
* CPU & IO virtualization (exposed, enabled for use by the OS so that Virtualization Based Security may be enabled)
* Boot to external media (e.g. USB, SD, ...)
* Boot via on-board network devices

Refer to the "Group Settings" section of [DfciSettings.h](https://github.com/microsoft/mu_plus/blob/dev/201908/DfciPkg/Include/Settings/DfciSettings.h)

## Device Ownership

The Windows Autopilot database is used for authorizing DFCI management.  It includes a map of device identifiers an owner's Azure Active Directory tenant.  It is [populated](https://docs.microsoft.com/en-us/windows/deployment/windows-autopilot/add-devices#registering-devices) as part of the Windows Autopilot program by OEMs and Microsoft-authorized [Cloud Solution Providers](https://partner.microsoft.com/en-US/membership/cloud-solution-provider).  The DFCI UEFI code leverages the SMBIOS manufacturer, model, and serial number for packet targeting.  DFCI is a Windows Autopilot feature, available from Microsoft Intune.

## Management Authorities

DFCI supports UEFI settings and permission management by multiple entities.  In the recommended configuration, systems are shipped with 1 Microsoft public certificate included, "CN=Microsoft Device Management Trust", which provides the root of trust for management.  This certificate authorizes Microsoft to automatically enroll management delegates with varying permissions.  "Microsoft Device Management Trust" is only used to delegate management to APS and to provide second-chance recovery.  APS in turn delegates management to Intune and provides a REST endpoint for online recovery. After enrollment, Intune performs the day-to-day UEFI management, whereas APS and Device Management Trust authorities provide various recovery paths.

| Authority | DFCI ID | Usage |
| --- | --- | --- |
| "CN=Microsoft Device Management Trust" | DFCI_IDENTITY_SIGNER_ZTD | Allowed to enroll a management delegates without a physical presence prompt.  Enrolls the Autopilot Service authority.  Can act as a backup recovery service. |
| Autopilot Service (APS) | DFCI_IDENTITY_SIGNER_OWNER | Enrolls Microsoft Intune as a delegated management provider.  Provides an online recovery service in case the OS is disconnected from Intune. |
| Microsoft Intune | DFCI_IDENTITY_SIGNER_USER | Performs day-to-day UEFI settings and permissions management |

## Security & Privacy Considerations

When a DFCI owner is enrolled, DFCI must take precedence over any other UEFI management solution.  Physically-present user, including authenticated, may not bypass DFCI permissions on `DFCI_IDENTITY_LOCAL`.

**Protection:** The hardware/firmware implementation of DFCI must __protect__ DFCI configuration code and data such that they cannot by bypassed by an operating system or casual physical attacker.  For example, non-volatile storage that is locked prior to Boot Device Selection.

**Enforcement:** The hardware/firmware implementation of DFCI must __enforce__ DFCI configurations such that they cannot by bypassed by an operating system or casual physical attacker.  For example, power to devices is disabled or busses disabled and the configuration is locked prior to Boot Device Selection.

It is recommended to include an "Opt Out" button that enables a physically-present user on an unenrolled device to eject the DFCI_IDENTITY_SIGNER_ZTD from DFCI.  The effectively disables automated management - any enrollment attempt will display a red prompt at boot.
![DFCI OptOut](collateral/opt_out_mu.jpg)
A user could then prevent enrollment by configuring a BIOS password or enroll their own User certificate (proceeding through the red prompt).

### Online Recovery via the Autopilot Service

The Recovery REST interface includes machine identities.  Before transferring machine identities, the server's authenticity should be verified against DFCI_PRIVATE_SETTING_ID__DFCI_HTTPS_CERT.  After authenticating the server, the network traffic, including machine identities, are kept private by HTTPS encryption.  But wait, there's more... the server certificate is updated regularly, so UEFI must first ensure it has the up-to-date DFCI_PRIVATE_SETTING_ID__DFCI_HTTPS_CERT.  DFCI_PRIVATE_SETTING_ID__DFCI_BOOTSTRAP_URL provides a REST API to download a signed settings packet containing DFCI_PRIVATE_SETTING_ID__DFCI_HTTPS_CERT.  For this workflow, the server is not authenticated, but the payload will be authenticated prior to consumption.

### Unknown Certificate Enrollment

This is *not* a Microsoft-supported scenario but might be encountered during development and testing.  On an unenrolled system, if enrollment packets are supplied to the DFCI mailboxes that are signed by an unknown certificate, a red authorization prompt is displayed during boot.  The prompt requests the physically-present user to authorize the enrollment of the unknown certificate by typing the last 2 characters of the certificate's SHA-1 thumbprint.  If a BIOS password is configured, the password must be entered prior to authorizing the enrollment. This is designed to avoid accidental or nefarious enrollment while allowing for valid custom identity management.
