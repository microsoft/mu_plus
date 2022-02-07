/** @file

  This header defines a set of interfaces to determine if a given capsule is
  supported by this firmware. This includes, but is not limited to, checking the
  GUID and doing basic validation.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _IS_CAPSULE_SUPPORTED_H__
#define _IS_CAPSULE_SUPPORTED_H__

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
  );

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
  );

/**
  Return if this FMP is a system FMP or a device FMP, based upon CapsuleHeader.

  @param[in]  CapsuleHeader   A pointer to EFI_CAPSULE_HEADER

  @retval     TRUE            It is a system FMP or device FMP.
  @retval     FALSE           It is a not an FMP capsule.

**/
BOOLEAN
EFIAPI
IsFmpCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  );

/**
  Return if this FMP is a graphics capsule

  @param[in]  CapsuleHeader A pointer to EFI_CAPSULE_HEADER

  @retval     TRUE          It is a graphics capsule.
  @retval     FALSE         It is not a graphics capsule.

**/
BOOLEAN
EFIAPI
IsGraphicsCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  );

#endif
