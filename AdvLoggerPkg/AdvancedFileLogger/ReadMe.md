# Advanced File Logger

## About

The Advanced File Logger monitors for file systems mounted during boot.
When an eligible file system is detected, the log is flushed to the file system.
The log is flushed if the system is reset during POST, and at Exit Boot Services.

An eligible file system is one with a `UefiLogs` directory in the root of the file system.
If no log files are present, the Advanced File Logger will create a log index file which
contains the index of the last log file written, and nine log files each PcdAdvancedLoggerPages in size.
These files are pre allocated at one time to reduce interference with other users of the filesystem.

To enable the Advanced File Logger, the following change is needed in the .dsc:

```inf
[Components.<ArchOfDXE>]
  AdvLoggerPkg/AdvancedFileLogger/AdvancedFileLogger.inf
```

and the follow change is needed in the .fdf:

```inf
[Components.FV.<YourFvDXE>]
  INF AdvLoggerPkg/AdvancedFileLogger/AdvancedFileLogger.inf
```

## Driver Usage Configuration

This section describes the configuration options of the Advanced File Logger driver.

### Advanced file logger enforcement

The Advanced File Logger can be configured to enforce the storing of memory logs when setting
`PcdAdvancedFileLoggerForceEnable` to `TRUE`.

### Advanced file logger enlightenment

The Advanced File Logger can be set to store memory logs with a `UefiLogs` directory in the ESP partition.
Without such a directory **AND** the enforcement is not enabled, the Advanced File Logger will not store
memory logs.

### Advanced file logger enablement through Policy

The Advanced File Logger can be configured to be enabled through policy service by producing a policy under[`gAdvancedFileLoggerPolicyGuid`](../Include/Guid/AdvancedFileLoggerPolicy.h)
and setting the `FileLoggerEnable` field to `TRUE`. For the sake of backwards compatibility, if a platform does not
produce such policy or not support policy services at all, the Advanced File Logger will default to be enabled.

Note: The above enablement, enforcement and enlightenment are in serial, the general pseudo code is as follows:

```c
  if (PolicyNotFound) {
    Enabled = TRUE;
  } else {
    Enabled = Policy.FileLoggerEnable;
  }

  if (Enable) {
    if (PcdGet (PcdAdvancedFileLoggerForceEnable)) {
      StoreLogs = TRUE;
    } else if (DirectoryExists (L"UefiLogs")) {
      StoreLogs = TRUE;
    } else {
      StoreLogs = FALSE;
    }
  }

  if (StoreLogs) {
    // Store logs
    // The UEFI_Index.txt file will indicate the last log file written
    StoreLogs ("UefiLogs/Log#.txt");
  }
```

### Advanced file logger Flush events

The advanced file logger can be flushed or configured to flush on the following events:

| Event | Description |
| --- | --- |
| `gEfiEventReadyToBootGuid` | By setting `BIT0` of `PcdAdvancedFileLoggerFlush`, logger will flushed to ESP at "Ready To Boot" event |
| `gEfiEventExitBootServicesGuid` | By setting `BIT1` of `PcdAdvancedFileLoggerFlush`, logger will flushed to ESP at "Exit Boot Services" event |
| System Reset | This will always be enabled as long as the system supports `gEdkiiPlatformSpecificResetFilterProtocolGuid` |

---

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent
