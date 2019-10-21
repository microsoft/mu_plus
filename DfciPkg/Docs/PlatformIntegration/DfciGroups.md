# DFCI Groups

DFCI Groups allow numerous like typed settings to be managed together.  Groups can also be used to provide multiple names for the same setting.  This allows actual device settings to be mapped into a different namespaces for settings.  

One such example from the Microsoft scenario is `Dfci.OnboardCameras.Enable`.  This group setting is used to manage the state of all onboard cameras and gives the management entity an ability to control all cameras regardless of how many each platform has.  A platform may have `Device.FrontCamera.Enable` and `Device.RearCamera.Enable` settings.  Adding those settings to the group `Dfci.OnboardCameras.Enable` allows a general purpose management entity to control both.  

## Handling State Reporting

If all of the settings of the group have the same value then that value will be returned (ie `Enabled` or `Disabled` for an Enable type setting).  If they are not the same then `Inconsistent` will be returned. `Unknown` will be returned if there are no members in a group.

## Restrictions on group names

Group names and settings names are in the same name space and duplicate names are not allowed. Like settings names, group names are limited to 96 characters in length, and are null terminated CHAR8 strings.

## DfciGroupLib

The DfciGroupLib is how groups are managed. This library separates the grouping configuration from the setting providers to allow better flexibility and better maintainability.  

**DfciGroupLib** is defined in the **DfciPkg** located in **mu_plus** repository (https://github.com/microsoft/mu_plus/). 

## Library Interfaces

| Interfaces | Usage |
| ----- | ----- |
| DfciGetGroupEntries | DfciGetGroupEntries returns an array of groups, and each group points to a list of settings that are members of the group. |
