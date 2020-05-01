/** @file -- TpmSgNvIndexLib.h

  Interfaces to define NV index for SystemGuard.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TPM_SG_NV_INDEX_LIB_H_
#define _TPM_SG_NV_INDEX_LIB_H_

//
// Constant definitions from Mircosoft "System Guard Secure Launch and SMM
// Protection", section "System requirements for System Guard"
#define SG_NV_INDEX_HANDLE    0x01C101C0
#define SG_NV_INDEX_SIZE      128

/**
  Executes DefineSpace for SystemGuard NV Index used by the OS, with
  requirements in Mircosoft "System Guard Secure Launch and SMM Protection",
  section "System requirements for System Guard", item "TPM NV Index"

  @retval     EFI_SUCCESS         NV Index was defined.
  @retval     EFI_ALREADY_STARTED NV Index has been defined already.
  @retval     EFI_DEVICE_ERROR    Tpm2NvReadPublic() returned an unexpected error.
  @retval     Others              Tpm2NvDefineSpace() returned an error.
**/
EFI_STATUS
EFIAPI
DefineSgTpmNVIndexforOS (
  VOID
  );

#endif // _TPM_SG_NV_INDEX_LIB_H_
