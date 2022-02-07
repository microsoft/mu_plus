/** @file
  Simple SerialLib based Status Code handler

Copyright (C) Microsoft Corporation. All rights reserved.

  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiMm.h>
#include "StatusCodeHandlerMm.h"

/**
  Status Code Standalone MM Entry point.

  Register this handler with the Standalone MM Router

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS  Successfully registered

**/
EFI_STATUS
EFIAPI
StandaloneEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return MmEntry ();
}
