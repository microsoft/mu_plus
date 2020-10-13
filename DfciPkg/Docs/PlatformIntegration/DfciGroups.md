# DFCI Groups

DFCI Groups allow numerous like typed settings to be managed together.  Groups can also be used to provide multiple names for the same setting.  This allows actual device settings to be mapped into a different namespaces for settings.

One such example from the Microsoft scenario is `Dfci.OnboardCameras.Enable`.  This group setting is used to manage the state of all onboard cameras and gives the management entity an ability to control all cameras regardless of how many each platform has.  A platform may have `Device.FrontCamera.Enable` and `Device.RearCamera.Enable` settings.  Adding those settings to the group `Dfci.OnboardCameras.Enable` allows a general purpose management entity to control both.

## Handling State Reporting

If all of the settings of the group have the same value then that value will be returned (ie `Enabled` or `Disabled` for an Enable type setting).  If they are not the same then `Inconsistent` will be returned. `Unknown` will be returned if there are no members in a group.

## Restrictions on group names

Group names and settings names are in the same name space and duplicate names are not allowed. Like settings names, group names are limited to 96 characters in length, and are null terminated CHAR8 strings.

## DfciGroupLib

The DfciGroupLib is how groups are managed. This library separates the grouping configuration from the setting providers to allow better flexibility and better maintainability.

**DfciGroupLib** is defined in the **DfciPkg** located in **mu_plus** repository <https://github.com/microsoft/mu_plus/>

## Library Interfaces

| Interfaces | Usage |
| ----- | ----- |
| DfciGetGroupEntries | DfciGetGroupEntries returns an array of groups, and each group points to a list of settings that are members of the group.

## DFCI Standard Groups for OEM extension

| Group Setting String | Description |
| - | - |
| "Dfci.OnboardCameras.Enable" | Enable/Disable all built-in cameras |
| "Dfci.OnboardAudio.Enable" | Enable/Disable all built-in microphones & speakers |
| "Dfci.OnboardRadios.Enable" | Enable/Disable all built-in radios (e.g. Wi-Fi, BlueTooth, NFC, Mobile Broadband...) |
| "Dfci.BootExternalMedia.Enable" | Enable/disable boot from external media |
| "Dfci.BootOnboardNetwork.Enable" | Enable/disable boot from built-in network adapters |
| "Dfci.CpuAndIoVirtualization.Enable" | Enable/disable both CPU & IO Virtualization (i.e. prerequisite for Windows Virtualization Based Security (a.k.a. Device Guard, Core Isolation, Secured Core) |

## Example

* Declare names for all individual BIOS settings that may be modified by the DFCI-standard settings.  The DFCI-standard string values are prefixed with "```Dfci.```".  A naming convention for device-specific setting strings is proposed as "```Device.```", as follows:

``` c
// Cameras
//
// Group setting                                      "Dfci.OnboardCameras.Enable"
#define DEVICE_SETTING_ID__FRONT_CAMERA               "Device.FrontCamera.Enable"
#define DEVICE_SETTING_ID__REAR_CAMERA                "Device.RearCamera.Enable"
#define DEVICE_SETTING_ID__IR_CAMERA                  "Device.IRCamera.Enable"

```

* Map the individual settings to the DFCI groups, an example DfciGroups.c is as follows:

``` c
STATIC DFCI_SETTING_ID_STRING mAllCameraSettings[] = {
    DEVICE_SETTING_ID__FRONT_CAMERA,
    DEVICE_SETTING_ID__REAR_CAMERA,
    DEVICE_SETTING_ID__IR_CAMERA,
    NULL
};
STATIC DFCI_SETTING_ID_STRING mAllCpuAndIoVirtSettings[] = {
    DEVICE_SETTING_ID__ENABLE_VIRT_SETTINGS,
    NULL
};

STATIC DFCI_GROUP_ENTRY mMyGroups[] = {
    { DFCI_SETTING_ID__ALL_CAMERAS,     (DFCI_SETTING_ID_STRING *) &mAllCameraSettings },
    { DFCI_SETTING_ID__ALL_AUDIO,       (DFCI_SETTING_ID_STRING *) &mAllAudioSettings },
    { DFCI_SETTING_ID__ALL_RADIOS,      (DFCI_SETTING_ID_STRING *) &mAllRadiosSettings },
    { DFCI_SETTING_ID__EXTERNAL_MEDIA,  (DFCI_SETTING_ID_STRING *) &mExternalMediaSettings },
    { DFCI_SETTING_ID__ENABLE_NETWORK,  (DFCI_SETTING_ID_STRING *) &mOnboardNetworkSettings },
    { DFCI_SETTING_ID__ALL_CPU_IO_VIRT, (DFCI_SETTING_ID_STRING *) &mAllCpuAndIoVirtSettings },
    { NULL,                             NULL }
};

/**
 * Return a pointer to the Group Array to DFCI
 *
 */
DFCI_GROUP_ENTRY *
EFIAPI
DfciGetGroupEntries (VOID) {

    return (DFCI_GROUP_ENTRY *) &mMyGroups;
}

```
