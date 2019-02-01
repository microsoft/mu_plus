# MS Network DependencyLib and NetworkDelayLib

## About
These libraries implement a method to disable the network stack unless there is a reason to use the network.  Not starting the network improves boot performance and causes fewer issues during manufacturing.


## How to use these libraries
Add the NetworkDelayLib as a NULL library reference.  All this library does is introduce a [Depex] on a NetworkDelay protocol.  

```
  MdeModulePkg/Universal/Network/SnpDxe/SnpDxe.inf {
    <LibraryClasses>
      NULL|PcBdsPkg/Library/MsNetworkDelayLib/MsNetworkDelayLib.inf
  }
```

The NetworkDependencylib implements a StartNetwork() interface that will publish the NetworkDelay protocol.  

The future goal of this functionality is to have the EFI_BOOT_MANAGER_POLICY.ConnectDeviceClass() function be overriden and insert a call to DeviceDependencyLib.StartNetwork() when a request is made to start the network class.

You nay see other references to NetworkDependencyLib as the conversion to using EFI_BOOT_MANAGER_POLICY.ConnectDeviceClass() is not complete.


## Copyright
Copyright (c) 2019, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
