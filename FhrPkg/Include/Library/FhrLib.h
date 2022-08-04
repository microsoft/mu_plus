/** @file
  Definitions for the FHR helper library.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FHR_LIB_H__
#define __FHR_LIB_H__

#include <Fhr.h>

/**
  Validates a FHR firmware data block.

  @param[in]  FhrFwData     The firmware data block to be validated.

  @retval   EFI_SUCCESS               The data block was successfully validated.
  @retval   RETURN_INVALID_PARAMETER  Failed to validated data block.
**/
EFI_STATUS
EFIAPI
FhrValidateFwData (
  IN  FHR_FW_DATA  *FhrFwData
  );

/**
  Recalculates the firmware data block checksum.

  @param[in,out]  FhrFwData     The firmware data block be updated.

  @retval   EFI_SUCCESS               The data block was successfully validated.
  @retval   RETURN_INVALID_PARAMETER  Failed to validated data block.
**/
VOID
EFIAPI
FhrUpdateFwDataChecksum (
  IN OUT FHR_FW_DATA  *FhrFwData
  );

#endif
