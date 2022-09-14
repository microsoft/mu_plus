# Platform Integration of DFCI

This section of documentation is focused on UEFI firmware developers and helping them enable their platforms with the
DFCI feature.

DFCI consists mostly of a software feature that is written in the DXE phase of UEFI.  It has numerous architecture and
platform independent modules with only a few required platform libraries. It also requires the platform adhere to and
use the DFCI components to ensure the DFCI features work as designed.  Finally to enable an End-To-End management
scenario there maybe custom requirements in adjacent UEFI firmware components.

## Dfci Menu application

The [DfciMenu](https://github.com/microsoft/mu_plus/tree/dev/201908/DfciPkg/Application/DfciMenu) application is
optimized for **mu_plus MsGraphicsPkg**. It is VFR but since many platforms use custom layouts and graphical
representation this area might need some adjustments.  The DfciMenu application publishes a HII formset that should be
located by your pre-boot UEFI menu application (e.g. "FrontPage") and displayed.

* Formset GUID: `gDfciMenuFormsetGuid = {0x3b82283d, 0x7add, 0x4c6a, {0xad, 0x2b, 0x71, 0x9b, 0x8d, 0x7b, 0x77, 0xc9 }}`
* Entry Form: `#define DFCI_MENU_FORM_ID           0x2000`
* Source Location: `DfciPkg\Application\DfciMenu`

## DFCI DXE Drivers

| Dxe Driver | Location |
| ---| ---|
| DfciManager.efi | DfciPkg/DfciManager/DfciManager.inf |
| IdentityAndAuthManager.efi | DfciPkg/IdentityAndAuthManager/IdentityAndAuthManagerDxe.inf |
| SettingsManager.efi | DfciPkg/SettingsManager/SettingsManagerDxe.inf |
| DfciMenu.inf | DfciPkg/Application/DfciMenu/DfciMenu.inf |

## DFCI Core Libraries

*These DFCI Standard libraries are expected to be used as is for standard functionality.*

| Library | Location |
| --- | ---|
| DfciRecoveryLib | DfciPkg/Library/DfciRecoveryLib/DfciRecoveryLib.inf |
| DfciSettingsLib | DfciPkg/Library/DfciSettingsLib/DfciSettingsLib.inf |
| DfciV1SupportLib | DfciPkg/Library/DfciV1SupportLibNull/DfciV1SupportLibNull.inf |
| DfciXmlDeviceIdSchemaSupportLib | DfciPkg/Library/DfciXmlDeviceIdSchemaSupportLib/DfciXmlDeviceIdSchemaSupportLib.inf |
| DfciXmlIdentitySchemaSupportLib | DfciPkg/Library/DfciXmlIdentitySchemaSupportLib/DfciXmlIdentitySchemaSupportLib.inf |
| DfciXmlPermissionSchemaSupportLib | DfciPkg/Library/DfciXmlPermissionSchemaSupportLib/DfciXmlPermissionSchemaSupportLib.inf |
| DfciXmlSettingSchemaSupportLib | DfciPkg/Library/DfciXmlSettingSchemaSupportLib/DfciXmlSettingSchemaSupportLib.inf |
| ZeroTouchSettingsLib | ZeroTouchPkg/Library/ZeroTouchSettings/ZeroTouchSettings.inf|
| DfciSettingPermissionLib | DfciPkg/Library/DfciSettingPermissionLib/DfciSettingPermissionLib.inf |

## DFCI Platform provided libraries

The following libraries have to be provided by the platform:

| Library | Documentation | Function |
| ----- | -----| ----- |
| DfciDeviceIdSupportLib | [Documentation](DfciDeviceIdSupportLib.md) | Provides SMBIOS information - Manufacturer, Product, and Serial number |
| DfciGroupLib | [Documentation](DfciGroups.md) | Provides lists of platform settings that are in the Dfci group settings.
| DfciUiSupportLib | [Documentation](DfciUiSupportLib.md) | Provides UI for various user interactions |

## DFCI Setting Providers

Setting providers is how a platform provides a setting to DFCI

[Setting detailed overview](DfciSettingProviders.md)

## Mu Changes

* DFCI Recovery service uses HTTPS certificates with Subject Alternative Names.  This requires a source modification to NetworkPkg,
[removal of EFI_TLS_VERIFY_FLAG_NO_WILDCARDS from TlsConfigureSession()](https://github.com/microsoft/mu_basecore/commit/931ff1a45ce13a6a8c3e296f89c6de21f23a17ed#diff-620e10fa41a63814688b931d19fefa89R628).
* [Configure OpenSSL to support modern TLS ciphers](https://github.com/microsoft/mu_tiano_plus/commit/1f3b135ddc821718a78c352316197889c5d3e0c2)

## Platform DSC statements

Adding DFCI to your system consists of:

1. Write your settings providers. Use **DfciPkg/Library/DfciSampleProvider**.
2. Writing three library classes for the DfciDeviceIdSupportLib, DfciGroupLib, and DfciUiSupportLib.
3. Adding the DSC sections below.
4. Adding the FDF sections below.

```text
[LibraryClasses.XXX]
  DfciXmlSettingSchemaSupportLib|DfciPkg/Library/DfciXmlSettingSchemaSupportLib/DfciXmlSettingSchemaSupportLib.inf
  DfciXmlPermissionSchemaSupportLib|DfciPkg/Library/DfciXmlPermissionSchemaSupportLib/DfciXmlPermissionSchemaSupportLib.inf
  DfciXmlDeviceIdSchemaSupportLib|DfciPkg/Library/DfciXmlDeviceIdSchemaSupportLib/DfciXmlDeviceIdSchemaSupportLib.inf
  DfciXmlIdentitySchemaSupportLib|DfciPkg/Library/DfciXmlIdentitySchemaSupportLib/DfciXmlIdentitySchemaSupportLib.inf
  ZeroTouchSettingsLib|ZeroTouchPkg/Library/ZeroTouchSettings/ZeroTouchSettings.inf
  DfciRecoveryLib|DfciPkg/Library/DfciRecoveryLib/DfciRecoveryLib.inf
  DfciSettingsLib|DfciPkg/Library/DfciSettingsLib/DfciSettingsLib.inf
  DfciV1SupportLib|DfciPkg/Library/DfciV1SupportLibNull/DfciV1SupportLibNull.inf

  DfciDeviceIdSupportLib|YOURPLATFORMPKG/Library/DfciDeviceIdSupportLib/DfciDeviceIdSupportLib.inf
  DfciUiSupportLib|YOURPLATFORMPKG/Library/DfciUiSupportLib/DfciUiSupportLib.inf
  DfciGroupLib|YOURPLATFORMPKG/Library/DfciGroupLib/DfciGroups.inf

[Components.XXX]
  DfciPkg/SettingsManager/SettingsManagerDxe.inf {
  #Platform should add all it settings libs here
  <LibraryClasses>
        NULL|ZeroTouchPkg/Library/ZeroTouchSettings/ZeroTouchSettings.inf
        NULL|YOUR_PLATFORM_PKG/Library/YOUR_FIRST_SETTING_PROVIDER.inf
        NULL|YOUR_PLATFORM_PKG/Library/YOUR_SECOND_SETTING_PROVIDER.inf
        NULL|DfciPkg/Library/DfciPasswordProvider/DfciPasswordProvider.inf
        NULL|DfciPkg/Library/DfciSettingsLib/DfciSettingsLib.inf
        NULL|DfciPkg/Library/DfciVirtualizationSettings/DfciVirtualizationSettings.inf
        DfciSettingPermissionLib|DfciPkg/Library/DfciSettingPermissionLib/DfciSettingPermissionLib.inf
  <PcdsFeatureFlag>
     gDfciPkgTokenSpaceGuid.PcdSettingsManagerInstallProvider|TRUE
  }

  DfciPkg/IdentityAndAuthManager/IdentityAndAuthManagerDxe.inf
  DfciPkg/DfciManager/DfciManager.inf
  DfciPkg/Application/DfciMenu/DfciMenu.inf
```

## Platform FDF statements

```text
[FV.YOUR_DXE_FV]
INF  DfciPkg/SettingsManager/SettingsManagerDxe.inf
INF  DfciPkg/IdentityAndAuthManager/IdentityAndAuthManagerDxe.inf
INF  DfciPkg/Application/DfciMenu/DfciMenu.inf
INF  DfciPkg/DfciManager/DfciManager.inf
```

## Unsigned Settings packets

Dfci has a feature where a platform can enable some settings to be changes with an
unsigned packet.
This is allowed only when the system is not enrolled in Dfci.
This can allow setting parameters that don't affect the security of the system and there is
a cost benefit to being able to deploy these setting changes easily, and not step up to full Dfci.
To enable to platform to allow unsigned settings, the platform must produce an unsigned permission
list in xml format and include this xml file in the platform build .fdf file:

```xml
<!--
NOTE:
    None of the Permission Masks or the Delegated Masks are actually used.
    However, they must be present for the XML parser used by Dfci.
    This include Default, Delegated, Append, PMask, and DMask values.
 -->
<PermissionsPacket xmlns="urn:UefiSettings-Schema">
    <Permissions Default="243" Delegated="0" Append="False">
        <Permission>
            <Id>Device.PlatformSetting1.Enable</Id>
            <PMask>243</PMask>
            <DMask>0</DMask>
        </Permission>
        <Permission>
            <Id>Device.PlatformSetting2.Enable</Id>
            <PMask>243</PMask>
            <DMask>0</DMask>
        </Permission>

        <Permission>
            <Id>Device.PlatformSetting3.Enable</Id>
            <PMask>243</PMask>
            <DMask>0</DMask>
        </Permission>
      </Permissions>
</PermissionsPacket>
```

To include this file in the platform .fdf file, do the following:

```ini
FILE FREEFORM = PCD(gDfciPkgTokenSpaceGuid.PcdUnsignedPermissionsFile) {
    SECTION RAW = YourPlatformPkg/StaticFiles/UnsignedPermissions.xml
}
```

Certain platforms may choose to enable all settings to be set via unsigned packets by
building with the Pcd PcdUnsignedListFormatAllow set to FALSE.
This will enable all settings to be changed using unsigned packets.
When PcdUnsignedListFormatAllow is FALSE, the unsigned settings list becomes a disallow list,
providing a list of settings that do NOT have the permission to be set by an unsigned
packet.
An Unsigned Permissions file is required to be read before the Disallow operation
is enabled.

To generate an unsigned settings packet, refer to the DFCI_UnsignedSettings test case.
The GenUsb.bat file will produce an unsigned packet (Unsigned_Settings_apply.bin) from a
settings xml file (UnsignedSettings.xml).

To deploy the Unsigned_Settings_apply.bin file, set the UEFI Variable
gDfciSettingsManagerVarNamespace:DfciSettingsRequest to the contents of the
Unsigned_Settings_apply.bin file, and restart the system.

## Testing DFCI operation

Please refer to the [DFCI TestCase documentation](../../UnitTests/DfciTests/readme.md)
