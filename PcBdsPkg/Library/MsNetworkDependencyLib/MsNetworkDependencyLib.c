/** @file
 *NetworkDependencyLib.  Use this library if you are dependent on the network stack.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/MsNetworkCtlGuid.h>

#include <Protocol/DevicePath.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/TimerLib.h>

static EFI_HANDLE mHandle;

/**
MsNetworkDelay Constructor
*/
EFI_STATUS
EFIAPI
MsNetworkDependencyLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
) {

    mHandle = ImageHandle;
    return EFI_SUCCESS;
}


/**
Library function to enable the network stack
**/
EFI_STATUS
EFIAPI
StartNetworking() {
   EFI_STATUS   Status;
   VOID        *Interface;

   Status = gBS->LocateProtocol (&gMsNetworkDelayProtocolGuid,NULL, &Interface);
   if (EFI_NOT_FOUND == Status) {
       Status = gBS->InstallProtocolInterface (&mHandle,
                                              &gMsNetworkDelayProtocolGuid,
                                               EFI_NATIVE_INTERFACE,
                                               NULL);
       DEBUG((DEBUG_INFO, "%a Starting Network Stack\n", __FUNCTION__));
       EfiBootManagerConnectAll ();
       DEBUG((DEBUG_INFO, "%a Connecting done\n", __FUNCTION__));
   }
   return Status;
}


