/** @file

  DXE Driver header for handling MAC Address Emulation.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MAC_ADDRESS_EMULATION_DXE_H_
#define _MAC_ADDRESS_EMULATION_DXE_H_

#include <Uefi.h>
#include <Protocol/SimpleNetwork.h>

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
  BOOLEAN            Assigned;
  EFI_MAC_ADDRESS    EmulationAddress;
  EFI_MAC_ADDRESS    PermanentAddress;
} MAC_EMULATION_SNP_NOTIFY_CONTEXT;

typedef struct {
  LIST_ENTRY                          Link;
  EFI_HANDLE                          Handle;
  EFI_SIMPLE_NETWORK_PROTOCOL         *Snp;
  EFI_SIMPLE_NETWORK_INITIALIZE       OriginalInitialize;
  MAC_EMULATION_SNP_NOTIFY_CONTEXT    *MacContext;
} MAC_EMULATION_SNP_CONTEXT;

/**
  Performs sanity checks to ensure an snp can support mac emulation, and ensures that multiple interfaces are not programmed.

  @param[in] SnpHandle - A handle to an Snp
  @param[in] Snp - An snp protocol associated with the handle provided
  @param[in] SnpContext - The snp context created by this driver's entry point
  @retval TRUE - the SNP supports mac emulation and can be programmed with the emulated address
  @retval FALSE - the SNP should not be programmed with the emulated address
**/
BOOLEAN
SnpSupportsMacEmuCheck (
  IN CONST EFI_HANDLE                        SnpHandle,
  IN CONST EFI_SIMPLE_NETWORK_PROTOCOL       *Snp,
  IN CONST MAC_EMULATION_SNP_NOTIFY_CONTEXT  *SnpContext
  );

/**
  Sets the provided SNP's station address using the context information provided

  @param[in] Snp - Non-NULL pointer to an SNP which supports station address programming
  @param[in] Context - The snp context created by this driver's entry point

  @remark  Modifies the provided SNP's station address
**/
VOID
SetSnpMacViaContext (
  IN EFI_SIMPLE_NETWORK_PROTOCOL           *Snp,
  IN OUT MAC_EMULATION_SNP_NOTIFY_CONTEXT  *Context
  );

/**
  Wrap Initialize on installed SNP instances before they are used.

  @param[in]  Event   - The EFI_EVENT for this notify.
  @param[in]  Context - An instance of MAC_EMULATION_SNP_NOTIFY_CONTEXT.

**/
VOID
EFIAPI
SimpleNetworkProtocolNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Initialize SNP and apply MAC emulation before returning to its caller.

  @param[in] This               The SNP interface being initialized.
  @param[in] ExtraRxBufferSize  Additional receive buffer size.
  @param[in] ExtraTxBufferSize  Additional transmit buffer size.

  @return The status from the original Initialize method.
**/
EFI_STATUS
EFIAPI
MacEmulationSnpInitialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN UINTN                        ExtraRxBufferSize OPTIONAL,
  IN UINTN                        ExtraTxBufferSize OPTIONAL
  );

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
  );

#endif // _MAC_ADDRESS_EMULATION_DXE_H_
