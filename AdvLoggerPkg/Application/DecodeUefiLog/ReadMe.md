# AdvLoggerPkg - DecodeUefiLog

DecodeUefiLog is used to get the Uefi debug log that is stored by UEFI in system memory.

## About

Advanced Logger stores the debug log file in memory, with additional data for each line.
At runtime, if the Advanced Logger is enabled, this in memory log is available through the UEFI
Variable store.
As the log in memory has additional metadata and alignment structure, DecodeUefiLog parses the
in memory UefiLog to a text stream and writes the decoded log to a local file.

## Usage

Copy the two files, DecodeUefiLog.py and UefiVariablesSupportLib.py, to the system that has
the Advanced Logger enabled.

The simplest case:

```.sh
DecodeUefiLog -o NewLogFile.txt
```

Decode lines after a starting line number and send them to the file specified by -o:

```.sh
  DecodeUefiLog -s 5000 -o NewLogFile.txt
```

Copy the raw in memory log to a binary file.

```.sh
  DecodeUfieLog -r RawLog.bin
```

Decode a raw file into a text file:

```.sh
  DecodeUefiLog -l RawLog.bin -o NewLogFIle.txt
```

[-o Option] Decode lines and separate to multiple files by Kb size -size:

```.sh
  DecodeUefiLog -size 30 -o NewLogFile.txt
```

---

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent
