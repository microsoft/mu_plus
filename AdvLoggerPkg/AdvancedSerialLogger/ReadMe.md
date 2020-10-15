# Advanced Serial Logger

## About

The Advanced Serial Logger starts, the memory log is flushed to the serial port.
As more log is appended, the serial logger flushes it out to the serial port.

To enable the Advanced Serial Logger, the following change is needed in the .dsc:

```inf
[Components.<ArchOfDXE>]
  AdvLoggerPkg/AdvancedSerialLogger/AdvancedSerialLogger.inf
```

and the follow change is needed in the .fdf:

```inf
[Components.FV.<YourFvDXE>]
  INF AdvLoggerPkg/AdvancedSerialLogger/AdvancedSerialLogger.inf
```

---

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent
