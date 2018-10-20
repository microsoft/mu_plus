/** @file
 *NetworkDependencyLib.  Use this library if you are dependent on the network stack.

Copyright (c) 2015 - 2018, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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


