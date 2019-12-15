-# MuVarPolicyFoundationDxe Driver and Policies
## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

## Overview

This driver works in conjunction with the Variable Policy engine to create two policy-based concepts that can be leveraged by other drivers in the system and to produce an EDK2 Variable Locking Protocol. The two concepts are: Dxe Phase Indicators and Write Once State Variables.

## DXE Phase Indicators

To support the DXE Phase Indicators, a new policy is installed that creates the `gMuVarPolicyDxePhaseGuid` variable namespace. By policy, all variables in this namespace are required to be the size of a `PHASE_INDICATOR` (which is a `UINT8`) and must be volatile and readable in both BootServices and Runtime. All variables in this namespace will also be made read-only immediately upon creation (as such, they are also Write-Once, but have a special purpose). Because these variables are required to be volatile, it will not be possible to create more after ExitBootServices.

This driver will also register callbacks for EndOfDxe, ReadyToBoot, and ExitBootServices. At the time of the corresponding event, a new `PHASE_INDICATOR` variable will be created for the callback that has just been triggered. The purpose of these variables is two-fold:

- They can be queried by any driver or library that needs to know what phases of boot have already occurred. This is especially convenient for libraries that might be linked against any type of driver or application, but may not have been able to register callbacks for all the events because they don't know their execution order or time.
- They can be used as the delegated "Variable State" variables in other Variable Policy lock policies. As such, you could describe a variable that locks "at ReadyToBoot" or "EndOfDxe".

Note that the `PHASE_INDICATOR` variables are intentionally named as short abbreviations, such as "EOD" and "RTB". This is to minimize the size of the policy entries for lock-on-state policies.

### Important Note on Timing

The EndOfDxe and ReadyToBoot state variables will be created at the **end** of the Notify list for the respective event. As such, any variable that locks on those events will still be writeable in Notify callbacks for the event. However, the ExitBootServices state variable (due to architectural requirements) will be created at a non-deterministic time in the Notify list. Therefore, it is unpredictable whether any variable which locks "on ExitBootServices" would still be writeable in any given callback.

## Write-Once State Variables

The Write-Once State Variables are actually quite similar to the DXE Phase Variables in terms of policy, but are kept distinct because of the different intentions for their use. This driver will also register a policy that creates the `gMuVarPolicyWriteOnceStateVarGuid` namespace. This policy will also limit these variables to being: volatile, BootServices and Runtime, read-only on create, and a fixed size -- `sizeof(POLICY_LOCK_VAR)`, which is also a `UINT8`, given that that is the size of the Value field of the current `VARIABLE_LOCK_ON_VAR_STATE_POLICY` structure.

The primary difference is that the DXE Phase Variables have a very clear purpose and meaning. While the `gMuVarPolicyWriteOnceStateVarGuid` namespace is designed to be general-purpose and used to easily describe delegated variables for `VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE`-type policies without requiring every driver to define two policies in order to employ this common pattern.

### Example

Driver XYZ wants to define a policy to allow a boot application (that executes after the policy creation interface may be closed) to modify a collection of variables until the boot application chooses to lock them. One way to accomplish this is the create a policy that delegates the lock control of the target variables to the state of variable A. However, now the variable A is the linchpin for the protections of the target variables and should have some protections of its own. This might require another policy.

Instead, the driver could create a policy that delegates lock control of the target variables to a variable in the `gMuVarPolicyWriteOnceStateVarGuid`. This would not require a second policy, as a general-purpose policy is already in place. Now the boot application only needs to create variable A in the `gMuVarPolicyWriteOnceStateVarGuid` namespace (with the value designated in the policy created by driver XYZ) in order to lock the target variables.

### Important Note on Naming

Since the `gMuVarPolicyWriteOnceStateVarGuid` namespace is a single variable namespace, naming collisions are possible. Because of the write-once nature of the policy, it is not a concern that an existing lock may be overwritten, but there still may be policy confusion if there are two policies that delegate to the exact same name. It is up to the platform architect to ensure that naming collisions like this do not occur.

### EDK2 Variable Locking Protocol

Finally, this driver installs an EDK2 Variable Locking Protocol instance. This implementation locks a variable by creating a policy entry and registering it via the Variable Policy Protocol. The policy entry is of type VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE and locks the variable based on state of the Phase Indicator variable "EOD" (meaning End Of Dxe) in `gMuVarPolicyDxePhaseGuid` namespace.
