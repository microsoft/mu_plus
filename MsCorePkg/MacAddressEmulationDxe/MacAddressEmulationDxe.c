/** @file

  DXE Driver for handling MAC Address Emulation.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/NetLib.h>

#include <Protocol/SimpleNetwork.h>

#include "Library/MacAddressEmulationPlatformLib.h"


//
// Utility macro that splits EFI_MAC_ADDRESS into
// a set of CHAR8 arguments (e.g. for printing)
//
#define MAC_TO_CHAR8_ARGS(MacAddress) \
  ((UINT8 *)&MacAddress)[0], \
  ((UINT8 *)&MacAddress)[1], \
  ((UINT8 *)&MacAddress)[2], \
  ((UINT8 *)&MacAddress)[3], \
  ((UINT8 *)&MacAddress)[4], \
  ((UINT8 *)&MacAddress)[5]

typedef struct {
  VOID            *Registration;
  BOOLEAN         Assigned;
  EFI_MAC_ADDRESS EmulationAddress;
  EFI_MAC_ADDRESS PermanentAddress;
} MAC_EMULATION_SNP_NOTIFY_CONTEXT;


/**
  Update the MAC address for the SNP instance specified by SnpHandle

  @param[in]  SnpHandle - Controller handle corresponding to the controller on which the MAC address should be updated.
  @param[in]  Context   - Pointer to context.

  @retval  EFI_SUCCESS - MAC address successfully updated on the SNP instance installed on the specified handle.
  @retval  other       - Unexpected failure updating MAC address.

**/
EFI_STATUS
UpdateMacAddress (
  EFI_HANDLE SnpHandle,
  MAC_EMULATION_SNP_NOTIFY_CONTEXT *Context
  )
{
  EFI_STATUS                  Status;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;

  DEBUG ((DEBUG_INFO, "[%a]: Updating MAC address to %02x:%02x:%02x:%02x:%02x:%02x\n", __FUNCTION__, MAC_TO_CHAR8_ARGS(Context->EmulationAddress)));

  Status = gBS->HandleProtocol (SnpHandle, &gEfiSimpleNetworkProtocolGuid, (VOID **)&Snp);
  if (Status == EFI_SUCCESS) {
    // Make sure SNP is started
    if (Snp->Mode->State != EfiSimpleNetworkInitialized) {
      DEBUG ((DEBUG_ERROR, "[%a]: SNP handle in unexpected state %d, cannot update MAC.\n", __FUNCTION__, Snp->Mode->State));
      return EFI_NOT_READY;
    }

    // Make sure this is the right interface type (Ethernet) and that the adapter supports MAC address programming.
    if (Snp->Mode->IfType != NET_IFTYPE_ETHERNET) {
      DEBUG ((DEBUG_WARN, "[%a]: SNP interface type is not Ethernet.\n", __FUNCTION__));
      return EFI_UNSUPPORTED;
    }

    if (!Snp->Mode->MacAddressChangeable) {
      DEBUG ((DEBUG_WARN, "[%a]: SNP interface does not support MAC address programming", __FUNCTION__));
      return EFI_UNSUPPORTED;
    }

    // If emulation was already assigned, make sure that this is the same interface that was assigned previously
    // by comparing the permanent MAC address against the address cached during the first assignment (updated in
    // context below).
    if ((Context->Assigned) &&
        (CompareMem (&Snp->Mode->PermanentAddress, &Context->PermanentAddress, NET_ETHER_ADDR_LEN) != 0))
    {
      DEBUG ((DEBUG_WARN, "[%a]: Emulated MAC address already assigned, cannot update MAC.\n", __FUNCTION__));
      return EFI_NOT_READY;
    }

    // Attempt to set the MAC address.
    Status = Snp->StationAddress (Snp, FALSE, &Context->EmulationAddress);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a]: Failed to set MAC address on SNP interface. Status=%r\n", __FUNCTION__, Status));
    } else {
      // Update context to indicate that we've assigned the emulation to this particular device.
      // save the permanent address to facilitate the check above.
      CopyMem (&Context->PermanentAddress, &Snp->Mode->PermanentAddress, NET_ETHER_ADDR_LEN);
      Context->Assigned = TRUE;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "[%a]: Failed to retrive SNP interface. Status=%r\n", __FUNCTION__, Status));
  }

  return Status;
}

/**
  Callback that is invoked when an SNP instance is Initialized. Checks the newly installed SNP registrations (if any)
  and updates the MAC address if a supported adapter is found.

  @param[in]  Event   - The EFI_EVENT for this notify.
  @param[in]  Context - An instance of MAC_EMULATION_SNP_NOTIFY_CONTEXT.

  @retval  None

**/
VOID
EFIAPI
SimpleNetworkProtocolNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                        Status;
  MAC_EMULATION_SNP_NOTIFY_CONTEXT  *MacContext;
  UINTN                             HandleCount;
  UINTN                             HandleIdx;
  EFI_HANDLE                        *SnpHandleBuffer;

  if (Context == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a]: Context unexpectedly null.\n", __FUNCTION__));
    ASSERT (Context != NULL);
    return;
  }

  DEBUG ((DEBUG_INFO, "[%a]: Start\n", __FUNCTION__));

  MacContext = (MAC_EMULATION_SNP_NOTIFY_CONTEXT *)Context;

  // Iterate over all SimpleNetwork instances until we find one that is supported for emulation.
  HandleCount = 0;
  SnpHandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleNetworkProtocolGuid, NULL, &HandleCount, &SnpHandleBuffer);
  if (Status == EFI_NOT_FOUND) {
    // This should return at least one instance of SNP, since somewhere Snp->Initialize() was invoked to generate this
    // event. However, the event is also invoked on creation of the notify, and in that case there will be no SNP
    // instances. In any case, no SNP instances means nothing to do.
    return;
  } else if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a]: Unexpected error from LocateHandleBuffer. Status=%r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
    return;
  }

  for (HandleIdx=0; HandleIdx < HandleCount; HandleIdx++) {
    if (SnpSupportsMacEmulation (SnpHandleBuffer[HandleIdx])) {
      Status = UpdateMacAddress (SnpHandleBuffer[HandleIdx], MacContext);
      if (Status == EFI_SUCCESS) {
        DEBUG ((DEBUG_INFO, "[%a]: MAC Address on handle 0x%p successfully updated.\n", __FUNCTION__, SnpHandleBuffer[HandleIdx]));
        // If we assigned the address, we are done. But we do not close the event or free the context in case the Snp is
        // shutdown and later restarted.
        break;
      }
      // If we get here, then there was an issue updating the MAC address for this particular SNP instance.
      // This might not be an error (e.g. we assigned the address to another interface already), so continue.
      DEBUG ((DEBUG_WARN, "[%a]: Could not update MAC Address on handle 0x%p. Status=%r\n", __FUNCTION__, SnpHandleBuffer[HandleIdx], Status));
    } else {
      DEBUG ((DEBUG_INFO, "[%a]: SNP Handle 0x%p was not a supported controller for MAC emulation.\n", __FUNCTION__, SnpHandleBuffer[HandleIdx]));
    }
  }

  if (SnpHandleBuffer != NULL) {
    FreePool (SnpHandleBuffer);
  }
}

/**
  Driver entry:  Initializes MAC Address Emulation

  @param[in]  ImageHandle - Handle to this image.
  @param[in]  SystemTable - Pointer to the system table.

  @retval  EFI_STATUS code.

**/
EFI_STATUS
EFIAPI
MacAddressEmulationEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE* SystemTable
  )
{
  MAC_EMULATION_SNP_NOTIFY_CONTEXT *Context;
  EFI_MAC_ADDRESS Address;
  EFI_STATUS Status;

  // Determine general platform runtime support.  Return unsupported to fully unload driver if not enabled.
  Status = IsMacEmulationEnabled (&Address);
  if (EFI_ERROR(Status)) {
    if (Status != EFI_UNSUPPORTED) {
      DEBUG ((DEBUG_ERROR, "[%a]: Failed to determine MAC Emulated Address support. Status = %r\n", __FUNCTION__, Status));
    }
    return Status;
  }

  // Allocate and initialize context
  Context = AllocateZeroPool (sizeof (MAC_EMULATION_SNP_NOTIFY_CONTEXT));
  if (Context == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a]: cannot allocate notify context\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }
  Context->Assigned = FALSE;
  CopyMem (&Context->EmulationAddress, &Address, sizeof (EFI_MAC_ADDRESS));

  // Enable support for the high level OS driver to load support properly
  Status = PlatformMacEmulationEnable (&Address);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a]: Failed platform initialization of MAC Emulation. Status = %r\n", __FUNCTION__, Status));
    return Status;
  }

  // Set up a call back on Snp->Initialize() invocations.
  Status = EfiNamedEventListen (
             &gSnpNetworkInitializedEventGuid,
             TPL_CALLBACK,
             SimpleNetworkProtocolNotify,
             Context,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a]: Failed to initialize a SNP Network listen event. Status = %r\n", __FUNCTION__, Status));
    // Don't return error so the driver does not unload in case the PlatformMacEmulationEnable library call needed to install a callback
  }

  // Return success, support is ready
  return EFI_SUCCESS;
}