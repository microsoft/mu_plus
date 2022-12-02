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
  VOID            *Registration;
  BOOLEAN         Assigned;
  EFI_MAC_ADDRESS EmulationAddress;
  EFI_MAC_ADDRESS PermanentAddress;
} MAC_EMULATION_SNP_NOTIFY_CONTEXT;

/**
  @brief  Function signature for SNP matching to provide to other functions.
**/
typedef
BOOLEAN
(*SNP_MATCH_FUNCTION)(
  IN CONST EFI_HANDLE SnpHandle,
  IN CONST EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  OPTIONAL IN CONST MAC_EMULATION_SNP_NOTIFY_CONTEXT *SnpContext
  );

/**
  @brief Performs sanity checks to ensure an snp can support mac emulation, and ensures that multiple interfaces are not programmed.
  @param[in] SnpHandle - A handle to an Snp
  @param[in] Snp - An snp protocol associatrred with the handle provided
  @param[in] SnpContext - The snp context created by this driver's entry point
  @retval TRUE - the SNP supports mac emulation and can be programmed with the emulated address
  @retval FALSE - the SNP should not be programmed with the emulated address
**/
BOOLEAN
SnpSupportsMacEmuCheck (
  IN CONST EFI_HANDLE SnpHandle,
  IN CONST EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  IN CONST MAC_EMULATION_SNP_NOTIFY_CONTEXT *SnpContext
  );

/**
  @brief  Iterates through all available SNPs available and finds the first instance which meets the criteria specified by match function
  @param[in] MatchFunction - Function pointer to caller provided function which will check whether the SNP matches
  @param[in] MatchFunctionContext - The snp context created by this driver's entry point
  @retval  NULL - no matching SNP was found, or invalid input parameter
  @retval  NON-NULL - a pointer to the first matching SNP
**/
EFI_SIMPLE_NETWORK_PROTOCOL*
FindMatchingSnp (
  IN SNP_MATCH_FUNCTION MatchFunction,
  OPTIONAL IN MAC_EMULATION_SNP_NOTIFY_CONTEXT *MatchFunctionContext
  );

/**
  @brief  Sets the provided SNP's station address using the context information provided
  @param[in] Snp - Non-NULL pointer to an SNP which supports station address programming 
  @param[in] Context - The snp context created by this driver's entry point

  @remark  Modifes the provided SNP's station address
**/
VOID
SetSnpMacViaContext (
  IN EFI_SIMPLE_NETWORK_PROTOCOL* Snp,
  IN OUT MAC_EMULATION_SNP_NOTIFY_CONTEXT *Context
  );

/**
  Callback that is invoked when an SNP instance is Initialized. Checks the newly installed SNP registrations (if any)
  and updates the MAC address if a supported adapter is found.

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
  );

#endif // _MAC_ADDRESS_EMULATION_DXE_H_