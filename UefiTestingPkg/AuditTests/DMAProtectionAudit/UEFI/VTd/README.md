# DMAR Table Audit

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

## About

**DMAProtectionUnitTestApp.c**

Shell based UEFI unit test based off of the MsUnitTestPkg that test for:
1.  Intel VTd global status register shows VTd enabled
2.  All RMRR regions are set as EfiReservedMemoryType
3.  Bus mastering enabled (BME) is disabled on ExitBootServices. Because we can no longer write to file after ExitBootServices a variable is used to store the test state and the machine.

Note: this unit test requires a restart to finish its testing. If you plan to use this unit test in automation make sure to set up your startup.nsh script properly.
