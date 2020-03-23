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

The NetworkDependencyLib implements a StartNetwork() interface that will publish the NetworkDelay protocol.

The future goal of this functionality is to have the EFI_BOOT_MANAGER_POLICY.ConnectDeviceClass() function be overridden and insert a call to DeviceDependencyLib.StartNetwork() when a request is made to start the network class.

You nay see other references to NetworkDependencyLib as the conversion to using EFI_BOOT_MANAGER_POLICY.ConnectDeviceClass() is not complete.


## Copyright
Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
