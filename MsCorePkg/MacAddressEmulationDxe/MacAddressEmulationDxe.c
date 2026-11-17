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

LIST_ENTRY  mSnpContexts = INITIALIZE_LIST_HEAD_VARIABLE (mSnpContexts);

/**
  Performs sanity checks to ensure an snp can support mac emulation, and ensures that multiple interfaces are not programmed.

  @param[in] SnpHandle - A handle to an Snp
  @param[in] Snp - An snp protocol associated with the handle provided
  @param[in] SnpContext - The snp context created by this driver's entry point
  @retval TRUE - the SNP supports mac emulation and can be programmed with the emulated address
  @retval FALSE - the SNP should not be programmed with the emulated address

  @remark  Called at TPL_CALLBACK.
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
    return FALSE;
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
  Sets the provided SNP's station address using the context information provided

  @param[in] Snp - Non-NULL pointer to an SNP which supports station address programming
  @param[in] Context - The snp context created by this driver's entry point

  @remark  Modifies the provided SNP's station address
  @remark  Called at TPL_CALLBACK.
**/
VOID
SetSnpMacViaContext (
  IN EFI_SIMPLE_NETWORK_PROTOCOL           *Snp,
  IN OUT MAC_EMULATION_SNP_NOTIFY_CONTEXT  *Context
  )
{
  EFI_STATUS  Status = EFI_NOT_STARTED;

  DEBUG ((DEBUG_VERBOSE, "[%a]: Start\n", __FUNCTION__));

  if ((Snp != NULL) && (Context != NULL)) {
    Status = Snp->StationAddress (Snp, FALSE, &Context->EmulationAddress);
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
  Find the saved Initialize method for an SNP interface.

  @param[in] Snp  The SNP interface to find.

  @return The wrapper context, or NULL if the interface is not wrapped.
**/
static
MAC_EMULATION_SNP_CONTEXT *
FindSnpContext (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *Snp
  )
{
  LIST_ENTRY                 *Link;
  MAC_EMULATION_SNP_CONTEXT  *SnpContext;

  if (Snp == NULL) {
    ASSERT (Snp != NULL);
    return NULL;
  }

  for (Link = GetFirstNode (&mSnpContexts); !IsNull (&mSnpContexts, Link); Link = GetNextNode (&mSnpContexts, Link)) {
    SnpContext = BASE_CR (Link, MAC_EMULATION_SNP_CONTEXT, Link);
    if (SnpContext->Snp == Snp) {
      return SnpContext;
    }
  }

  return NULL;
}

/**
  Initialize SNP and apply MAC emulation while stack callbacks remain masked.
**/
EFI_STATUS
EFIAPI
MacEmulationSnpInitialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN UINTN                        ExtraRxBufferSize OPTIONAL,
  IN UINTN                        ExtraTxBufferSize OPTIONAL
  )
{
  MAC_EMULATION_SNP_CONTEXT  *SnpContext;
  EFI_STATUS                 Status;
  EFI_TPL                    OldTpl;

  OldTpl     = gBS->RaiseTPL (TPL_CALLBACK);
  SnpContext = FindSnpContext (This);
  if (SnpContext == NULL) {
    Status = EFI_INVALID_PARAMETER;
  } else {
    Status = SnpContext->OriginalInitialize (This, ExtraRxBufferSize, ExtraTxBufferSize);
    if (!EFI_ERROR (Status) && SnpSupportsMacEmuCheck (SnpContext->Handle, This, SnpContext->MacContext)) {
      SetSnpMacViaContext (This, SnpContext->MacContext);
    }
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Wrap Initialize on installed SNP instances. No SNP methods run at this TPL.
**/
VOID
EFIAPI
SimpleNetworkProtocolNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                   Status;
  UINTN                        HandleCount;
  UINTN                        HandleIdx;
  EFI_HANDLE                   *Handles;
  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp;
  MAC_EMULATION_SNP_CONTEXT    *SnpContext;

  if (Context == NULL) {
    return;
  }

  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleNetworkProtocolGuid, NULL, &HandleCount, &Handles);
  if (EFI_ERROR (Status)) {
    return;
  }

  for (HandleIdx = 0; HandleIdx < HandleCount; HandleIdx++) {
    Status = gBS->HandleProtocol (Handles[HandleIdx], &gEfiSimpleNetworkProtocolGuid, (VOID **)&Snp);
    if (EFI_ERROR (Status) || (Snp->Initialize == MacEmulationSnpInitialize)) {
      continue;
    }

    SnpContext = FindSnpContext (Snp);
    if (SnpContext == NULL) {
      SnpContext = AllocateZeroPool (sizeof (*SnpContext));
      if (SnpContext == NULL) {
        DEBUG ((DEBUG_ERROR, "[%a]: Cannot allocate SNP wrapper context.\n", __FUNCTION__));
        continue;
      }

      InsertTailList (&mSnpContexts, &SnpContext->Link);
    }

    SnpContext->Handle             = Handles[HandleIdx];
    SnpContext->Snp                = Snp;
    SnpContext->OriginalInitialize = Snp->Initialize;
    SnpContext->MacContext         = Context;
    Snp->Initialize                = MacEmulationSnpInitialize;
  }

  FreePool (Handles);
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
  EFI_EVENT                         Event;
  VOID                              *Registration;

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

  Event = EfiCreateProtocolNotifyEvent (
            &gEfiSimpleNetworkProtocolGuid,
            TPL_NOTIFY,
            SimpleNetworkProtocolNotify,
            Context,
            &Registration
            );
  if (Event == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a]: Failed to register SNP protocol notification.\n", __FUNCTION__));
    // Don't return error so the driver does not unload in case the PlatformMacEmulationEnable library call needed to install a callback
  }

  // Return success, support is ready
  return EFI_SUCCESS;
}
