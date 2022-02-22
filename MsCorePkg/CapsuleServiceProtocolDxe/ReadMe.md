# Capsule Service Protocol DXE

This driver produces a protocol (gCapsuleServiceProtocolGuid) which can be used by the CapsuleRuntimeService.
There is a readme which details why a protocol was used in that folder (see `CapsuleRuntimeDxe`).

This protocol does not apply capsules. It combines CapsuleLib, PersistLib, and CapsuleRuntimeDxe.

It is significantly less configurable and is focused on reducing attack surface.

## Constraints Introduced in Comparison to the Traditional Capsule Runtime mechanism

- MRC does not preserve memory. We do a warm reset but make no guarantee that memory is preserved.
- Capsules will not be applied at runtime.
- All capsules will do a reset before applying.
- Option roms are not supported.
- Embedded Drivers inside of Capsules are not supported.

## Implementation

### Flow chart

Here is a flow chart that should explain the overall flow of the capsule system.

!["flowchart"](Images/CapsuleFlowchart.png)

The new system stores capsules on the EFI partition of the main disk.
While it shares many similarities to Capsule on Disk,
it is significantly more opinionated and has few modes of operation/configuration.
It has a few additional security features as well.

Capsules are staged by the OS by calling the Runtime service `UpdateCapsule`.
The capsule header is given a quick sanity check, hashed, given a unique ID, and saved to the EFI partition.
Once saved, a variable is created to keep track of the capsules which have been saved and are processed in a
first in, first out manner.

### Libraries

Two other pieces of capsule infrastructure are leveraged by this driver: IsCapsuleSupportedLib
and CapsulePersistenceLib.
These libraries are used to validate a capsule as well as manage the storage of capsules.
This driver also uses QueueLib to keep track of capsules that have been persisted.
Each library has a readme containing further information located in their respective folders.

## Further Notes

The first time a capsule is persisted, we clear the `Capsules` folder on the disk to remove stale files.

The *only* way to stage a capsule is by calling the Runtime service.
Currently there are no plans to support capsules that are placed manually in the `Capsules` folder.

## Next Boot Path

Before the system is reset, the variable `PreviousCapsuleCount` is set to zero.
This variable signals that capsules are present.
It is zero as it is a marker of the number of capsules on the last boot.
If we set it to the current number of capsules, we would detect it as no progress being made, and would abort.
