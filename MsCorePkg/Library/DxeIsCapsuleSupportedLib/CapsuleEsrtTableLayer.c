/** @file
  ESRT Table Layer for Capsules to access.

  This should not leak any details of the ESRT table.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include "CapsuleEsrtTableLayer.h"

/**
  Checks if a given Capsule GUID matches the FwClass of an item on the ESRT

  @param[in]  CapsuleGuid Pointer to a GUID

  @retval     TRUE        Successfully found a match on the ESRT
  @retval     FALSE       We didn't find a match on the ESRT

*/
BOOLEAN
EFIAPI
IsCapsuleGuidInEsrtTable (
  IN EFI_GUID  *CapsuleGuid
  )
{
  // TODO: query the ESRT cache to determine if the FWClass matches an entry
  // For now we always assume that it is in the ESRT since the FMP devices aren't
  // loaded on a certified boot
  return TRUE;
}
