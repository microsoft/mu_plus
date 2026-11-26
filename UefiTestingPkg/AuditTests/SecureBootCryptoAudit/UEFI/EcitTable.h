/** @file
  Bounded, platform-independent ECIT structural validation.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ECIT_TABLE_H_
#define ECIT_TABLE_H_

#include <Uefi.h>

#define MAX_ECIT_LENGTH  SIZE_1MB

/**
  Validate an ECIT without locating it or changing audit claim state.

  @param[in] Buffer      Readable bytes containing the ECIT.
  @param[in] BufferSize  Number of readable bytes in Buffer.

  @retval EFI_SUCCESS           The complete declared table is structurally valid.
  @retval EFI_INVALID_PARAMETER Buffer is NULL.
  @retval EFI_CRC_ERROR         The table checksum is invalid.
  @retval EFI_COMPROMISED_DATA  The header, entries, or bounds are invalid.
**/
EFI_STATUS
ValidateEcitTable (
  IN CONST VOID  *Buffer,
  IN UINTN       BufferSize
  );

#endif
