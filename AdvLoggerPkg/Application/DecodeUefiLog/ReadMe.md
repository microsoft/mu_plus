# AdvLoggerPkg - DecodeUefiLog

DecodeUefiLog is used to get the Uefi debug log that is stored by UEFI.

## About

Advanced Logger stores the debug log file in memory, with additional data for each line.
At runtime, if the Advanced Logger is enabled, this in memory log is available through the UEFI Variable store.
As the log in memory has additional metadata and alignment structure, DecodeUefiLog parses the on memory UefiLog to a text stream and writes the decoded log to a local file.

## Usage

Copy the two files DecideUefiLog.py and UefiVariablesSupportLib.py to the system with Advanced Logger enabled.

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
