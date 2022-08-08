# FHR Functional Test

Test support for the Firmware Hot Restart (FHR) feature. This application
tests the following.

1. The memory map provided on cold boot is compatible with FHR.
2. Patterns all OS reclaimable memory.
3. Initiates an FHR pointing back tot he test for resume.
4. Checks all provided information is persisted correctly.
5. Checks that memory pattern is still in place.
6. Repeat steps 3+ until reboot count is achieved.

## Parameters

__-nomemory__ : Skips the setting and checking on the memory pattern. This
significantly speeds up the test time, but will miss memory corruption issues.

__-fullpage__ : Applies the memory pattern to the entirety of every page rather
then to just the first 64 bits.
