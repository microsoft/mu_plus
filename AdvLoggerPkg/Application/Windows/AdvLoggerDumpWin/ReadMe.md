# AdvLoggerPkg - AdvLoggerDumpWin

AdvLoggerDumpWin is used to retrieve the advanced UEFI debug log stored in system memory.
If the Advanced Logger is enabled, the debug log is stored in memory and is available through the UEFI
Variable store.

## About

This tool creates a Windows executable that writes the UEFI variable data to a binary log file. As the
log in memory has additional metadata and alignment structure, DecodeUefiLog.py (in AdvLoggerPkg)
parses the in-memory UefiLog to a text stream and writes the decoded log to a local file. The Python
script can be used to both retrieve the log and decode it.
AdvLoggerDumpWin is able to be used in systems without Python.

## Building

To build the project, install Visual Studio 2022 with the following workloads:

* Desktop development with C++
* Universal Windows Platform development

For individual components, ensure the following are installed (may be included with the above workloads):

* Windows 11 SDK
* NugGet package manager
* MSVC v143 - VS 2022 C++ (Your Architecture) build tools
* C++ ATL fot latest v143 build tools (Your Architecture)
* C++ MFC for latest v143 build tools (Your Architecture)

In Visual Studio, open the project solution file AdvLoggerPkg\Application\Windows\AdvLoggerDumpWin.sln

The packages.config should tell NuGet to install the Microsoft.Windows.CppWinRT package. If not, add nuget.org
as a package source and install the package.

Build the solution in Release mode. The executable will be in the Release folder for the given architecture.

## Usage

With administrator privileges, run the executable.
In an administrator command prompt:

```.sh
AdvLoggerDumpWin.exe
```

The program creates a new log file `new_logfile.bin` in the current directory.

## Copyright

Copyright (C) Microsoft Corporation. \
SPDX-License-Identifier: BSD-2-Clause-Patent
