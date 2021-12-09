/** @file
  Library for determining if a given capsule is supported by firmware.

  Caution: This module requires additional review when modified.
  This module will have external input - capsule image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  IsCapsuleImageSupported() and DoSanityCheckOnFmpCapsule()
  receive untrusted input and perform basic validation.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
  The firmware checks whether the capsule image is supported by the CapsuleGuid
  in CapsuleHeader or if there is other specific information in the capsule image.
  This also does basic validation of the capsule itself.

  Caution: This function may receive untrusted input.

  @param[in]  CapsuleHeader   Pointer to the UEFI capsule image to be checked.

  @retval     EFI_SUCCESS     Input capsule is supported by firmware.
  @retval     EFI_UNSUPPORTED Input capsule is not supported by the firmware.

**/
EFI_STATUS
EFIAPI
IsCapsuleImageSupported (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  DEBUG ((DEBUG_ERROR, "[%a] This is a null lib and should not be used in production\n", __FUNCTION__));
  return EFI_UNSUPPORTED;
}

/**
  Return if this CapsuleGuid is a FMP capsule GUID or not.

  @param[in] CapsuleGuid  A pointer to an EFI_GUID

  @retval TRUE            It is a FMP capsule GUID.
  @retval FALSE           It is not a FMP capsule GUID.

**/
BOOLEAN
EFIAPI
IsFmpCapsuleGuid (
  IN EFI_GUID  *CapsuleGuid
  )
{
  DEBUG ((DEBUG_ERROR, "[%a] This is a null lib and should not be used in production\n", __FUNCTION__));
  return FALSE;
}

/**
  Return if this FMP is a system FMP or a device FMP, based upon CapsuleHeader.

  @param[in]  CapsuleHeader   A pointer to EFI_CAPSULE_HEADER

  @retval     TRUE            It is a system FMP or device FMP.
  @retval     FALSE           It is a not an FMP capsule.

**/
BOOLEAN
EFIAPI
IsFmpCapsule (
  IN EFI_CAPSULE_HEADER         *CapsuleHeader
  )
{
  DEBUG ((DEBUG_ERROR, "[%a] This is a null lib and should not be used in production\n", __FUNCTION__));
  return FALSE;
}


/**
  Return if this FMP is a graphics capsule

  @param[in]  CapsuleHeader A pointer to EFI_CAPSULE_HEADER

  @retval     TRUE          It is a graphics capsule.
  @retval     FALSE         It is not a graphics capsule.

**/
BOOLEAN
EFIAPI
IsGraphicsCapsule (
  IN EFI_CAPSULE_HEADER         *CapsuleHeader
  )
{
  DEBUG ((DEBUG_ERROR, "[%a] This is a null lib and should not be used in production\n", __FUNCTION__));
  return FALSE;
}
