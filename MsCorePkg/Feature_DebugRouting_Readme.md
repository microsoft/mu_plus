# Debugging with DxeDebugLibRouter
## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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



![](DebugOverview_mu.png)
