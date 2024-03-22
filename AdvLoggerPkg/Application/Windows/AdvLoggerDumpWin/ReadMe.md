# AdvLoggerPkg - AdvLoggerDumpWin
AdvLoggerDumpWin is used to retrieve the advanced UEFI debug log stored in system memory.

## About
If the Advanced Logger is enabled, the debug log is stored in memory and is available through the UEFI
Variable store.
This tool creates a Windows executable that writes the UEFI variable data to a binary log file. As the 
log in memory has additional metadata and alignment structure, DecodeUefiLog.py (in AdvLoggerPkg) 
parses the in-memory UefiLog to a text stream and writes the decoded log to a local file. The Python
script can be used to both retrieve the log and decode it. 
AdvLoggerDumpWin is able to be used in systems without Python.

## Usage:
With administrator privileges, run the executable. 
In an administrator command prompt:
```
AdvLoggerDumpWin.exe
```
The program creates a new log file `new_logfile.bin` in the current directory.


## Building
To build the project, open the solution file in Visual Studio.
Ensure you have the necessary dependencies.
	* Microsoft.Windows.CppWinRT package
	* Windows SDK
	* VS build tools


## Copyright
Copyright (C) Microsoft Corporation. All rights reserved.  
SPDX-License-Identifier: BSD-2-Clause-Patent