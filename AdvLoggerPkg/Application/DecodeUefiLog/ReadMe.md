# AdvLoggerPkg - DecodeUefiLog

DecodeUefiLog is used to get the Uefi debug log that is stored by UEFI.

## About

Advanced Logger stores the debug log file in EfiReserved memory, with additional data for each line.
At runtime, if the Advanced Logger is enabled, this in memory log is available through the UEFI Variable store.
As the log in memory has additional metadata and alignment structure, DecodeUefiLog parses the UefiLog to a text stream and writes the log to a local file.

## Usage

Copy the two files DecideUefiLog.py and UefiVariablesSupportLib.py to the system with the Advanced Logger enabled.
The simplest case:

```
DecodeUefiLog -o NewLogFile.txt.
```
