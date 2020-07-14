# AdvLoggerPkg - Advanced Logger Package

## About

This package contains the various libraries to have in memory logging.

## Configuration

The following configurations are supported:

| Phase | Usage |
| --- | --- |
|DXE Only|Uses DxeCore, DxeRuntime, and Dxe AdvancedLoggerLib libraries for logging from start of DXE CORE through Exit Boot Services.  Accepts the PEI Advanced Logger Hob if one is generated.  Produces the AdvancedLogger protocol.|
|DXE+SMM|Requires DXE modules above, and adds the Smm AdvancedLoggerLib library.  Collects SMM generated messages in the in memory log|
|PEI|Uses PeiCore and Pei AdvancedLoggerLib libraries.  Creates the Advanced Logger Hob if PcdAdvancedLoggerPeiInRAM is set.|
|SEC|Uses the Sec Advanced Logger Library. SEC requires a fixed load address, so it piggy backs on the Temporary RAM PCD information.  Produces a Fixed Address temporary RAM log.  When memory is added, the Sec Advanced Logger library converts the Temporary RAM logging information to the PEI Advanced Logger Hob.|
|PEI64|Uses Pei64 Advanced Logger Library. Requires the SEC fixed address temporary log information in order to log Pei64 bit DEBUG messages.|

PCD's used by Advanced Logger

| PCD | Function of the PCD|
| --- | --- |
|PcdAdvancedLoggerForceEnable|The default operation is to check if a Logs directory is present in the root of the filesystem.  If the logs directory is present, logging is enabled. When PcdAdvancedLoggerForceEnable is TRUE, and the device is not a USB device, a Logs directory will be created and logging is enabled.  When logging is enabled, the proper log files will be created if not already preset.|
|PcdAdvancedLoggerPeiInRAM|For system that have memory at PeiCore entry. The full in memory log buffer if PcdAdvancedLoggerPages is allocated in the Pei Core constructor and PcdAdvancedLoggerPreMemPages is ignored.|
|PcdAdvancedSerialLoggerDebugPrintErrorLevel|The standard debug flags filter which log messages are produced.  This PCD allow a subset of log messages to be forwarded to the Serial Port Lib.|
|PcdAdvancedSerialLoggerDisable|Specifies when to disable writing to the serial port.|
|PcdAdvancedLoggerPreMemPages|Amount of temporary RAM used for the debug log.|
|PcdAdvancedLoggerPages|Amount of system RAM used for the debug log|
|PcdAdvancedLoggerLocator|When enabled, the AdvLogger creates a variable "AdvLoggerLocator" with the address of the LoggerInfo buffer|

# Libraries

The following libraries are used with AdvancedLogger:

| Library | Function of the Library |
| ---| --- |
  AdvancedLoggerAccessLib | Used to access the memory log - used by FileLogger and Serial/Dxe/Logger |
| AdvancedLoggerLib | One per module type - used to provide access to the in memory log buffer |
| AdvLoggerSmmAccessLib | Used to intercept GetVariable in order to provide an OS utility the ability to read the log |
| BaseDebugLibAdvancedLogger | Basic Dxe etc DebugLib |
| DebugAgent | Used to intercept SEC initialization |
| PeiDebugLibAdvancedLogger | Basic Pei DebugLib |

# Platform note:

The SEC version of the Advanced Logger uses the temporary RAM block. This block is fixed in size and location, and these need to be adjusted to make room for the Advanced Logger buffer.  There may be cases where the processor cache size is too small to enable the Advanced Logger during SEC.

The following changes are needed in the .dsc

```yaml
[LibraryClasses.common]
  DebugLib|AdvLoggerPkg/Library/BaseDebugLibAdvancedLogger/BaseDebugLibAdvancedLogger.inf

[LibraryClasses.IA32.SEC]
  AdvancedLoggerLib|AdvLoggerPkg/Library/AdvancedLoggerLib/Sec/AdvancedLoggerLib.inf
  DebugAgentLib|AdvLoggerPkg/Library/DebugAgent/Sec/AdvancedLoggerSecDebugAgent.inf

[LibraryClasses.IA32.PEI_CORE]
  AdvancedLoggerLib|AdvLoggerPkg/Library/AdvancedLoggerLib/PeiCore/AdvancedLoggerLib.inf

[LibraryClasses.IA32.PEIM]
  DebugLib|AdvLoggerPkg/Library/PeiDebugLibAdvancedLogger/PeiDebugLibAdvancedLogger.inf

[LibraryClasses.X64.PEIM]
  AdvancedLoggerLib|AdvLoggerPkg/Library/AdvancedLoggerLib/Pei64/AdvancedLoggerLib.inf

[LibraryClasses.X64]
  AdvancedLoggerLib|AdvLoggerPkg/Library/AdvancedLoggerLib/Dxe/AdvancedLoggerLib.inf
  AdvancedLoggerAccessLib|AdvLoggerPkg/Library/AdvancedLoggerAccessLib/AdvancedLoggerAccessLib.inf

[LibraryClasses.X64.DXE_CORE]
  AdvancedLoggerLib|AdvLoggerPkg/Library/AdvancedLoggerLib/DxeCore/AdvancedLoggerLib.inf

[LibraryClasses.X64.DXE_SMM_DRIVER]
  AdvancedLoggerLib|AdvLoggerPkg/Library/AdvancedLoggerLib/Smm/AdvancedLoggerLib.inf

[LibraryClasses.X64.SMM_CORE]
  AdvancedLoggerLib|AdvLoggerPkg/Library/AdvancedLoggerLib/Smm/AdvancedLoggerLib.inf

[LibraryClasses.X64.DXE_RUNTIME_DRIVER]
  AdvancedLoggerLib|AdvLoggerPkg/Library/AdvancedLoggerLib/Runtime/AdvancedLoggerLib.inf

[PcdsFeatureFlag]
## Build Example if you build environment differentiates customer builds from internal test builds
!if $(SHIP_MODE) == FALSE
  gAdvLoggerPkgTokenSpaceGuid.PcdAdvancedFileLoggerForceEnable|TRUE
  gAdvLoggerPkgTokenSpaceGuid.PcdAdvancedFileLoggerLocator|TRUE
!else
  gAdvLoggerPkgTokenSpaceGuid.PcdAdvancedFileLoggerForceEnable|FALSE
  gAdvLoggerPkgTokenSpaceGuid.PcdAdvancedFileLoggerLocator|TRUE
!endif
```

The following changes should be in the family .dsc where the processor specific changes are specified

```yaml
[PcdsFixedAtBuild.common]
  gAdvLoggerPkgTokenSpaceGuid.PcdAdvancedLoggerPreMemPages|24
```

## Advanced File Logger

The Advanced File Logger monitors for file systems mounted during boot.  When an eligible file system is detected, the log is flushed to the file system.  The log is flushed if the system is reset during POST, and at Exit Boot Services.

An eligible file system is one with a Logs directory in the root of the file system.  If no log files are present, the Advanced File Logger will create a log index file which contains the index of the last log file written, and nine log files each PcdAdvancedLoggerPages in size.  These files are pre allocated at one time to reduce interference with other users of the filesystem.

To enable the Advanced File Logger, the following change is needed in the .dsc:

```yaml
[Components.ArchOfDXE]
  AdvLoggerPkg/AdvancedFileLogger/AdvancedFileLogger.inf
```

and the follow change is needed in the .fdf:

```yaml
[Components.FV.YourFvDXE]
  INF AdvLoggerPkg/AdvancedFileLogger/AdvancedFileLogger.inf
```

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
