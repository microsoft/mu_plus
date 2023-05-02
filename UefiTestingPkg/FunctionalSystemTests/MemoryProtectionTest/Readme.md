# Memory Protection Test App

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

## About This Test

This test verifies the memory protection features of Project Mu. The test checks the
memory protection policy of the system and adds a test for each memory type of each
protection type active. For example, if the system protection policy has NX protection
on EfiBootServicesData, the test will allocate a buffer of that type and check the
attributes of that buffer either by attempting to execute code within it or by fetching
the attributes using the memory attribute protocol. The test has a few methods of running.

## Testing using Resets

This method is the slowest, reaching upwards of 45 minutes with a strict protection policy
(benchmarked on a Q35 DEBUG build). The test will intentionally cause faults for each
protection policy and use a custom installed fault handler for IA32_PAGE_FAULT exceptions
which simply performs a warm reset on a fault. This method can be run on x86 based platforms.
Currently, the test doesn't install a synchronous handler for initiating a reset on ARM
platforms.

## Testing by Clearing Faults

This method takes less than 5 seconds to run. The test will intentionally cause faults for
each protection policy with the expectation that the Project Mu fault handler will clear the
fault and allow the test to continue. Project Mu currently only supports this method of
running the test on x86 based platforms.

## Testing by using the Memory Attribute Protocol

This method takes less than 5 seconds to run. Instead of intentionally causing faults,
this method checks the attributes of the region which should fault against the expected
protection attributes. For example, instead of intentionally causing a stack overflow to
check the stack guard, the test will find the stack base and check that the attributes
are EFI_MEMORY_RP using the Memory Attribute Protocol (because the page at the stack base
is a guard page marked EFI_MEMORY_RP). This method can be run on either X64 or
ARM64 platforms.

## Running the Test

The test is run from the shell using the command `MemoryProtectionTestApp.efi`. By default,
the test will try to run using the Memory Attribute Protocol method, then the Clearing
Faults method, then the warm reset method. If none of these methods are available, the
test will fail. The test can be run using a specific method by passing in one of the
following arguments:

'--Reset'

'--ClearFaults'

'--MemoryAttribute'
