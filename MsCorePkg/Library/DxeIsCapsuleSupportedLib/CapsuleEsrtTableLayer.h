/** @file

  This headers defines a set of interfaces to communicate with the FMP devices
  (via the ESRT table) to determine if a given capsule GUID matches a device
  represented in the ESRT table.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DXE_CAPSULE_ESRT_TABLE_LAYER_H__
#define __DXE_CAPSULE_ESRT_TABLE_LAYER_H__

/**
  Checks if a given Capsule GUID matchs the FwClass of an item on the ESRT

  @param[in]  CapsuleGuid Pointer to a GUID

  @retval     TRUE        Successfully found a match on the ESRT
  @retval     FALSE       We didn't find a match on the ESRT

*/
BOOLEAN
EFIAPI
IsCapsuleGuidInEsrtTable (
  IN EFI_GUID  *CapsuleGuid
  );

#endif
