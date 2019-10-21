# Platform Integration of DFCI

This section of documentation is focused on UEFI firmware developers and helping them enable their platforms with the DFCI feature.  

DFCI consists mostly of a software feature that is written in the DXE phase of UEFI.  It has numerous architecture and platform independent modules with only a few required platform libraries.  It also requires the platform adhere to and use the DFCI components to ensure the DFCI features work as designed.  Finally to enable an End-To-End management scenario there maybe custom requirements in adjacent UEFI firmware components.

## Dfci Menu application

The [DfciMenu](https://github.com/microsoft/mu_plus/tree/dev/201908/DfciPkg/Application/DfciMenu) application is optimized for **mu_plus MsGraphicsPkg**.  It is VFR but since many platforms use custom layouts and graphical representation this area might need some adjustments.  The DfciMenu application publishes a HII formset that should be located by your "frontpage" and shown.

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

The following libraries have to be provided by the platfrom:

| Library | Documentation | Function |
| ----- | -----| ----- |
| DfciDeviceIdSupportLib | [Documentation](DfciDeviceIdSupportLib.md) | Provides SMBIOS information - Manufacturer, Product, and Serial number |
| DfciGroupLib | [Documentation](DfciGroups.md) | Provides lists of platform settings that are in the Dfci group settings.
| DfciUiSupportLib | [Documentation](DfciUiSupportLib.md) | Provides UI for various user interactions |

## DFCI Setting Providers

Setting providers is how a platform provides a setting to DFCI

[Setting detailed overview](DfciSettingProviders.md)

## Platform DSC statememts

Adding DFCI to your system consists of:

1. Write your settings providers. Use **DfciPkg/Library/DfciSampleProvider**.
2. Writing three library classes for the DfciDeviceIdSupportLib, DfciGroupLib, and DfciUiSupportLib.
3. Adding the DSC sections below.
4. Adding the FDF sections below.

``` yaml
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
        NULL|YOURPLATFORMPKG/Library/YOUR_FIRST_SETTING_PROVIDER.inf
        NULL|YOURPLATFORMPKG/Library/YOUR_SECOND_SETTING_PROVIDER.inf
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

```yaml
[FV.YOUR_DXE_FV]
INF  DfciPkg/SettingsManager/SettingsManagerDxe.inf
INF  DfciPkg/IdentityAndAuthManager/IdentityAndAuthManagerDxe.inf
INF  DfciPkg/Application/DfciMenu/DfciMenu.inf
INF  DfciPkg/DfciManager/DfciManager.inf
```
