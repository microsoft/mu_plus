/** @file AcpiRgrt.c
 *
 *This driver provides an entry in the ACPI table that provides a PNG graphic
  for regulatory purposes.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <IndustryStandard/Acpi.h>
#include <Library/SafeIntLib.h>
#include <AcpiRgrt.h>
#include <Protocol/AcpiTable.h>
#include <Library/DxeServicesLib.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>

UINTN                             mRgImageSize;
UINT8*                            mRgImageData;



/**
  Install RGRT ACPI Table when ACPI Table Protocol is available.

  The Regulatory GraphicResource Table (RGRT) is an optional ACPI
  table that enables boot firmware to provide the operating
  system with color and contrast used to paint the screen
  background during boot.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
VOID
EFIAPI
InstallAcpiTable (
  IN EFI_EVENT                      Event,
  IN VOID*                          Context
  )
{
  UINTN                             TableKey;
  EFI_STATUS                        Status;
  EFI_ACPI_TABLE_PROTOCOL           *AcpiTable;
  UINT8                             Checksum;
  MSFT_RGRT_ACPI_TABLE*             RgrtAcpiTable;
  UINT32                            RgrtTableSize;
  UINT64                            OemTableId;

  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTable);
  if (EFI_ERROR (Status)) { // we are expected to be called at least once before ACPI is installed
    return;
  }

  if (mRgImageSize == 0 || mRgImageData == NULL) {
    DEBUG((DEBUG_ERROR, "ACPI RGRT graphic not found\n"));
    goto cleanup;
  }

  // Figure out the size we need, which is struct + image
  Status = SafeUint32Add(sizeof(MSFT_RGRT_ACPI_TABLE), (UINT32) mRgImageSize, &RgrtTableSize);

  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "ACPI RGRT table- image is too large\n"));
    goto cleanup;
  }
  // Now that we know the size, allocate the struct
  RgrtAcpiTable = (MSFT_RGRT_ACPI_TABLE*) AllocateZeroPool(RgrtTableSize);

  // Check to make sure we actually have the memory
  if (RgrtAcpiTable == NULL) {
    DEBUG((DEBUG_ERROR, "ACPI RGRT table failed to allocate %x of memory for table\n", RgrtTableSize));
    goto cleanup;
  }

  // Add the signature, length, and revision
  RgrtAcpiTable->Header.Signature = MSFT_ACPI_REGULATORY_GRAPHIC_RESOURCE_TABLE_SIGNATURE;
  RgrtAcpiTable->Header.Length = RgrtTableSize;
  RgrtAcpiTable->Header.Revision = MSFT_ACPI_REGULATORY_GRAPHIC_RESOURCE_TABLE_IMAGE_REVISION;

  // Copy the default parameters into the table headers
  CopyMem (&RgrtAcpiTable->Header.OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (RgrtAcpiTable->Header.OemId));
  OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
  CopyMem (&RgrtAcpiTable->Header.OemTableId, &OemTableId, sizeof (OemTableId));

  RgrtAcpiTable->Header.OemRevision      = PcdGet32 (PcdAcpiDefaultOemRevision);
  RgrtAcpiTable->Header.CreatorId        = PcdGet32 (PcdAcpiDefaultCreatorId);
  RgrtAcpiTable->Header.CreatorRevision  = PcdGet32 (PcdAcpiDefaultCreatorRevision);

  // Set the version
  RgrtAcpiTable->Version = MSFT_ACPI_REGULATORY_GRAPHIC_RESOURCE_TABLE_IMAGE_VERSION;
  // Set the image type
  RgrtAcpiTable->ImageType = MSFT_ACPI_REGULATORY_GRAPHIC_RESOURCE_TABLE_IMAGE_TYPE_PNG;

  // Copy the image into the table
  CopyMem (&RgrtAcpiTable->Image, mRgImageData, mRgImageSize);

  //
  // The ACPI table must be checksumed before calling the InstallAcpiTable()
  // service of the ACPI table protocol to install it.
  //
  Checksum = CalculateCheckSum8 ((UINT8 *)RgrtAcpiTable, RgrtTableSize);
  RgrtAcpiTable->Header.Checksum = Checksum;

  //Install the table
  Status = AcpiTable->InstallAcpiTable (
                          AcpiTable,
                          RgrtAcpiTable,
                          RgrtTableSize,
                          &TableKey
                          );

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "ACPI RGRT table failed to install: %r\n",Status));
  }

cleanup:
  // If we got here by an event, make sure we close it
  if (Event != NULL) {
    gBS->CloseEvent(Event);
  }

  //Make sure to free the memory we allocated
  if (RgrtAcpiTable != NULL) {
    FreePool(RgrtAcpiTable);
  }

  if (mRgImageData != NULL) {
    FreePool(mRgImageData);
    mRgImageData = NULL;
    mRgImageSize = 0;
  }
}


/**
  The driver's entry point.

  It produces ACPI Regulatory GraphicResource Table (RGRT)

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
DriverEntry (
  IN    EFI_HANDLE                  ImageHandle,
  IN    EFI_SYSTEM_TABLE            *SystemTable
  )
{
  VOID                              *Registration;
  EFI_STATUS                        Status;
  EFI_GUID*                         ImageLocationGuid;

  ImageLocationGuid = PcdGetPtr(PcdRegulatoryGraphicFileGuid); // Get the guid of the regulatory graphic

  if (ImageLocationGuid == NULL) {
    DEBUG((DEBUG_ERROR, "ACPI RGRT failed to find Graphic Image GUID\n"));
    return EFI_ABORTED;
  }
  // Get the image out of the FV
  Status = GetSectionFromAnyFv(ImageLocationGuid, EFI_SECTION_RAW, 0, (VOID **)&mRgImageData, &mRgImageSize);
  // if we didn't find it
  if (EFI_ERROR (Status) || mRgImageSize == 0) {
    DEBUG((DEBUG_ERROR, "ACPI RGRT table failed to find Graphic Image location\n"));
    return EFI_ABORTED;
  }
  // register for a call back (we will be called immediately as well)
  EfiCreateProtocolNotifyEvent (&gEfiAcpiTableProtocolGuid, TPL_CALLBACK, InstallAcpiTable, NULL, &Registration);

  return EFI_SUCCESS;
}
