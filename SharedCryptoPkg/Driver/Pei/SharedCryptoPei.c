/** @file
  This module installs Crypto PPI used by Project Mu Firmware

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/PeiServicesLib.h>
#include <Ppi/SharedCryptoPpi.h>
#include <Library/DebugLib.h>

extern const SHARED_CRYPTO_FUNCTIONS mSharedCryptoFunctions;

CONST EFI_PEI_PPI_DESCRIPTOR mUefiCryptoPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gSharedCryptoPpiGuid,
  (SHARED_CRYPTO_PPI*) &mSharedCryptoFunctions
};


/**
Entry to MsWheaReportPei.

@param FileHandle                     The image handle.
@param PeiServices                    The PEI services table.

@retval Status                        From internal routine or boot object, should not fail
**/
EFI_STATUS
EFIAPI
SharedCryptoPeiEntry (
  IN EFI_PEI_FILE_HANDLE              FileHandle,
  IN CONST EFI_PEI_SERVICES           **PeiServices
  )
{
  //Install the Ppi
  EFI_STATUS Status = (*PeiServices)->InstallPpi (PeiServices, &mUefiCryptoPpiList);
  return Status;
}
