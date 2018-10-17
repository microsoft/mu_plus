# Device Firmware Configuration Interface (DFCI) 

!!! danger "BETA"
    Note that this feature is currently in **Beta**.  Interfaces and implementation may change.  

## About

The Device Firmware Configuration Interface (**DFCI**) is a standard for managing UEFI firmware settings.  It is designed to enable management by device owners who are authenticated via public key infrastructure (PKI).  DFCI provides multiple levels of delegated ownership, permissions, & PKI to enable a central device ownership authority to provision (and re-provision) to delegated owners as well as to provide multiple layers of recovery in case a delegated PKI either becomes unavailable or is compromised.  DFCI can be configured to enable Zero-touch UEFI Management (**ZUM**).  In this configuration, no consent prompt is presented to the physically-present user during enrollment of ownership authorities, as they are presumed to not be the device owner.  This enables practical, zero-touch, end-to-end management by [modern management solutions](https://en.wikipedia.org/wiki/List_of_Mobile_Device_Management_software), sometimes referred to as MDM, EMM, or UEM. 

Note that for management of servers in a datacenter, DFCI does not presume to be the solution.  [Redfish](https://www.dmtf.org/standards/redfish) may be a more suitable solution for this environment. 

### More content is in the works...
