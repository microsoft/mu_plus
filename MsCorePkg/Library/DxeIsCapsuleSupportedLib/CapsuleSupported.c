/** @file
  Library for determining if a given capsule is supported by the firmware

  Caution: This module requires additional review when modified.
  This module will have external input - capsule image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  IsCapsuleImageSupported() and DoSanityCheckOnFmpCapsule()
  receive untrusted input and perform basic validation.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <IndustryStandard/WindowsUxCapsule.h>

#include <Guid/FmpCapsule.h>
#include <Guid/SystemResourceTable.h>
#include <Guid/EventGroup.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IsCapsuleSupportedLib.h>

#include "CapsuleEsrtTableLayer.h"

/**
  Validate if it is valid capsule header

  Caution: This function may receive untrusted input.

  This function assumes the caller provided correct CapsuleHeader pointer
  and CapsuleSize.

  This function validates the fields in EFI_CAPSULE_HEADER.

  @param[in]  CapsuleHeader   Points to a capsule header.
  @param[in]  CapsuleSize     Size of the whole capsule image.

  @retval     TRUE            It is a valid capsule header.
  @retval     FALSE           It is not a valid capsule header.

**/
STATIC
BOOLEAN
IsValidCapsuleHeader (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader,
  IN UINT64              CapsuleSize
  )
{
  if (CapsuleHeader == NULL) {
    // do a quick investment
    return FALSE;
  }

  if (CapsuleHeader->CapsuleImageSize != CapsuleSize) {
    return FALSE;
  }

  if (CapsuleHeader->HeaderSize >= CapsuleHeader->CapsuleImageSize) {
    return FALSE;
  }

  return TRUE;
}

/**
  Return if there is a FMP header below capsule header.

  @param[in]  CapsuleHeader A pointer to EFI_CAPSULE_HEADER

  @retval     TRUE          There is a FMP header below capsule header.
  @retval     FALSE         There is not a FMP header below capsule header
**/
STATIC
BOOLEAN
IsNestedFmpCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  EFI_CAPSULE_HEADER  *NestedCapsuleHeader;
  UINTN               NestedCapsuleSize;

  // Do a quick null check
  if (CapsuleHeader == NULL) {
    return FALSE;
  }

  // Check the top level guid against the ESRT Table
  if (IsCapsuleGuidInEsrtTable (&CapsuleHeader->CapsuleGuid) == FALSE) {
    return FALSE;
  }

  //
  // Check nested capsule header
  // FMP GUID after ESRT one
  //
  NestedCapsuleHeader = (EFI_CAPSULE_HEADER *)((UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize);
  NestedCapsuleSize   = (UINTN)CapsuleHeader + CapsuleHeader->CapsuleImageSize - (UINTN)NestedCapsuleHeader;
  if (NestedCapsuleSize < sizeof (EFI_CAPSULE_HEADER)) {
    return FALSE;
  }

  if (!IsValidCapsuleHeader (NestedCapsuleHeader, NestedCapsuleSize)) {
    return FALSE;
  }

  if (!IsFmpCapsuleGuid (&NestedCapsuleHeader->CapsuleGuid)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Does a rough sanity check for an fmp capsule.

  It focuses on layout and general correctness rather than authentication
  or if this capsule matches an FMP device on this system.

  Caution: This function may receive untrusted input.

  This function assumes the caller validated the capsule by using
  IsValidCapsuleHeader(), so that all fields in EFI_CAPSULE_HEADER are correct.
  The capsule buffer size is CapsuleHeader->CapsuleImageSize.

  This function validates the fields in EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER
  and EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER.

  This function need support nested FMP capsule.

  @param[in]  CapsuleHeader           Points to a capsule header.

  @retval     EFI_SUCCESS             Input capsule is a correct FMP capsule.
  @retval     EFI_INVALID_PARAMETER   Input capsule is not a correct FMP capsule.
**/
STATIC
EFI_STATUS
DoSanityCheckOnFmpCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER        *FmpCapsuleHeader;
  UINT8                                         *EndOfCapsule;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader;
  UINT8                                         *EndOfPayload;
  UINT64                                        *ItemOffsetList;
  UINT32                                        ItemNum;
  UINTN                                         Index;
  UINTN                                         FmpCapsuleSize;
  UINTN                                         FmpCapsuleHeaderSize;
  UINT64                                        FmpImageSize;
  UINTN                                         FmpImageHeaderSize;

  // Do a quick null check
  if (CapsuleHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsFmpCapsuleGuid (&CapsuleHeader->CapsuleGuid)) {
    return DoSanityCheckOnFmpCapsule ((EFI_CAPSULE_HEADER *)((UINTN)CapsuleHeader + CapsuleHeader->HeaderSize));
  }

  if (CapsuleHeader->HeaderSize >= CapsuleHeader->CapsuleImageSize) {
    DEBUG ((DEBUG_ERROR, "[%a] -HeaderSize(0x%x) >= CapsuleImageSize(0x%x)\n", __FUNCTION__, CapsuleHeader->HeaderSize, CapsuleHeader->CapsuleImageSize));
    return EFI_INVALID_PARAMETER;
  }

  FmpCapsuleHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER *)((UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize);
  EndOfCapsule     = (UINT8 *)CapsuleHeader + CapsuleHeader->CapsuleImageSize;
  FmpCapsuleSize   = (UINTN)EndOfCapsule - (UINTN)FmpCapsuleHeader;

  if (FmpCapsuleSize < sizeof (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER)) {
    DEBUG ((DEBUG_ERROR, "[%a] -FmpCapsuleSize(0x%x) < EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER\n", __FUNCTION__, FmpCapsuleSize));
    return EFI_INVALID_PARAMETER;
  }

  // Check EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER
  if (FmpCapsuleHeader->Version != EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER_INIT_VERSION) {
    DEBUG ((DEBUG_ERROR, "[%a] -FmpCapsuleHeader->Version(0x%x) != EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER_INIT_VERSION\n", __FUNCTION__, FmpCapsuleHeader->Version));
    return EFI_INVALID_PARAMETER;
  }

  ItemOffsetList = (UINT64 *)(FmpCapsuleHeader + 1);

  ItemNum = FmpCapsuleHeader->PayloadItemCount;

  // Currently we do not support Embedded Drivers.
  // This opens up concerns about validating the driver as we can't trust secure boot chain (pk)
  if (FmpCapsuleHeader->EmbeddedDriverCount != 0) {
    DEBUG ((DEBUG_ERROR, "[%a] - FMP Capsule contains an embedded driver.  This is not supported by this implementation\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  if ((FmpCapsuleSize - sizeof (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER))/sizeof (UINT64) < ItemNum) {
    DEBUG ((DEBUG_ERROR, "[%a] -ItemNum(0x%x) too big\n", ItemNum));
    return EFI_INVALID_PARAMETER;
  }

  FmpCapsuleHeaderSize = sizeof (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER) + (sizeof (UINT64) * ItemNum);

  // Check ItemOffsetList
  for (Index = 0; Index < ItemNum; Index++) {
    if (ItemOffsetList[Index] >= FmpCapsuleSize) {
      DEBUG ((DEBUG_ERROR, "[%a] -ItemOffsetList[%d](0x%lx) >= FmpCapsuleSize(0x%x)\n", __FUNCTION__, Index, ItemOffsetList[Index], FmpCapsuleSize));
      return EFI_INVALID_PARAMETER;
    }

    if (ItemOffsetList[Index] < FmpCapsuleHeaderSize) {
      DEBUG ((DEBUG_ERROR, "[%a] -ItemOffsetList[%d](0x%lx) < FmpCapsuleHeaderSize(0x%x)\n", __FUNCTION__, Index, ItemOffsetList[Index], FmpCapsuleHeaderSize));
      return EFI_INVALID_PARAMETER;
    }

    //
    // All the address in ItemOffsetList must be stored in ascending order
    //
    if (Index > 0) {
      if (ItemOffsetList[Index] <= ItemOffsetList[Index - 1]) {
        DEBUG ((DEBUG_ERROR, "[%a] -ItemOffsetList[%d](0x%lx) < ItemOffsetList[%d](0x%x)\n", __FUNCTION__, Index, ItemOffsetList[Index], Index - 1, ItemOffsetList[Index - 1]));
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  // Check EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER
  for (Index = 0; Index < ItemNum; Index++) {
    ImageHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *)((UINT8 *)FmpCapsuleHeader + ItemOffsetList[Index]);
    if (Index == ItemNum - 1) {
      EndOfPayload = (UINT8 *)((UINTN)EndOfCapsule - (UINTN)FmpCapsuleHeader);
    } else {
      EndOfPayload = (UINT8 *)(UINTN)ItemOffsetList[Index+1];
    }

    FmpImageSize = (UINTN)EndOfPayload - ItemOffsetList[Index];

    FmpImageHeaderSize = sizeof (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER);
    if ((ImageHeader->Version > EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION) ||
        (ImageHeader->Version < 1))
    {
      DEBUG ((DEBUG_ERROR, "[%a] -ImageHeader->Version(0x%x) Unknown\n", __FUNCTION__, ImageHeader->Version));
      return EFI_INVALID_PARAMETER;
    }

    if (ImageHeader->Version == 1) {
      FmpImageHeaderSize = OFFSET_OF (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER, UpdateHardwareInstance);
    } else if (ImageHeader->Version == 2) {
      FmpImageHeaderSize = OFFSET_OF (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER, ImageCapsuleSupport);
    }

    if (FmpImageSize < FmpImageHeaderSize) {
      DEBUG ((DEBUG_ERROR, "[%a] -FmpImageSize(0x%lx) < FmpImageHeaderSize(0x%x)\n", __FUNCTION__, FmpImageSize, FmpImageHeaderSize));
      return EFI_INVALID_PARAMETER;
    }

    // No overflow
    if (FmpImageSize != (UINT64)FmpImageHeaderSize + (UINT64)ImageHeader->UpdateImageSize + (UINT64)ImageHeader->UpdateVendorCodeSize) {
      DEBUG ((DEBUG_ERROR, "[%a] -FmpImageSize(0x%lx) mismatch, UpdateImageSize(0x%x) UpdateVendorCodeSize(0x%x)\n", __FUNCTION__, FmpImageSize, ImageHeader->UpdateImageSize, ImageHeader->UpdateVendorCodeSize));
      return EFI_INVALID_PARAMETER;
    }
  }

  if (ItemNum == 0) {
    // No driver & payload element in FMP
    EndOfPayload = (UINT8 *)(FmpCapsuleHeader + 1);
    if (EndOfPayload != EndOfCapsule) {
      DEBUG ((DEBUG_ERROR, "[%a] -EndOfPayload(0x%x) mismatch, EndOfCapsule(0x%x)\n", __FUNCTION__, EndOfPayload, EndOfCapsule));
      return EFI_INVALID_PARAMETER;
    }

    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

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
  // quick null check
  if (CapsuleHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // check Display Capsule Guid
  if (IsGraphicsCapsule (CapsuleHeader)) {
    // Simple sanity check
    if (CapsuleHeader->HeaderSize >= CapsuleHeader->CapsuleImageSize) {
      DEBUG ((DEBUG_ERROR, "[%a] -HeaderSize(0x%x) >= CapsuleImageSize(0x%x)\n", __FUNCTION__, CapsuleHeader->HeaderSize, CapsuleHeader->CapsuleImageSize));
      return EFI_INVALID_PARAMETER;
    }

    return EFI_SUCCESS;
  }

  if (IsFmpCapsule (CapsuleHeader)) {
    // Fake capsule header is valid case in QueryCapsuleCapabilities().
    if (CapsuleHeader->HeaderSize == CapsuleHeader->CapsuleImageSize) {
      return EFI_SUCCESS;
    }

    //
    // Check layout of FMP capsule
    return DoSanityCheckOnFmpCapsule (CapsuleHeader);
  }

  DEBUG ((DEBUG_ERROR, "Unknown Capsule Guid - %g\n", &CapsuleHeader->CapsuleGuid));
  return EFI_UNSUPPORTED;
}

/**
  Return if this CapsuleGuid is a FMP capsule GUID or not.

  @param[in]  CapsuleGuid A pointer to an EFI_GUID

  @retval     TRUE        It is a FMP capsule GUID.
  @retval     FALSE       It is not a FMP capsule GUID.

**/
BOOLEAN
EFIAPI
IsFmpCapsuleGuid (
  IN EFI_GUID  *CapsuleGuid
  )
{
  // do a quick null check
  if (CapsuleGuid == NULL) {
    return FALSE;
  }

  return CompareGuid (&gEfiFmpCapsuleGuid, CapsuleGuid);
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
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  // Do a quick null check
  if (CapsuleHeader == NULL) {
    return FALSE;
  }

  return IsFmpCapsuleGuid (&CapsuleHeader->CapsuleGuid) || IsNestedFmpCapsule (CapsuleHeader);
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
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  // Do a quick null check
  if (CapsuleHeader == NULL) {
    return FALSE;
  }

  return CompareGuid (&(CapsuleHeader->CapsuleGuid), &gWindowsUxCapsuleGuid);
}
