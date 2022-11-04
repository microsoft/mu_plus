/** @file

  DXE Driver header for handling MAC Address Emulation.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

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

typedef
BOOLEAN
(*SNP_MATCH_FUNCTION)(
  IN CONST EFI_HANDLE SnpHandle,
  IN CONST EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  OPTIONAL IN CONST VOID *SnpContext
  );

BOOLEAN
SnpSupportsMacEmuCheck (
  IN CONST EFI_HANDLE SnpHandle,
  IN CONST EFI_SIMPLE_NETWORK_PROTOCOL *Snp,
  IN CONST VOID *SnpContext
  );

EFI_SIMPLE_NETWORK_PROTOCOL*
FindMatchingSnp (
  SNP_MATCH_FUNCTION MatchFunction,
  VOID *MatchFunctionContext
  );

VOID
SetSnpMacViaContext (
  IN EFI_SIMPLE_NETWORK_PROTOCOL* Snp,
  IN OUT MAC_EMULATION_SNP_NOTIFY_CONTEXT *Context
  );

VOID
EFIAPI
SimpleNetworkProtocolNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

EFI_STATUS
EFIAPI
MacAddressEmulationEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE* SystemTable
  );