/** @file
 *MsNetworkDelayLib.  Use this library as a NULL library attached to
 SnpDxe to prevent the network from starting if you are dependent on the network stack.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>

/**
MsNetworkDelay Constructor
*/
EFI_STATUS
EFIAPI
MsNetworkDelayLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
) {

    DEBUG ((DEBUG_INFO, "%a: Network Delay satisfied\n", __FUNCTION__));
    return EFI_SUCCESS;
}
