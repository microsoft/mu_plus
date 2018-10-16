# Identity and Authentication Manager

Basic overview of the IdentityAndAuthManager module.  

## File Overview

### IdentityAndAuthManager.H

* Private header file defining private functions for use across module
* Define the internal structure that holds the auth handle to identity mapping

### IdentityAndAuthManagerDxe

* Implement the Dxe specific parts of this.  Including:
  * Event handling
  * Protocol access
  * Protocol installation

### AuthManager.C

* Provide the implementation for the auth protocol functions

### AuthManagerProvision.C

* Support using Variable to set, change, or remove the AuthManager Key based identities

### AuthManagerProvisionedData.C

* Support NV storage of Provisioned Data.  This manages loading internal store and saving changes to internal store.
* This differs from the Provision.c file in that this has nothing to do with User input or applying user changes.  This is internal to the module only.  

### IdentityManager.C

* Support the get identity functionality
* Dispose Auth Handle
* Private 
  * Identity / auth token map management (Add, Free, Find)
* Add security *TODO*

### IdentityAndAuthManagerDxe.INF

* Dxe Module inf file 

### DfciAuthentication.h  PUBLIC HEADER FILE

* Defines the DXE protocol to access Identity and Auth management
