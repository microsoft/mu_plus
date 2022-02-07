/** @file
  Simple SerialLib based Status Code handler

Copyright (C) Microsoft Corporation. All rights reserved.

  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiMm.h>
#include "StatusCodeHandlerMm.h"

/**
  Status Code Traditional MM Entry point.

  Register this handler with the Traditional MM Router

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS  Successfully registered

**/
EFI_STATUS
EFIAPI
TraditionalEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return MmEntry ();
}
