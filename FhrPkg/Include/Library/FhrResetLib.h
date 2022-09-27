/** @file
  Definitions for the FHR library to help the platform with resets.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FHR_RESET_H_
#define FHR_RESET_H_

#include <Fhr.h>

EFI_STATUS
EFIAPI
FhrCheckResetData (
  OUT BOOLEAN         *IsFhr,
  OUT FHR_RESET_DATA  **FhrResetData,
  IN UINTN            DataSize,
  IN VOID             *ResetData
  );

EFI_STATUS
EFIAPI
FhrSetIndicator (
  IN  FHR_RESET_DATA  *FhrResetData
  );

#endif
