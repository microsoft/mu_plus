/** @file
  Definitions for the FHR boot manager library.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FHR_LIB_H__
#define __FHR_LIB_H__

#include <Uefi.h>

/**
  Handles the FHR resume process. This routine will not return if this is an
  FHR resume.

  @retval   EFI_SUCCESS     Not an FHR resume, no work needed.
**/
EFI_STATUS
EFIAPI
FhrBootManager (
  VOID
  );

#endif
