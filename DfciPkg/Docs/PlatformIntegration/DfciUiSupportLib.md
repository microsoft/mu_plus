# DfciUiSupportLib

DfciUiSupportLib allows DFCI to communicate with the user during DFCI initialization, enrollment, or to indicate a non secure environment is available.

## Interfaces

| Interface | Usage |
| --- | --- |
| DfciUiIsManufacturingMode | Returns TRUE or FALSE.  Used to self OptIn a cert used for zero touch enrollment. If the device doesn't support a manufacturing mode, return FALSE. |
| DfciUiIsAvailable | For one touch enrollment, the user has to authorize the enrollment.  Since DFCI normally runs before consoles are started, DFCI will wait until END_OF_DXE and then make sure the User Interface (UI) is available.  If no UI is available at that time, the enrollment will fail. |
| DfciUiDisplayMessageBox | Displays a message box |
| DfciUiDisplayPasswordDialog | Displays a prompt for the ADMIN password |
| DfciUiDisplayAuthDialog | Displays a prompt for confirmation of an enrolling certificate.  The response is the last two characters of the thumbprint. If there is an ADMIN password set, then this dialog will also request the ADMIN password. |
| DfciUiExitSecurityBoundary | The platform settings application usually runs in a secure state before variables are locked.  The DFCI Menu application will call ExitSecurityBoundary before starting the network or performing USB operations to minimize the security risks associated with external accesses. |
