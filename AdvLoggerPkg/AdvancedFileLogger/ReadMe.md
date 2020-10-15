# Advanced File Logger

## About

The Advanced File Logger monitors for file systems mounted during boot.
When an eligible file system is detected, the log is flushed to the file system.
The log is flushed if the system is reset during POST, and at Exit Boot Services.

An eligible file system is one with a Logs directory in the root of the file system.
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

---

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent
