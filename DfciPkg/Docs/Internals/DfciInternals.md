# DFCI Internals

This section describes the internal operations of DFCI.

## Communications with Provider

DFCI communicates with a controlling identity.
One of the controlling identities could be Microsoft Intune.
The communications path from the controlling identity is though the use of UEFI variables.
DFCI processes the mailbox variables during a system restart.

## Identity Manager

In the source code, the Identity manager is implemented in **IdentityAndAuthManager** is defined
in the **DfciPkg** located in the **mu_plus** repository <https://github.com/microsoft/mu_plus/>.
Identity and Auth Manager is responsible for managing the Identities.
The initial state of the system has the Local User with full authentication to make changes to
any of the available settings.
There are seven Identities known by DFCI:

| Identity   | Owner Mask | Use of the Identity |
| ---        | ---  | --- |
| Owner      | 0x80 | The system owner. Used by a controlling agent - that authorizes Use to control some settings |
| User       | 0x40 |  A delegated user.  Used by Microsoft Intune.
| User1      | 0x20 |  Not currently used |
| User2      | 0x10 |  Not currently used |
| Zero Touch | 0x08 |  Limited use Identity to allow an Enroll from a controlling agent.  The system has the Zero Touch Certificate installed during manufacturing.  Zero Touch cannot be enrolled through the normal enroll operation. Zero Touch has no use when a system is enrolled. |
| Reserved   | 0x04 |  |
| Unsigned   | 0x02 |  Not a certificate - Limited use Identity used as the Identity processing unsigned settings packets |
| Local User | 0x01 |  Not a certificate - just a known, default, user |

The Identity Manager reads the incoming mailbox to process a Identity enroll, Identity
certificate update, and Identity unenroll operations.
Except for the Local User, when an Identity is enrolled, it means adding a Certificate that will
be used to validate incoming settings.

The Identity Manager verifies that the incoming identity mailbox packet:

1. Is signed by one of the Identities
2. The signed identity has permission to update the target identity.
3. Target information in the packet matches the system information

The one exception is when an new Owner is being enrolled, no Identities validate the mailbox
packet, and the Local User has permission to enroll an owner, DFCI will pause booting to prompt
the Local User for permission to do the enroll.
The user will be asked to validate the enrollment by entering the last two characters of the new
owners certificate hash.

When installed by the manufacturer of the system, the Zero Touch certificate will have
permission to allow the Zero Touch owner packet to be enrolled without user intervention.
Hence, the term Zero Touch enrollment.

## Permissions Manager

The permission manager processes incoming permission mailbox packets.
Permission packets must be signed by one of Owner, User, User1 or User2.
When processing the incoming permissions XML, the signer permissions are used to enable adding
or changing a permission.

## Settings Manager

The settings manager processes incoming settings mailbox packets.
Settings packets must be signed by one of Owner, User, User1 or User2.
When processing the incoming settings XML, the signer permissions are used to change a setting.

## Identity Packet Formats

An Identity packet consists of a binary header, a DER encoded certificate file, a test signature
validating the signing capability, and the signature of the packet:

![Identity packet format](Images/EnrollPacket_mu.jpg)

The Test Signature is the detached signature of signing the public key certificate by the
private key of the public key certificate.
The Signature field of the packet is the detached signature of signing
Header-PublicCert-TestSignature by:

| Operation | Signing Key |
| ---       | --- |
| Enroll    | The private key of the matching Public Key Certificate |
| Roll      | The private key matching the public cert of the Identity being rolled.
| Unenroll  | The private key matching the public cert of the Identity being unenrolled.

![Identity packet signatures](Images/EnrollPacketSignature_mu.jpg)

## Permission Packet Formats

A Permission packet consists of a binary header, an XML payload, and a signature:

![Permission packet format](Images/PermissionPacket_mu.jpg)

![Permission packet signature](Images/DataPacketSignature_mu.jpg)

Sample permission packet:

```xml
<?xml version="1.0" encoding="utf-8"?>
<PermissionsPacket xmlns="urn:UefiSettings-Schema">
    <CreatedBy>Cloud Controller</CreatedBy>
    <CreatedOn>2018-03-28</CreatedOn>
    <Version>1</Version>
    <LowestSupportedVersion>1</LowestSupportedVersion>
    <Permissions Default="129" Delegated="192" Append="False">
        <!--

           Sample DDS initial enroll permissions

           Permission Mask - 128 (0x80) = Owner
                              64 (0x40) = User
                              32 (0x20) = User1
                              16 (0x10) = User2
                               8 (0X08) = ZTD
                               2 (0X02) = Unsigned Settings
                               1 (0x01) = Local User
           Owner keeps the following settings for itself
         -->
        <Permission>
            <Id>Dfci.OwnerKey.Enum</Id>
            <PMask>128</PMask>
            <DMask>128</DMask>
        </Permission>
        <Permission>
            <Id>Dfci.Recovery.Enable</Id>
            <PMask>128</PMask>
            <DMask>128</DMask>
        </Permission>
        <Permission>
            <!--
                   Needs 128 (Owner Permission) to set the key,
                   Needs  64 (User Permission) for User to roll the key
             -->
            <Id>Dfci.UserKey.Enum</Id>
            <PMask>192</PMask>
            <DMask>128</DMask>
        </Permission>
        <Permission>
            <Id>Dfci.RecoveryBootstrapUrl.String</Id>
            <PMask>128</PMask>
            <DMask>128</DMask>
        </Permission>
        <Permission>
            <Id>Dfci.RecoveryUrl.String</Id>
            <PMask>128</PMask>
            <DMask>128</DMask>
        </Permission>
        <Permission>
            <Id>Dfci.Hwid.String</Id>
            <PMask>128</PMask>
            <DMask>128</DMask>
        </Permission>
  </Permissions>
</PermissionsPacket>
```

## Settings Packet Formats

![Setting packet signature](Images/SettingsPacket_mu.jpg)

![Setting packet signature](Images/DataPacketSignature_mu.jpg)

Sample settings payload:

```xml
<?xml version="1.0" encoding="utf-8"?>
<SettingsPacket xmlns="urn:UefiSettings-Schema">
    <CreatedBy>Mike Turner</CreatedBy>
    <CreatedOn>2019-03-06 10:10:00</CreatedOn>
    <Version>2</Version>
    <!--

      Make sure you edit DfciSettingsPattern.xml and then
      run BuildSettings.bat to generate the DfciSettings.xml

      -->
    <LowestSupportedVersion>2</LowestSupportedVersion>
    <Settings>
        <Setting>
            <Id>Dfci.RecoveryBootstrapUrl.String</Id>
            <Value>http://some URL to access recovery cert updates/</Value>
        </Setting>
        <Setting>
            <Id>Dfci.RecoveryUrl.String</Id>
            <Value>https://some URL to access recovery update packets/</Value>
        </Setting>
        <Setting>
            <Id>Dfci.HttpsCert.Binary</Id>
            <Value>
                <!--
                    This is where a BASE64 encoded string of the certificate used for HTTPS operations is stored.
                -->
                 wA==
            </Value>
        </Setting>
        <Setting>
            <Id>Dfci.RegistrationId.String</Id>
            <Value>
                12345678-1234-5678-1234-012345674321
            </Value>
        </Setting>
        <Setting>
            <Id>Dfci.TenantId.String</Id>
            <Value>
                98765432-1234-5678-1234-012345674321
            </Value>
        </Setting>
    </Settings>
</SettingsPacket>
```

## Packet Processing

In order to minimize rebooting when accepting packets from the owner, there are 6 mailboxes for DFCI
There are two for each Identity, Permission, and Settings.  We call them:

1. Identity
2. Identity2
3. Permission
4. Permission2
5. Settings
6. Settings2

Only packets of the correct type are processed out of each mailbox.
Packets are processed in the following order:

1. Enroll Identity
2. Enroll Identity2
3. Apply Permission
4. Apply Permission2

    At this point, if there is a severe error, the Identities and Permissions are reverted to what
    they were before processing the packets. The following are still processed, in order.

5. Settings
6. Settings2
7. Unenroll Identity 2
8. Unenroll Identity

## UEFI CSP

Intune accesses the variables through the
[UEFI CSP](https://docs.microsoft.com/en-us/windows/client-management/mdm/uefi-csp)
provider.

## Out of band recovery

Normally, the cloud provider would just send unenroll packets through the OS to the UEFICsp.  However, if Windows is
unable to boot, the UEFI front page application has a method to contact the owner via HTTPS.

## Setting Provider

Settings providers provide an interface for DFCI to access platform settings.
The OEM is responsible for providing one or more settings providers for platform settings.
The OEM is required to implement an instance of DfciGroupLib that maps DFCI-standard settings to OEM
platform settings.
While DFCI-standard settings are abstract (eg. Dfci.OnboardAudio.Enable), a platform may have multiple
settings that cover portions of audio.
For example, there may be a microphone or other input setting, and a setting to enable or
disable the audio output.
Using the DfciGroup lib, the individual platform settings can be mapped to the DFCI-standard settings.
For more information on groups, see [Dfci Groups](../PlatformIntegration/DfciGroups.md)

### Setting Provider expected return codes

| Return Code           | Reason to return this code |
| ---                   | --- |
| EFI_SUCCESS           | Operation of get or set was successful, and the returned value, if any, is valid |
| EFI_NOT_FOUND         | Returned by DFCI if a setting provider is not found. [See notes](#notes-on-efi-not-found) |
| EFI_UNSUPPORTED       | Particular operation is not supported. [See notes](#notes-on-efi-unsupported) |
| EFI_BUFFER_TOO_SMALL  | Get operation called with 0 size to get the size of a buffer to allocate |
| EFI_INVALID_PARAMETER | Coding error that provided incorrect parameters |
| EFI_OUT_OF_RESOURCES  | Possibly out of memory, or some other lower function error |

### Notes on EFI NOT FOUND

There are two expected reasons a settings provider could return EFI_NOT_FOUND.
The first is that the settings is a DFCI standard setting from a newer version of DFCI than was
implemented in the platform.
The second is that the setting does not apply to the platform because it does not contain the
feature.
For example, if a platform has no cameras at all, it should not implement a Settings Provider for
them.
When EFI_NOT_FOUND is returned, Intune will additionally compare the DFCI version reported to
determine compliance.
For example, a platform reporting DFCI v1, and which has no cameras, returns EFI_NOT_FOUND on a
request to disable all cameras.
Intune infers that the lack of a standard settings provider for a given version of DFCI
indicates that a setting is not relevant.
On attempts to enable or disable this setting, Intune remaps this error reporting compliant or
not-applicable.
On a request to disable WPBT (a DFCI version 2 feature), the same platform would return
EFI_NOT_FOUND because it lacks a DFCI version 2 Standard WPBT settings provider.
Intune would observe the DFCI version, and report non-compliant, because the platform cannot
manage or reliably report the status of the requested setting.

### Notes on EFI UNSUPPORTED

There are reasons for an operation to be not supported.
One example is that a platform may provide a setting provider that has a setting that is always
disabled, and return EFI_UNSUPPORTED when there is an attempt to enable that setting.
Another case is if the hardware doesn't support the setting, or the OEM has chosen not to allow
certain settings.

### Settings provider for enabled features without platform control

Please refer to the DfciVirtualizationSettings provider in [Project mu_plus](
https://github.com/microsoft/mu_plus/tree/release/202002/DfciPkg/Library/DfciVirtualizationSettings)
for this section.
A platform may conform to a Enabled setting, but have no method to control this setting.
The CPU and I/O virtualization setting is one of those.
The table below indicates the return code for specific cases of of this type of provider:

| Return Code     | Reason to return this code |
| ---             | --- |
| EFI_SUCCESS     | Operation of get was successful, and the returned value is valid |
| EFI_UNSUPPORTED | A Set operation is not allowed |

---

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent
