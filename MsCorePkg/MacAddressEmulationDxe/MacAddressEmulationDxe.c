/** @file

  DXE Driver for handling MAC Address Emulation.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "MacAddressEmulationDxe.h"

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/NetLib.h>

#include <Library/MacAddressEmulationPlatformLib.h>

/**
  Performs sanity checks to ensure an snp can support mac emulation, and ensures that multiple interfaces are not programmed.

  @param[in] SnpHandle - A handle to an Snp
  @param[in] Snp - An snp protocol associated with the handle provided
  @param[in] SnpContext - The snp context created by this driver's entry point
  @retval TRUE - the SNP supports mac emulation and can be programmed with the emulated address
  @retval FALSE - the SNP should not be programmed with the emulated address

  @remark  typically called at TPL_NOTIFY
**/
BOOLEAN
SnpSupportsMacEmuCheck (
  IN CONST EFI_HANDLE                        SnpHandle,
  IN CONST EFI_SIMPLE_NETWORK_PROTOCOL       *Snp,
  IN CONST MAC_EMULATION_SNP_NOTIFY_CONTEXT  *Context
  )
{
  if ((SnpHandle == NULL) || (Snp == NULL) || (Context == NULL)) {
    return FALSE;
  }

  if (Snp->Mode->State != EfiSimpleNetworkInitialized) {
    DEBUG ((DEBUG_WARN, "[%a]: SNP handle in unexpected state %d, cannot update MAC.\n", __FUNCTION__, Snp->Mode->State));
    return FALSE
  }

  if (Snp->Mode->IfType != NET_IFTYPE_ETHERNET) {
    DEBUG ((DEBUG_WARN, "[%a]: SNP interface type is not Ethernet.\n", __FUNCTION__));
    return FALSE;
  }

  if (!Snp->Mode->MacAddressChangeable) {
    DEBUG ((DEBUG_WARN, "[%a]: SNP interface does not support MAC address programming", __FUNCTION__));
    return FALSE;
  }

  if (!PlatformMacEmulationSnpCheck (SnpHandle)) {
    DEBUG ((DEBUG_WARN, "[%a]: Platform library reports not to support this SNP", __FUNCTION__));
    return FALSE;
  }

  if (Context->Assigned == TRUE) {
    // If emulation was already assigned, make sure that this is the same interface that was assigned previously
    // by comparing the permanent MAC address against the address cached during the first assignment (updated in
    // context below).
    if (CompareMem (&Snp->Mode->PermanentAddress, &Context->PermanentAddress, NET_ETHER_ADDR_LEN) != 0) {
      DEBUG ((DEBUG_VERBOSE, "[%a]: Another SNP was already programmed (not this one) skipping.", __FUNCTION__));
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Iterates through all available SNPs available and finds the first instance which meets the criteria specified by match function

  @param[in] MatchFunctionContext - The snp context created by this driver's entry point
  @retval  NULL - no matching SNP was found, or invalid input parameter
  @retval  NON-NULL - a pointer to the first matching SNP

  @remark  typically called at TPL_NOTIFY
**/
EFI_SIMPLE_NETWORK_PROTOCOL *
FindMatchingSnp (
  IN MAC_EMULATION_SNP_NOTIFY_CONTEXT  *MatchFunctionContext
  )
{
  EFI_STATUS                   Status;
  UINTN                        HandleCount;
  UINTN                        HandleIdx;
  EFI_HANDLE                   *SnpHandleBuffer;
  EFI_SIMPLE_NETWORK_PROTOCOL  *SnpInstance;

  SnpInstance     = NULL;
  SnpHandleBuffer = NULL;

  DEBUG ((DEBUG_VERBOSE, "[%a]: Start\n", __FUNCTION__));

  if (MatchFunctionContext != NULL) {
    Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleNetworkProtocolGuid, NULL, &HandleCount, &SnpHandleBuffer);
    if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
      DEBUG ((DEBUG_ERROR, "[%a]: Unexpected error from LocateHandleBuffer. Status=%r\n", __FUNCTION__, Status));
      ASSERT_EFI_ERROR (Status);
    }
  } else {
    Status = EFI_INVALID_PARAMETER;
  }

  if (!EFI_ERROR (Status)) {
    for (HandleIdx = 0; HandleIdx < HandleCount; HandleIdx++) {
      Status = gBS->HandleProtocol (SnpHandleBuffer[HandleIdx], &gEfiSimpleNetworkProtocolGuid, (VOID **)&SnpInstance);

      if (!EFI_ERROR (Status) && SnpSupportsMacEmuCheck (SnpHandleBuffer[HandleIdx], SnpInstance, MatchFunctionContext)) {
        break;
      } else {
        SnpInstance = NULL;
      }
    }
  }

  if (SnpHandleBuffer != NULL) {
    FreePool (SnpHandleBuffer);
  }

  DEBUG ((DEBUG_VERBOSE, "[%a]: End\n", __FUNCTION__));

  return SnpInstance;
}

/**
  Sets the provided SNP's station address using the context information provided

  @param[in] Snp - Non-NULL pointer to an SNP which supports station address programming
  @param[in] Context - The snp context created by this driver's entry point

  @remark  Modifies the provided SNP's station address
  @remark  typically called at TPL_NOTIFY
**/
VOID
SetSnpMacViaContext (
  IN EFI_SIMPLE_NETWORK_PROTOCOL           *Snp,
  IN OUT MAC_EMULATION_SNP_NOTIFY_CONTEXT  *Context
  )
{
  EFI_STATUS  Status = EFI_NOT_STARTED;
  EFI_TPL     OldTpl;

  DEBUG ((DEBUG_VERBOSE, "[%a]: Start\n", __FUNCTION__));

  if ((Snp != NULL) && (Context != NULL)) {
    // At this point the SimpleNetworkProtocolNotify puts this at TPL_NOTIFY
    // to preempt a network stack running at TPL_CALLBACK level.
    // But since Snp->StationAddress sets the TPL to TPL_CALLBACK we need to
    // artificially lower to TPL_CALLBACK for this call to work as expected
    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    gBS->RestoreTPL (TPL_CALLBACK);

    Status = Snp->StationAddress (Snp, FALSE, &Context->EmulationAddress);

    gBS->RaiseTPL (TPL_HIGH_LEVEL);
    gBS->RestoreTPL (OldTpl);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a]: Failed to set MAC address on SNP interface. Status=%r\n", __FUNCTION__, Status));
  } else {
    // Update context to indicate that we've assigned the emulation to this particular device.
    // save the permanent address to facilitate the check above.
    CopyMem (&Context->PermanentAddress, &Snp->Mode->PermanentAddress, NET_ETHER_ADDR_LEN);
    Context->Assigned = TRUE;
  }

  DEBUG ((DEBUG_VERBOSE, "[%a]: End\n", __FUNCTION__));
}

/**
  Callback that is invoked when an SNP instance is Initialized. Checks the newly installed SNP registrations (if any)
  and updates the MAC address if a supported adapter is found.

  @param[in]  Event   - The EFI_EVENT for this notify.
  @param[in]  Context - An instance of MAC_EMULATION_SNP_NOTIFY_CONTEXT.

  @remark  This callback should be at least 1 level higher TPL than network stack to ensure timely mac programming
           or network stack start should be delayed until later.
**/
VOID
EFIAPI
SimpleNetworkProtocolNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  MAC_EMULATION_SNP_NOTIFY_CONTEXT  *MacContext;
  EFI_SIMPLE_NETWORK_PROTOCOL       *SnpToConfigureEmu;

  DEBUG ((DEBUG_VERBOSE, "[%a]: Start\n", __FUNCTION__));

  if (Context == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a]: Context unexpectedly null.\n", __FUNCTION__));
    ASSERT (Context != NULL);
    return;
  }

  MacContext = (MAC_EMULATION_SNP_NOTIFY_CONTEXT *)Context;

  SnpToConfigureEmu = FindMatchingSnp (MacContext);

  SetSnpMacViaContext (SnpToConfigureEmu, MacContext);

  DEBUG ((DEBUG_VERBOSE, "[%a]: End\n", __FUNCTION__));
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
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  MAC_EMULATION_SNP_NOTIFY_CONTEXT  *Context;
  EFI_MAC_ADDRESS                   Address;
  EFI_STATUS                        Status;

  // Determine general platform runtime support.  Return unsupported to fully unload driver if not enabled.
  Status = GetMacEmulationAddress (&Address);
  if (EFI_ERROR (Status)) {
    if (Status != EFI_UNSUPPORTED) {
      DEBUG ((DEBUG_ERROR, "[%a]: Failed to determine MAC Emulated Address support. Status = %r\n", __FUNCTION__, Status));
    }

    return Status;
  }

  // Allocate and initialize context
  Context = AllocateZeroPool (sizeof (*Context));
  if (Context == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a]: cannot allocate notify context\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  Context->Assigned = FALSE;
  CopyMem (&Context->EmulationAddress, &Address, sizeof (Context->EmulationAddress));

  // Enable support for the high level OS driver to load support properly
  Status = PlatformMacEmulationEnable (&Address);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a]: Failed platform initialization of MAC Emulation. Status = %r\n", __FUNCTION__, Status));
    return Status;
  }

  // Set up a call back on Snp->Initialize() invocations.
  Status = EfiNamedEventListen (
             &gSnpNetworkInitializedEventGuid,
             TPL_NOTIFY,
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
