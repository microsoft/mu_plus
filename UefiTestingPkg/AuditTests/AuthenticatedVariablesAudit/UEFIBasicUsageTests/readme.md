# UEFI Basic Usage Tests

These tests focus on the limits of UEFI private authenticated variables

Based on the code in Tianocore EDK2 as of 1/26/2023, the following basic functionality should be supported

|  Test Name              | Description                                                                         | Expectation |
|:------------------------|:-----------------------------------------------------------------------------------:|:------------|
| VariableKeyLengths      | Confirms UEFI supports:<br> 2k, 3k, and 4k crypto<br> Digest algorithms of SHA-256  | SUCCESS     |
| AdditionalCertificates  | Confirms UEFI supports:<br> Additional certificiates in the signature (1,2,3 add.)  | SUCCESS     |
| PreventUpdate           | Confirms UEFI Supports:<br> Preventing updates to variable data                     | SUCCESS     |
| PreventRollback         | Confirms UEFI Supports:<br> Preventing rollbacks to variable data                   | SUCCESS     |