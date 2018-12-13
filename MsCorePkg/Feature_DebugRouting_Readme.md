# Debugging with DxeDebugLibRouter
## Copyright

Copyright (c) 2018, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## About

### DxeDebugLibRouter

The DxeDebugLibRouter is an implementation of DebugLib that routes the DebugPrint and DebugAssert messages depending on what the platform is capable of and what has been set-up.

In the example below we show how to use DxeDebugLibRouter to route debug messages through either the serial interface or the report status code interface, depending on what protocols and libraries are being used.

### StatusCodeHandler

If you wish to make use of the Report Status Code debugging feature you will need to set up a status code handler and install the gMsSerialStatusCodeHandlerDxeProtocolGuid tag GUID. The MsCorePkg version of StatusCodeHandler is setup to do this.

### DebugPortProtocolInstallLib

The DebugPortProtocolInstallLib is a shim library whos only purpose is to install a protocol that points to the currently linked DebugLib being used by the module. You can see how this is used in the DSC example shown below.


### ReportStatusCodeRouter

This library handles the routing of ReportStatusCode if the DxeDebugLibRouter is set-up to use the ReportStatusCode debug path. We have only implemented the serial output for report status code, but there are many ways you can implement a RSC observer including


* Serial Port Listener
* Save Debug To File System
* Save Debug To Memory
* ...


---

## How To Use

To make full use of the DxeDebugLibRouter each Dxe Driver will need to use the DebugPort implementation of DebugLib to route their messages through the DxeDebugLibRouter. The Flowchart below shows how this would work.


To set up DxeCore to work as the router you will need to set up the DSC as below:
```
[LibraryClasses.X64]
DebugLib|MdePkg/Library/UefiDebugLibDebugPortProtocol/UefiDebugLibDebugPortProtocol.inf

[Components.X64]
MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      NULL|MsCorePkg/Library/DebugPortProtocolInstallLib/DebugPortProtocolInstallLib.inf
      DebugLib|MsCorePkg/Library/DxeDebugLibRouter/DxeDebugLibRouter.inf
  }
```

## Debug Flow

1: The NULL library responsible for publishing the DebugPort protocol is linked against DxeMain. This allows the DebugLib used by Dxe drivers to locate the DebugLib used by Dxe Main

2: Dxe Driver makes a DebugPrint which is routed to the DebugLib linked to DxeMain

3: DebugLib routes the DebugPrint through either Serial or Report Status Code depending on what is installed at the time

A: This step can happen at any time. When the StatusCodeHandler is dispatched it installs a tag GUID letting the DebugLib know that Report Status Code is now available



![](DebugOverview.mu.png)
