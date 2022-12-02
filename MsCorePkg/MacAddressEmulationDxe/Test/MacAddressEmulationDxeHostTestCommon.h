/** @file

  Test file common header for MAC Address Emulation.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MAC_ADDRESS_EMULATION_HOST_TEST_COMMON_H
#define _MAC_ADDRESS_EMULATION_HOST_TEST_COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/NetLib.h>
#include <Library/UnitTestLib.h>
#include <Library/DebugLib.h>

#include <Protocol/SimpleNetwork.h>

#include "../MacAddressEmulationDxe.h"

extern EFI_BOOT_SERVICES mBootServices;

/**
  @brief  Stub LocateHandleBuffer function
**/
EFI_STATUS
EFIAPI 
LocateHandleBuffer (
  IN     EFI_LOCATE_SEARCH_TYPE       SearchType,
  IN     EFI_GUID                     *Protocol       OPTIONAL,
  IN     VOID                         *SearchKey      OPTIONAL,
  OUT    UINTN                        *NoHandles,
  OUT    EFI_HANDLE                   **Buffer
  );

/**
  @brief  Stub HandleProtocol function
**/
EFI_STATUS
EFIAPI
HandleProtocol (
  IN  EFI_HANDLE               Handle,
  IN  EFI_GUID                 *Protocol,
  OUT VOID                     **Interface
  );

/**
  @brief  Stub RestoreTPL function
**/
VOID
EFIAPI 
RestoreTPL (
  IN EFI_TPL      OldTpl
  );

/**
  @brief  Stub RaiseTPL function
**/
EFI_TPL
EFIAPI 
RaiseTPL (
  IN EFI_TPL      NewTpl
  );

#endif // _MAC_ADDRESS_EMULATION_HOST_TEST_COMMON_H