/** @file
  Definitions for the FHR helper library.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FHR_LIB_H__
#define __FHR_LIB_H__

#include <Fhr.h>

EFI_STATUS
EFIAPI
FhrValidateFwData (
  IN  FHR_FW_DATA  *FhrFwData
  );

#endif
